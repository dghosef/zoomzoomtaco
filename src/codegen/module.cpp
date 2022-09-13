#include "taco/codegen/module.h"

#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#if USE_OPENMP
#include <omp.h>
#endif

#include "codegen/codegen_c.h"
#include "codegen/codegen_cuda.h"
#include "codegen/codegen_pochi.h"
#include "taco/cuda.h"
#include "taco/error.h"
#include "taco/tensor.h"
#include "taco/util/env.h"
#include "taco/util/strings.h"
#include "taco/ir/simplify.h"
#include <chrono>
#include <utility>

class AutoProfiler {
public:
  AutoProfiler(std::string name)
      : m_name(std::move(name)),
        m_beg(std::chrono::high_resolution_clock::now()) {}
  ~AutoProfiler() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::microseconds>(end - m_beg);
    std::cout << m_name << " : " << dur.count() << " musec" << std::endl;
  }

private:
  std::string m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
};
typedef std::chrono::high_resolution_clock::time_point TimeVar;

#define duration(a)                                                            \
  std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()

using namespace std;

namespace taco {
namespace ir {

Stmt fully_simplify(Stmt stmt) {
  Stmt oldStmt;
  do {
    oldStmt = stmt;
    stmt = ir::simplify(stmt);
  } while (stmt != oldStmt);
  return stmt;
}
std::string Module::chars = "abcdefghijkmnpqrstuvwxyz0123456789";
std::default_random_engine Module::gen = std::default_random_engine();
std::uniform_int_distribution<int> Module::randint =
    std::uniform_int_distribution<int>(0, chars.length() - 1);

void Module::setJITTmpdir() { tmpdir = util::getTmpdir(); }

void Module::setJITLibname() {
  libname.resize(12);
  for (int i = 0; i < 12; i++)
    libname[i] = chars[randint(gen)];
}

void Module::addFunction(Stmt func) { funcs.push_back(func); }

void Module::compilePochi() {
  std::vector<std::unique_ptr<CodeGen_Pochi>> codegens;
  for (auto func : funcs) {
    if(to<Function>(func)->name == "compute") {
      func = fully_simplify(func);
      func = rewriteForLoops(func);
    }
    // Only the name parameter matters
    std::unique_ptr<CodeGen_Pochi> sourcegen = std::unique_ptr<CodeGen_Pochi>(
        new CodeGen_Pochi(std::cout, CodeGen::ImplementationGen, func.as<Function>()->name));
    sourcegen->compile(func);
    codegens.push_back(std::move(sourcegen));
  }
  if(!PochiVM::thread_pochiVMContext->m_curModule->Validate()) {
    std::cout << "Module is invalid" << std::endl;
    std::cout << PochiVM::thread_errorContext->m_errorMsg << std::endl;
    exit(1);
  }
  PochiVM::thread_pochiVMContext->m_curModule->PrepareForFastInterp();

  {
    for (auto func : funcs) {
      PochiVM::FastInterpFunction<FnPrototype> interpFn =
          PochiVM::thread_pochiVMContext->m_curModule
              ->GetFastInterpGeneratedFunction<FnPrototype>(func.as<Function>()->name);
      fns.insert({func.as<Function>()->name, interpFn});
      volatile auto fp = interpFn.m_fnPtr;
    }
  }
}

void Module::compileToSource(string path, string prefix) {
  PochiVM::NewModule(path + prefix);
  bool usePochi = true;
  for(auto func : funcs) {
    if(!should_use_pochi_codegen(func.as<Function>()->name)) {
      usePochi = false;
      break;
    }
  }
  if (!moduleFromUserSource) {
    if (usePochi) {
      compilePochi();
    } else {
      // create a codegen instance and add all the funcs
      bool didGenRuntime = false;

      header.str("");
      header.clear();
      source.str("");
      source.clear();

      taco_tassert(target.arch == Target::C99)
          << "Only C99 codegen supported currently";
      std::shared_ptr<CodeGen> sourcegen =
          CodeGen::init_default(source, CodeGen::ImplementationGen);
      std::shared_ptr<CodeGen> headergen =
          CodeGen::init_default(header, CodeGen::HeaderGen);

      for (auto func : funcs) {
        if(to<Function>(func)->name == "compute") {
          func = fully_simplify(func);
          func = rewriteForLoops(func);
        }
        sourcegen->compile(func, !didGenRuntime);
        if (!usePochi) {
          headergen->compile(func, !didGenRuntime);
        }
        didGenRuntime = true;
      }
     //cout << source.str() << endl;
    }

    if (!usePochi) {
      ofstream source_file;
      string file_ending = should_use_CUDA_codegen() ? ".cu" : ".c";
      source_file.open(path + prefix + file_ending);
      source_file << source.str();
      source_file.close();

      ofstream header_file;
      header_file.open(path + prefix + ".h");
      header_file << header.str();
      header_file.close();
    }
  }
}

void Module::compileToStaticLibrary(string path, string prefix) {
  taco_tassert(false) << "Compiling to a static library is not supported";
}

namespace {

void writeShims(vector<Stmt> funcs, string path, string prefix) {
  stringstream shims;
  bool usePochi = true;
  for(auto func : funcs) {
    if(!should_use_pochi_codegen(func.as<Function>()->name)) {
      usePochi = false;
      break;
    }
  }
  for (auto func : funcs) {
    if (should_use_CUDA_codegen()) {
      CodeGen_CUDA::generateShim(func, shims);
    } else if (usePochi) {
      CodeGen_Pochi::generateShim(func, shims);
    } else {
      CodeGen_C::generateShim(func, shims);
    }
  }

  ofstream shims_file;
  if (should_use_CUDA_codegen()) {
    shims_file.open(path + prefix + "_shims.cpp");
  } else {
    shims_file.open(path + prefix + ".c", ios::app);
  }
  shims_file << "#include \"" << path << prefix << ".h\"\n";
  shims_file << shims.str();
  shims_file.close();
}

} // anonymous namespace

string Module::compile() {
  string prefix = tmpdir + libname;
  string fullpath = prefix + ".so";

  string cc;
  string cflags;
  string file_ending;
  string shims_file;
  if (should_use_CUDA_codegen()) {
    cc = util::getFromEnv("TACO_NVCC", "nvcc");
    cflags =
        util::getFromEnv("TACO_NVCCFLAGS", get_default_CUDA_compiler_flags());
    file_ending = ".cu";
    shims_file = prefix + "_shims.cpp";
  } else {
    cc = util::getFromEnv(target.compiler_env, target.compiler);
    cflags = util::getFromEnv("TACO_CFLAGS", "-O3 -ffast-math -std=c99") +
             " -shared -fPIC";
#if USE_OPENMP
    cflags += " -fopenmp";
#endif
    file_ending = ".c";
    shims_file = "";
  }

  cc = "gcc";
  string cmd = cc + " " + cflags + " " + prefix + file_ending + " " +
               shims_file + " " + "-o " + fullpath + " -lm";

  // open the output file & write out the source
  compileToSource(tmpdir, libname);

  bool usePochi = true;
  for(auto func : funcs) {
    if(!should_use_pochi_codegen(func.as<Function>()->name)) {
      usePochi = false;
      break;
    }
  }
  if (!usePochi) {
    // write out the shims
    writeShims(funcs, tmpdir, libname);

    // now compile it
    int err = system(cmd.data());
    taco_uassert(err == 0) << "Compilation command failed:\n"
                           << cmd << "\nreturned " << err;

    // use dlsym() to open the compiled library
    if (lib_handle) {
      dlclose(lib_handle);
    }
    lib_handle = dlopen(fullpath.data(), RTLD_NOW | RTLD_LOCAL);
    taco_uassert(lib_handle)
        << "Failed to load generated code, error is: " << dlerror();

    return fullpath;
  } else {
    return "";
  }
}

void Module::setSource(string source) {
  this->source << source;
  moduleFromUserSource = true;
}

string Module::getSource() { return source.str(); }

void *Module::getFuncPtr(std::string name) {
  return dlsym(lib_handle, name.data());
}

int Module::callFuncPackedRaw(std::string name, void **args) {
  if(should_use_pochi_codegen(name)) {
    if(name.rfind("_shim_", 0) == 0) {
      name = name.substr(std::string("_shim_").size());
    }
    // std::cout << name << std::endl;
    // std::cout << fns.at(name).m_fnPtr << std::endl;
    return fns.at(name)(reinterpret_cast<void **>(args));
  }
  else {
    typedef int (*fnptr_t)(void **);
    static_assert(
        sizeof(void *) == sizeof(fnptr_t),
        "Unable to cast dlsym() returned void pointer to function pointer");
    void *v_func_ptr = getFuncPtr(name);
    fnptr_t func_ptr;
    *reinterpret_cast<void **>(&func_ptr) = v_func_ptr;

  #if USE_OPENMP
    omp_sched_t existingSched;
    ParallelSchedule tacoSched;
    int existingChunkSize, tacoChunkSize;
    int existingNumThreads = omp_get_max_threads();
    omp_get_schedule(&existingSched, &existingChunkSize);
    taco_get_parallel_schedule(&tacoSched, &tacoChunkSize);
    switch (tacoSched) {
    case ParallelSchedule::Static:
      omp_set_schedule(omp_sched_static, tacoChunkSize);
      break;
    case ParallelSchedule::Dynamic:
      omp_set_schedule(omp_sched_dynamic, tacoChunkSize);
      break;
    default:
      break;
    }
    omp_set_num_threads(taco_get_num_threads());
  #endif
    int ret = func_ptr(args);

  #if USE_OPENMP
    omp_set_schedule(existingSched, existingChunkSize);
    omp_set_num_threads(existingNumThreads);
  #endif

    return ret;
  }
}

} // namespace ir
} // namespace taco

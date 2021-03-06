#include "pochivm/common.h"
#include "runtime_lib_builder/symbol_list_util.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/User.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/Transforms/Utils/Debugify.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Linker/Linker.h"

using namespace llvm;
using namespace llvm::orc;

// Remove the 'target-features' and 'target-cpu' attributes from the function.
// In LLVM, a function A may be inlined into a function B only if B's target-features
// is a superset of that of A. (This is because B may be calling into A only after checking
// the CPU features needed by A exists. If A were inlined into B, the logic of A may be moved
// around by optimizer to outside of the CPU feature check, resulting in a bug).
//
// The IR generated by clang++ has 'target-features' and 'target-cpu' attributes set,
// but our IR doesn't. So in order to allow inlining of C++ functions into our IR,
// we must erase the 'target-features' and 'target-cpu' attributes, which we do here.
//
// It is not the best or safest approach (a better approach would be setting the correct
// attributes for the functions we generated), but it is the easiest approach so we go with it for now.
//
static void RemoveTargetAttribute(Function* func)
{
    static const int numFeatures = 2;
    static const char* const featuresToRemove[numFeatures] = {
        "target-cpu",
        "target-features"
    };
    for (int i = 0; i < numFeatures; i++)
    {
        const char* feature = featuresToRemove[i];
        if (func->hasFnAttribute(feature))
        {
            func->removeFnAttr(feature);
        }
    }
}

static void RunLLVMOptimizePass(Module* module)
{
    static const llvm::PassBuilder::OptimizationLevel
            optLevel = llvm::PassBuilder::OptimizationLevel::O3;

    llvm::PassBuilder m_passBuilder;
    llvm::LoopAnalysisManager m_LAM;
    llvm::FunctionAnalysisManager m_FAM;
    llvm::CGSCCAnalysisManager m_CGAM;
    llvm::ModuleAnalysisManager m_MAM;
    llvm::ModulePassManager m_MPM;

    m_passBuilder.registerModuleAnalyses(m_MAM);
    m_passBuilder.registerCGSCCAnalyses(m_CGAM);
    m_passBuilder.registerFunctionAnalyses(m_FAM);
    m_passBuilder.registerLoopAnalyses(m_LAM);
    m_passBuilder.crossRegisterProxies(m_LAM, m_FAM, m_CGAM, m_MAM);

    m_MPM = m_passBuilder.buildPerModuleDefaultPipeline(optLevel);

    ReleaseAssert(module != nullptr);
    m_MPM.run(*module, m_MAM);
}

static void ExtractFunction(const std::string& generatedFileDir,
                            const std::string& filenameBase,
                            bool isSymbolPair,
                            std::string functionName,
                            const std::string& implFunctionName)
{
    ReleaseAssert(isSymbolPair == (implFunctionName != ""));

    // important to extract symbol from bc file with debug info stripped!
    //
    std::string bcFileName = filenameBase + ".stripped.bc";
    std::string uniqueSymbolHash = GetUniqueSymbolHash(functionName);

    // If the symbol starts with '*', it is a symbol that is called by a wrapper
    // We should generate the .bc file, but not the .h header file
    //
    bool shouldGenerateCppHeaderFile = true;
    if (functionName[0] == '*')
    {
        ReleaseAssert(!isSymbolPair);
        shouldGenerateCppHeaderFile = false;
        functionName = functionName.substr(1);
    }

    SMDiagnostic llvmErr;
    std::unique_ptr<LLVMContext> context(new LLVMContext);
    std::unique_ptr<Module> module = parseIRFile(bcFileName, llvmErr, *context.get());

    if (module == nullptr)
    {
        fprintf(stderr, "[ERROR] An error occurred while parsing IR file '%s', detail:\n", bcFileName.c_str());
        llvmErr.print("update_symbol_matches", errs());
        abort();
    }

    // For each global variable, if the global variable is not a constant,
    // the definition (address storing the value of the global variable) resides in the host process!
    // We must make this global variable a declaration (instead of a definition),
    // so it correctly resolves to the address in host process.
    //
    // For constants, if the address is significant, we also must make it a declaration,
    // otherwise we would get different addresses inside generated code and inside host process.
    //
    auto mustDropDefintion = [](const GlobalVariable& gv)
    {
        bool mustDrop = false;
        if (!gv.isConstant())
        {
            // For variables, we must always drop definition, so the generated code and host process
            // resolve the variable to the same address.
            //
            mustDrop = true;
        }
        else
        {
            // For constant, if it is at least local_unnamed_addr (global_unnamed_addr is not required),
            // its address is not significant, so it's OK to keep the definition,
            // and it allows optimizer to potentially work better.
            // Otherwise, we must make it a declaration to resolve it to the correct address.
            //
            if (!gv.hasAtLeastLocalUnnamedAddr())
            {
                mustDrop = true;
            }
        }
        return mustDrop;
    };
    for (GlobalVariable& gv : module->globals())
    {
        // We can only drop definition for globals with non-local linkage.
        // If there is a global with local linkage that is both needed by the function and
        // must drop defintion, it is an irrecoverable error.
        // We will check for that after we eliminate all the unneeded code, and fail it is the case.
        //
        if (!gv.hasLocalLinkage())
        {
            if (mustDropDefintion(gv))
            {
                // Drop the definition to make this global variable a declaration.
                //
                gv.setLinkage(GlobalValue::ExternalLinkage);
                gv.setInitializer(nullptr);
                gv.setComdat(nullptr);
            }
        }
    }

    std::map< Constant*, std::set<GlobalValue*> > constExprUsageGraph;
    std::map< Operator*, std::set<GlobalValue*> > operatorUsageGraph;

    std::function<void(Value*, std::set<GlobalValue*>&)> computeDependencies =
            [&constExprUsageGraph, &operatorUsageGraph, &computeDependencies](
                    Value *value, std::set<GlobalValue*>& allUsers /*out*/)
    {
        if (isa<Instruction>(value))
        {
            Instruction* inst = dyn_cast<Instruction>(value);
            Function* func = inst->getParent()->getParent();
            allUsers.insert(func);
        }
        else if (isa<GlobalValue>(value))
        {
            GlobalValue* gv = dyn_cast<GlobalValue>(value);
            allUsers.insert(gv);
        }
        else if (isa<Constant>(value))
        {
            Constant* cst = dyn_cast<Constant>(value);
            if (!constExprUsageGraph.count(cst))
            {
                // memorized search is needed to avoid exponential time
                //
                std::set<GlobalValue*>& cstUsers = constExprUsageGraph[cst];
                for (User* cstUser : cst->users())
                {
                    computeDependencies(cstUser, cstUsers /*out*/);
                }
            }
            std::set<GlobalValue*>& users = constExprUsageGraph[cst];
            allUsers.insert(users.begin(), users.end());
        }
        else if (isa<Operator>(value))
        {
            Operator* op = dyn_cast<Operator>(value);
            if (!operatorUsageGraph.count(op))
            {
                // memorized search is needed to avoid exponential time
                //
                std::set<GlobalValue*>& opUsers = operatorUsageGraph[op];
                for (User* opUser : op->users())
                {
                    computeDependencies(opUser, opUsers /*out*/);
                }
            }
            std::set<GlobalValue*>& users = operatorUsageGraph[op];
            allUsers.insert(users.begin(), users.end());
        }
        else
        {
            ReleaseAssert(false && "unhandled type");
        }
    };

    std::map< GlobalValue*, std::set<GlobalValue*> > usageGraph;

    std::function<void(GlobalValue*)> addUserEdgesOfGlobalValue =
            [&computeDependencies, &usageGraph](GlobalValue* gv)
    {
        std::set<GlobalValue*> allUsers;
        for (User *user : gv->users())
        {
            computeDependencies(user, allUsers);
        }
        for (GlobalValue* user : allUsers)
        {
            usageGraph[user].insert(gv);
        }
    };

    for (GlobalValue& gv : module->global_values())
    {
        addUserEdgesOfGlobalValue(&gv);
    }

    Function* functionTarget = module->getFunction(functionName);
    if (functionTarget == nullptr)
    {
        fprintf(stderr, "[ERROR] Module '%s' does not contain function '%s'.\n",
                bcFileName.c_str(), functionName.c_str());
        abort();
    }
    // We have already excluded declarations in the symbol list, no reason it can be empty here
    //
    ReleaseAssert(!functionTarget->empty());

    // Currently we don't have a 'always inline' option, the inlining option is just a hint.
    // So lockdown functions with internal linkage: otherwise if LLVM choose not to inline it
    // the generated code will not be able to call the function in host process address space.
    //
    if (functionTarget->getLinkage() == GlobalValue::LinkageTypes::PrivateLinkage ||
        functionTarget->getLinkage() == GlobalValue::LinkageTypes::InternalLinkage)
    {
        // This should not really happen here? We have excluded internal linkage symbols in symbol list
        // but anyway.. no adverse effect to check it here again
        //
        fprintf(stderr, "[ERROR] Function '%s' in module '%s' has internal linkage type. "
                        "To include it in the runtime library, you must change its linkage "
                        "type to external (by removing the 'static' keyword etc).\n",
                functionTarget->getName().str().c_str(),
                bcFileName.c_str());
        abort();
    }

    std::set<GlobalValue*> isNeeded;
    std::set<Function*> callees;
    std::function<void(GlobalValue*, bool)> dfs =
            [&dfs, &isNeeded, &callees, &usageGraph](GlobalValue* cur, bool isRoot)
    {
        if (isNeeded.count(cur))
        {
            return;
        }
        isNeeded.insert(cur);
        if (isa<Function>(cur) && !isRoot)
        {
            // Stop at function. The function will be turned into an extern.
            //
            Function* callee = dyn_cast<Function>(cur);
            callees.insert(callee);
            return;
        }
        if (usageGraph.count(cur))
        {
            for (GlobalValue* dependent: usageGraph[cur])
            {
                dfs(dependent, false /*isRoot*/);
            }
        }
    };
    dfs(functionTarget, true /*isRoot*/);

    for (Function* callee : callees)
    {
        // If the callee has non-public linkage, the runtime library will not be
        // able to find the definition at runtime! Lock it down now and ask the
        // user to remove the 'static' keyword.
        // We cannot fix the problem by somehow changing the linkage type by ourselves,
        // as that could cause name collisions.
        //
        if (callee->getLinkage() == GlobalValue::LinkageTypes::PrivateLinkage ||
            callee->getLinkage() == GlobalValue::LinkageTypes::InternalLinkage)
        {
            fprintf(stderr, "[ERROR] Function '%s' in module '%s' called function '%s', which "
                            "has local linkage type. To include the function in runtime libary, "
                            "you have to either (1) make callee '%s' have external linkage type"
                            "(by removing the 'static' keyword etc) "
                            "or (2) make caller '%s' non-inlinable by generated code by specifying "
                            "the option in pochivm_register_runtime.cpp (not the C++ declaration).\n",
                    functionTarget->getName().str().c_str(),
                    bcFileName.c_str(),
                    callee->getName().str().c_str(),
                    callee->getName().str().c_str(),
                    functionTarget->getName().str().c_str());
            abort();
        }
    }

    // Record all the global value identifiers for now,
    // the module and context will be modified in the next step
    // We later use these information to verify that the generated module matches our expectation
    // (that it keeps and only keeps the globals we expected).
    //
    std::vector<std::string> allGlobalValueIds;
    std::set<std::string> neededGlobalValueIds;
    for (GlobalValue& gv : module->global_values())
    {
        allGlobalValueIds.push_back(gv.getGlobalIdentifier());
    }
    for (GlobalValue* gv : isNeeded)
    {
        std::string s = gv->getGlobalIdentifier();
        ReleaseAssert(!neededGlobalValueIds.count(s));
        neededGlobalValueIds.insert(s);
    }

    // The logic below basically follows llvm_extract.cpp.
    // We first do a ExtractGV pass, which does not actually remove stuff here,
    // but only delete the body and make them declarations instead of definitions.
    // Then we call dead code elimination pass to actually delete declarations (which
    // contains some tricky logic that we probably should not try to write by ourselves).
    //
    // Below is basically the ExtractGV pass logic, with modifications to fit our purpose.
    //

    // Copied directly from ExtractGV.cpp, no idea if it fits us completely...
    // "Visit the global inline asm." <-- what is that?
    //
    module->setModuleInlineAsm("");

    // Delete bodies of unneeded global vars
    //
    for (GlobalVariable& gv : module->globals())
    {
        if (!isNeeded.count(&gv))
        {
            // a deleted symbol becomes an external declaration,
            // and since it is unused, it will be dropped by dead code elimination
            //
            bool isLocalLinkage = gv.hasLocalLinkage();
            gv.setLinkage(GlobalValue::ExternalLinkage);
            if (isLocalLinkage)
            {
                gv.setVisibility(GlobalValue::HiddenVisibility);
            }

            gv.setInitializer(nullptr);
            gv.setComdat(nullptr);
        }
    }

    for (Function& fn : module->functions())
    {
        // We should not need to keep any function body other than our target
        //
        if (&fn != functionTarget)
        {
            ReleaseAssert(!isNeeded.count(&fn) || callees.count(&fn));
            fn.deleteBody();
            fn.setComdat(nullptr);
        }
        else
        {
            ReleaseAssert(isNeeded.count(&fn));
            fn.setLinkage(GlobalValue::LinkageTypes::ExternalLinkage);
        }
    }

    // Copied directly from ExtractGV.cpp, no idea if it fits us completely...
    //
    for (Module::alias_iterator I = module->alias_begin(), E = module->alias_end(); I != E;)
    {
        Module::alias_iterator CurI = I;
        ++I;

        if (!isNeeded.count(&*CurI))
        {
            Type *Ty =  CurI->getValueType();

            CurI->removeFromParent();
            Value* Declaration;
            if (FunctionType *FTy = dyn_cast<FunctionType>(Ty)) {
                Declaration = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                               CurI->getAddressSpace(),
                                               CurI->getName(), module.get());

            } else {
                Declaration =
                        new GlobalVariable(*module.get(), Ty, false, GlobalValue::ExternalLinkage,
                                           nullptr, CurI->getName());

            }
            CurI->replaceAllUsesWith(Declaration);
            delete &*CurI;
        }
    }

    {
        // Copied from llvm_extract.cpp
        // Delete dead declarations
        // TODO: switch to new pass manager?
        //
        legacy::PassManager Passes;

        // Delete unreachable globals
        //
        Passes.add(createGlobalDCEPass());

        if (OPTION_KEEP_DEBUG_INFO)
        {
            // Only remove dead debug info
            // When KEEP_DEBUG_INFO is false, all debug info has been removed
            // at the time we optimize the IR file.
            //
            Passes.add(createStripDeadDebugInfoPass());
        }

        // Remove dead func decls
        //
        Passes.add(createStripDeadPrototypesPass());

        Passes.run(*module.get());
    }

    // Drop the target attributes from all functions
    //
    for (Function& fn : module->functions())
    {
        RemoveTargetAttribute(&fn);
    }

    // Change the linkage type to External
    //
    functionTarget = module->getFunction(functionName);
    ReleaseAssert(functionTarget != nullptr);
    ReleaseAssert(!functionTarget->empty());
    functionTarget->setLinkage(GlobalValue::LinkageTypes::ExternalLinkage);
    // We will change the linkage type of our target function to 'available externally'
    // after the bitfile is linked into the module.
    // AvailableExternallyLinkage is not allowed to have comdat, so drop the combat now.
    // It should be fine that we simply drop the comdat, since C++ always follows ODR rule.
    //
    functionTarget->setComdat(nullptr);

    // Output extracted bitcode file
    //
    std::string outputFileName = std::string("extracted.") + uniqueSymbolHash + ".bc";
    {
        int fd = creat(outputFileName.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (fd == -1)
        {
            fprintf(stderr, "Failed to open file '%s' for write, errno = %d (%s)\n",
                    outputFileName.c_str(), errno, strerror(errno));
            abort();
        }
        raw_fd_ostream fdStream(fd, true /*shouldClose*/);
        WriteBitcodeToFile(*module.get(), fdStream);
        if (fdStream.has_error())
        {
            std::error_code ec = fdStream.error();
            fprintf(stderr, "Writing to file '%s' failed with errno = %d (%s)\n",
                    outputFileName.c_str(), ec.value(), ec.message().c_str());
            abort();
        }
        /* fd closed when fdStream is destructed here */
    }

    // Make sure that the generated IR file does not contain errors,
    // and that set of 'needed' symbols that we determined is exactly the set
    // of symbols kept by the dead code elimination pass
    //
    {
        // Load the extracted bitcode file, replace the current context and module
        //
        std::unique_ptr<LLVMContext> newContext(new LLVMContext);
        std::unique_ptr<Module> newModule = parseIRFile(outputFileName, llvmErr, *newContext.get());

        if (newModule == nullptr)
        {
            fprintf(stderr, "[INTERNAL ERROR] Generated IR file '%s' contains error. "
                            "Please report a bug. Detail:\n", outputFileName.c_str());
            llvmErr.print("update_symbol_matches", errs());
            abort();
        }
        module = std::move(newModule);
        context = std::move(newContext);
    }

    std::set<std::string> allGlobalIdsInNewModule;
    for (GlobalValue& gv : module->global_values())
    {
        std::string s = gv.getGlobalIdentifier();
        ReleaseAssert(!allGlobalIdsInNewModule.count(s));
        allGlobalIdsInNewModule.insert(s);
    }

    for (const std::string& gid : allGlobalValueIds)
    {
        bool expectExist = (neededGlobalValueIds.count(gid) > 0);
        bool exist = (allGlobalIdsInNewModule.count(gid) > 0);
        if (expectExist != exist)
        {
            fprintf(stderr, "[INTERNAL ERROR] We expected generated IR file '%s' to%s contain "
                            "global '%s', but in reality it does%s. Please report a bug.\n",
                    outputFileName.c_str(), (!expectExist ? " not" : ""), gid.c_str(), (exist ? "" : " not"));
            abort();
        }
    }

    // These two checks should not really fail.. but just be paranoid
    //
    for (const std::string& gid : neededGlobalValueIds)
    {
        if (!allGlobalIdsInNewModule.count(gid))
        {
            fprintf(stderr, "[INTERNAL ERROR] We expected generated IR file '%s' to contain "
                            "global '%s', but in reality it does not [check2]. Please report a bug.\n",
                    outputFileName.c_str(), gid.c_str());
            abort();
        }
    }

    for (const std::string& gid : allGlobalIdsInNewModule)
    {
        if (!neededGlobalValueIds.count(gid))
        {
            fprintf(stderr, "[INTERNAL ERROR] We expected generated IR file '%s' to not contain "
                            "global '%s', but in reality it does [check3]. Please report a bug.\n",
                    outputFileName.c_str(), gid.c_str());
            abort();
        }
    }

    auto SanityCheckGlobals = [&module, &functionName, &bcFileName, &mustDropDefintion]()
    {
        Function* target = module->getFunction(functionName);
        ReleaseAssert(target != nullptr);
        ReleaseAssert(!target->empty());

        // Assert that for each global variable that is not constant, it must have non-local linkage,
        // since it should be resolved to an address in the host process.
        //
        for (GlobalVariable& gv : module->globals())
        {
            if (mustDropDefintion(gv))
            {
                if (gv.hasLocalLinkage())
                {
                    fprintf(stderr, "[ERROR] Function '%s' in module '%s' referenced global "
                                    "variable '%s', which has local linkage type. To include the function in "
                                    "runtime libary, you have to make global variable '%s' have external linkage type "
                                    "(by removing the 'static' keyword etc). If it is a static variable inside a "
                                    "function, try to move the function to a header file and add 'inline', which will "
                                    "give the static variable linkonce_odr linkage.\n",
                            functionName.c_str(),
                            bcFileName.c_str(),
                            gv.getGlobalIdentifier().c_str(),
                            gv.getGlobalIdentifier().c_str());
                    abort();
                }
                // We have dropped the definition earlier. Just sanity check again.
                //
                ReleaseAssert(gv.hasExternalLinkage());
                ReleaseAssert(!gv.hasInitializer());
                ReleaseAssert(!gv.hasComdat());
            }
        }

        // Assert that all functions, except our target, has become external declarations.
        //
        for (Function& fn : module->functions())
        {
            if (&fn != target)
            {
                if (x_isDebugBuild)
                {
                    // In debug build, if the function needs wrapper (is a symbol pair),
                    // the implementation is left there with available externally linkage.
                    // This should not happen in release build because we will run the optimization
                    // pass to either inline it or drop it.
                    //
                    ReleaseAssert(fn.empty() || fn.hasAvailableExternallyLinkage());
                }
                else
                {
                    ReleaseAssert(fn.empty() && fn.hasExternalLinkage());
                }
            }
            else
            {
                ReleaseAssert(!fn.empty());
                ReleaseAssert(fn.hasExternalLinkage());
            }
        }
    };

    SanityCheckGlobals();

    if (isSymbolPair)
    {
        // If it is a symbol pair, we should link in the implementation IR file,
        // if the implementation function is directly called, but not inlined by the wrapper function
        //
        Function* implFn = module->getFunction(implFunctionName);
        if (implFn != nullptr)
        {
            ReleaseAssert(implFn->empty());
            ReleaseAssert(implFn->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);

            std::string implFnBcFilename = std::string("extracted.") +
                                           GetUniqueSymbolHash(std::string("*") + implFunctionName) + ".bc";

            std::unique_ptr<Module> implModule = parseIRFile(implFnBcFilename, llvmErr, *context.get());

            if (implModule == nullptr)
            {
                fprintf(stderr, "[INTERNAL ERROR] Generated IR file '%s' not found or contains error. "
                                "Please report a bug. Detail:\n", implFnBcFilename.c_str());
                llvmErr.print("update_symbol_matches", errs());
                abort();
            }

            implFn = implModule->getFunction(implFunctionName);
            ReleaseAssert(implFn != nullptr);
            ReleaseAssert(!implFn->empty());
            ReleaseAssert(implFn->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);

            Linker linker(*module.get());
            // linkInModule returns true on error
            //
            ReleaseAssert(linker.linkInModule(std::move(implModule)) == false);

            implFn = module->getFunction(implFunctionName);
            ReleaseAssert(implFn != nullptr);
            ReleaseAssert(!implFn->empty());
            // change implFn to available_externally linkage before optimization pass,
            // so that it is either inlined by optimization pass, or dropped
            //
            ReleaseAssert(implFn->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);
            implFn->setLinkage(GlobalValue::LinkageTypes::AvailableExternallyLinkage);

            Function* wrapperFn = module->getFunction(functionName);
            ReleaseAssert(wrapperFn != nullptr);
            ReleaseAssert(wrapperFn->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);
            ReleaseAssert(!wrapperFn->empty());

            // Only run optimization passes in non-debug build
            //
            if (!x_isDebugBuild)
            {
                RunLLVMOptimizePass(module.get());
                // The optimization passes should have either dropped the implementation of wrappedFn
                // (so the symbol is external without body) or have inlined it (so the symbol no longer exists)
                //
                implFn = module->getFunction(implFunctionName);
                if (implFn != nullptr)
                {
                    ReleaseAssert(implFn->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage);
                    ReleaseAssert(implFn->empty());
                }
            }
        }

        // The output should be the implFunctionName.bc, without leading '*'
        //
        // Write output file, and parse it again for sanity (errors in IR would
        // not show up when the IR is saved, but only when the IR is parsed)
        //
        uniqueSymbolHash = GetUniqueSymbolHash(implFunctionName);
        outputFileName = std::string("extracted.") + uniqueSymbolHash + ".bc";
        {
            int fd = creat(outputFileName.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (fd == -1)
            {
                fprintf(stderr, "Failed to open file '%s' for write, errno = %d (%s)\n",
                        outputFileName.c_str(), errno, strerror(errno));
                abort();
            }
            raw_fd_ostream fdStream(fd, true /*shouldClose*/);
            WriteBitcodeToFile(*module.get(), fdStream);
            if (fdStream.has_error())
            {
                std::error_code ec = fdStream.error();
                fprintf(stderr, "Writing to file '%s' failed with errno = %d (%s)\n",
                        outputFileName.c_str(), ec.value(), ec.message().c_str());
                abort();
            }
            /* fd closed when fdStream is destructed here */
        }

        {
            std::unique_ptr<LLVMContext> newContext(new LLVMContext);
            std::unique_ptr<Module> newModule = parseIRFile(outputFileName, llvmErr, *newContext.get());

            if (newModule == nullptr)
            {
                fprintf(stderr, "[INTERNAL ERROR] Generated IR file '%s' contains error. "
                                "Please report a bug. Detail:\n", outputFileName.c_str());
                llvmErr.print("update_symbol_matches", errs());
                abort();
            }
            module = std::move(newModule);
            context = std::move(newContext);
        }

        SanityCheckGlobals();
    }

    // Generate data file in header file format
    // build_runtime_lib will generate the CPP file that include these headers
    //
    if (shouldGenerateCppHeaderFile)
    {
        size_t bitcodeSize;
        uint8_t* bitcodeRawData = nullptr;
        {
            struct stat st;
            if (stat(outputFileName.c_str(), &st) != 0)
            {
                fprintf(stderr, "Failed to access file '%s' for file size, errno = %d (%s)\n",
                        outputFileName.c_str(), errno, strerror(errno));
                abort();
            }
            bitcodeSize = static_cast<size_t>(st.st_size);

            // one more char for the trailing '\0'
            //
            bitcodeRawData = new uint8_t[bitcodeSize + 1];

            FILE* fp = fopen(outputFileName.c_str(), "r");
            if (fp == nullptr)
            {
                fprintf(stderr, "Failed to open file '%s' for read, errno = %d (%s)\n",
                        outputFileName.c_str(), errno, strerror(errno));
                abort();
            }

            size_t bytesRead = fread(bitcodeRawData, sizeof(uint8_t), bitcodeSize, fp);
            ReleaseAssert(bytesRead == bitcodeSize);
            bitcodeRawData[bitcodeSize] = 0;

            {
                // just to sanity check we have reached eof
                //
                uint8_t _c;
                ReleaseAssert(fread(&_c, sizeof(uint8_t), 1 /*numElements*/, fp) == 0);
                ReleaseAssert(feof(fp));
            }

            fclose(fp);
        }

        ReleaseAssert(bitcodeRawData != nullptr);
        Auto(delete [] bitcodeRawData);

        {
            std::string headerFileOutput = generatedFileDir + "/bc." + uniqueSymbolHash + ".data.h";
            FILE* fp = fopen(headerFileOutput.c_str(), "w");
            if (fp == nullptr)
            {
                fprintf(stderr, "Failed to open file '%s' for write, errno = %d (%s)\n",
                        headerFileOutput.c_str(), errno, strerror(errno));
                abort();
            }

            fprintf(fp, "// GENERATED FILE, DO NOT EDIT!\n");
            fprintf(fp, "// INTERNAL FILE, DO NOT INCLUDE TO YOUR CPP!\n");
            fprintf(fp, "//\n\n");

            fprintf(fp, "#ifndef INSIDE_POCHIVM_RUNTIME_LIBRARY_BC_CPP_FILE_MACRO_GUARD\n");
            fprintf(fp, "static_assert(false, \"INTERNAL FILE, DO NOT INCLUDE TO YOUR CPP!\");\n");
            fprintf(fp, "#endif\n\n");

            fprintf(fp, "#include \"pochivm/bitcode_data.h\"\n\n");

            fprintf(fp, "namespace PochiVM {\n\n");

            std::string resVarname = std::string("__pochivm_internal_bc_") + uniqueSymbolHash;

            // 'extern' declaration is required for constants to be able to export elsewhere
            //
            fprintf(fp, "extern const BitcodeData %s;\n\n", resVarname.c_str());

            std::string bitcodeDataVarname = resVarname + "_bitcode_data";
            fprintf(fp, "const uint8_t %s[%d] = {\n    ", bitcodeDataVarname.c_str(), static_cast<int>(bitcodeSize + 1));

            for (size_t i = 0; i <= bitcodeSize; i++)
            {
                uint8_t value = bitcodeRawData[i];
                fprintf(fp, "%d", value);
                if (i != bitcodeSize)
                {
                    fprintf(fp, ", ");
                    if (i % 16 == 15)
                    {
                        fprintf(fp, "\n    ");
                    }
                }
            }
            fprintf(fp, "\n};\n\n");

            std::string symbolVarname = resVarname + "_symbol";
            fprintf(fp, "const char* const %s = \"%s\";\n\n",
                    symbolVarname.c_str(), functionName.c_str());

            fprintf(fp, "const BitcodeData %s = {\n", resVarname.c_str());
            fprintf(fp, "    %s,\n", symbolVarname.c_str());
            fprintf(fp, "    %s,\n", bitcodeDataVarname.c_str());
            fprintf(fp, "    %d\n};\n", static_cast<int>(bitcodeSize));

            fprintf(fp, "\n}  // namespace PochiVM\n\n");

            fclose(fp);
        }
    }
}

int main(int argc, char** argv)
{
    InitLLVM _init_llvm_(argc, argv);

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // [all_needed_symbols] [filename] [generated_file_dir]
    //    Match filename.syms with all_needed_symbols, append results to filename.syms.matches,
    //    and generate the extracted bc file (extracted.[symbol hash].bc) for each symbol
    //

    ReleaseAssert(argc == 4 || argc == 5);
    std::string allNeededSymbolsFile = argv[1];
    std::string filenameBase = argv[2];
    std::string generatedFileDir = argv[3];
    std::string symList = filenameBase + ".syms";
    std::string symListMatches = filenameBase + ".syms.matches";

    bool shouldExtractWrapper = false;
    if (argc == 5)
    {
        ReleaseAssert(std::string(argv[4]) == "--extract-wrappers");
        shouldExtractWrapper = true;
    }
    std::set<std::string> allNeededSymbols = ReadSymbolListFileOrDie(allNeededSymbolsFile);
    std::set<std::string> allSymbols = ReadSymbolListFileOrDie(symList);
    std::set<std::string> allSymbolMatches = ReadSymbolListFileOrDie(symListMatches);

    // First pass: extract all symbols which does not require a wrapper
    //
    for (const std::string& symbolOrSymbolPair : allNeededSymbols)
    {
        size_t commaPos = symbolOrSymbolPair.find(",");
        if (commaPos == std::string::npos)
        {
            std::string symbol = symbolOrSymbolPair;
            std::string rawSymbol = symbol;
            if (rawSymbol[0] == '*') { rawSymbol = rawSymbol.substr(1); }
            if (allSymbols.count(rawSymbol))
            {
                // The caller should always have purged .syms.matches from the needed symbols
                //
                ReleaseAssert(!allSymbolMatches.count(symbol));
                allSymbolMatches.insert(symbol);
                ExtractFunction(generatedFileDir, filenameBase, false /*isSymbolPair*/, symbol, "");
            }
        }
    }

    // Second pass: extract wrappers
    // build_runtime_lib always call --extract-wrappers last, so at this time all wrapperImpl IR should be available
    //
    if (shouldExtractWrapper)
    {
        for (const std::string& symbolOrSymbolPair : allNeededSymbols)
        {
            size_t commaPos = symbolOrSymbolPair.find(",");
            if (commaPos != std::string::npos)
            {
                std::string symbolImpl = symbolOrSymbolPair.substr(commaPos + 1);
                std::string symbolWrapper = symbolOrSymbolPair.substr(0, commaPos);

                ReleaseAssert(allSymbols.count(symbolWrapper));
                std::string tmp = std::string("*") + symbolImpl;
                if (allNeededSymbols.count(tmp))
                {
                    if (!allSymbolMatches.count(tmp))
                    {
                        fprintf(stderr, "[ERROR] Symbol '%s' cannot be found in object files. (use 'c++filt -n [symbol]' to demangle)\n",
                                symbolImpl.c_str());
                        fprintf(stderr, "A few possible reasons:\n");
                        fprintf(stderr, "  (1) Its implementation is missing.\n");
                        fprintf(stderr, "  (2) The CPP file containing its implementation is not listed in runtime/CMakeLists.txt. "
                                        "Add the CPP file to CMakeLists.txt to fix the issue.\n");
                        fprintf(stderr, "  (3) The implementation has internal linkage type. Remove the 'static' keyword to fix the issue.\n");
                        abort();
                    }
                }
                ReleaseAssert(!allSymbolMatches.count(symbolOrSymbolPair));
                allSymbolMatches.insert(symbolOrSymbolPair);
                ExtractFunction(generatedFileDir, filenameBase, true /*isSymbolPair*/, symbolWrapper, symbolImpl);
            }
        }
    }

    std::string tmpFile = symListMatches + ".tmp";
    WriteSymbolListFileOrDie(tmpFile, allSymbolMatches);

    // Give all-or-nothing guarantee by rename
    //
    int r = rename(tmpFile.c_str(), symListMatches.c_str());
    ReleaseAssert(r == 0 || r == -1);
    if (r == -1)
    {
        fprintf(stderr, "Failed to rename file '%s' into '%s', errno = %d (%s)\n",
                tmpFile.c_str(), symListMatches.c_str(), errno, strerror(errno));
        abort();
    }

    return 0;
}

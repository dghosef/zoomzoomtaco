import string


class Cppgen:
    def __init__(self):
        self.indent = 0

    def doindent(self):
        return "  " * self.indent

    def newline(self):
        return "\n" + self.doindent()

    def incindent(self):
        self.indent += 1

    def dedent(self):
        self.indent -= 1

    def call(self, fnname, params, templates=""):
        ret = ""
        ret += fnname
        if templates != "":
            ret += "<" + templates + ">"
        ret += "("
        ret += ", ".join(params)
        ret += ")"
        return ret

    def fn(self, rettype, name, params, body):
        ret = ""
        ret += rettype + " " + name + "("
        ret += ", ".join(params)
        ret += ") {"
        self.incindent()
        ret += self.newline()
        ret += body
        self.dedent()
        ret += self.newline()
        ret += "}"
        return ret

    def fndecl(self, rettype, name, params):
        ret = self.fn(rettype, name, params, "")
        return ret[:ret.find("{")]

    def lmbda(self, rettype, params, body):
        ret = ""
        ret += "[]("
        ret += ", ".join(params)
        ret += ") -> " + rettype + " {"
        self.incindent()
        ret += self.newline()
        ret += body
        self.dedent()
        ret += self.newline()
        ret += "}"
        return ret

    def hashmap(self, keytype, valuetype, name, keys, values):
        ret = ""
        ret += "std::map<" + keytype + ", " + valuetype + "> " + name + " = {"
        self.incindent()
        ret += self.newline()
        map_elems = []
        for key, value in zip(keys, values):
            map_elems.append(self.call("std::make_pair", [key, value]))
        ret += ("," + self.newline()).join(map_elems)
        self.dedent()
        ret += self.newline()
        ret += "};"
        return ret

    def retstmt(self, retval):
        return "return " + retval + ";"

    def block(self, stmts):
        self.incindent()
        ret = ""
        for stmt in stmts:
            ret += stmt + ";"
            ret += self.newline()
        self.dedent()
        return ret


# We need to
# Make a map from string -> lambda(vector<variableVT>->variableVT) that calls the fn
# make callers for each of the functions for the pochivm gen code thingy
# Each function in the map calls


"""
---------------------------------------------------
Pochi Compilation time:
Register callers of the target functions we need(simple wrappers)
Declare the wrappers in the .h file
Make the wrappers themselves in the .cpp file
---------------------------------------------------
Inside the Call function:
Visit the args and turn them into a vector of ValueVTs
Index into the map and get the lambda. Call the lambda with the vector of ValueVTs
---------------------------------------------------
Map contents:
string -> lambda(vector<ValueVT> -> ValueVT)
Should take in a vector of ValueVT and index them to call the
generated function. Cast the args to the type(VT->int for ex).
and then return a ValueVT
"""
# This goes into the gen runtime code file


def emit_registers(fns):
    cpp = Cppgen()
    block = []
    for fn in fns:
        block.append(cpp.call("RegisterFreeFn", [], "&" + fn[1] + "_wrapper"))
    cpp.indent = 0
    return cpp.block(block)


def emit_wrapper(fn):
    cpp = Cppgen()
    letters = string.ascii_lowercase
    fnname = fn[1]
    params = [i[0] + " " + i[1] for i in zip(fn[2], letters)]
    param_names = [i[0] for i in zip(letters, params)]
    body = cpp.retstmt(cpp.call(fnname, param_names))
    return cpp.fn(fn[0], fnname + "_wrapper", params, body)


def emit_wrappers(fns):
    ret = ""
    for fn in fns:
        ret += emit_wrapper(fn) + "\n"
    return ret


def emit_wrapper_decl(fn):
    cpp = Cppgen()
    letters = string.ascii_lowercase
    fnname = fn[1]
    params = [i[0] + " " + i[1] for i in zip(fn[2], letters)]
    return cpp.fndecl(fn[0], fnname + "_wrapper", params) + ";"


def emit_wrapper_decls(fns):
    ret = ""
    for fn in fns:
        ret += emit_wrapper_decl(fn) + "\n"
    return ret


def makemap(fns):
    keys = [fn[1] for fn in fns]
    keys = ["\"" + key + "\"" for key in keys]
    values = []
    cpp = Cppgen()
    for fn in fns:
        call_params = ["PochiVM::StaticCast<" + i[0] +
                       ">(params[" + str(i[1]) + "])" for i in zip(fn[2], range(len(fn[2])))]
        body = cpp.call("PochiVM::CallFreeFn::" +
                        fn[1] + "_wrapper", call_params)
        body = cpp.retstmt(body)
        values.append(cpp.lmbda("PochiVM::ValueVT", [
                      "std::vector<PochiVM::ValueVT> &params"], body))
    return cpp.hashmap("std::string", "std::function<PochiVM::ValueVT(std::vector<PochiVM::ValueVT>&)>", "fns", keys, values)


def write_cpp(path, code, includes):
    with open(path, "w+") as f:
        for include in includes:
            f.write("#include " + include + "\n")
        f.write(code)
        f.write("\n")


def make_fn(name, type, param_count=1):
    return [type, name, [type] * param_count]


def make_double_and_float_fn(name, param_count=1):
    return [make_fn(name, "double", param_count), make_fn(name + "f", "float", param_count)]


if __name__ == "__main__":
    print("Generating cpp code")
    two_param_fns = ["fmod", "pow", "atan2"]
    one_param_fns = ["fabs", "sqrt", "exp", "log", "sin", "cos", "tan",
                     "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh"]
    fns = [["int", "taco_binarySearchAfter", ["int *", "int", "int", "int"]],
           ["int", "taco_binarySearchBefore", ["int *", "int", "int", "int"]],
           ["void *", "calloc", ["size_t", "size_t"]],
           make_fn("abs", "int"),
           make_fn("labs", "int64_t"),
           make_fn("log10", "double")]
    for fn in two_param_fns:
        fns += make_double_and_float_fn(fn, 2)
    for fn in one_param_fns:
        fns += make_double_and_float_fn(fn, 1)
    write_cpp("./src/codegen/callables.h", makemap(fns),
              ["\"pochivm.h\"", "<map>", "<string>", "<functional>"])
    write_cpp("./PochiVM/runtime/taco_wrapper_impls.h", emit_wrappers(fns), [])
    write_cpp("./PochiVM/runtime/taco_wrappers.h", emit_wrapper_decls(fns), [])
    write_cpp("./PochiVM/runtime/taco_register.h", emit_registers(fns), [])
    print("Done generating cpp")

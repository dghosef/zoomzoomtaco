#include "codegen/compile_to_pochi.h"
#ifndef DONT_RECOMPILE

PochiVM::Value<void> PochiCompiler::unpackTensorProperty(
    std::string varname, const taco::ir::GetProperty *op, bool is_output_prop) {

  PochiVM::Block ret = PochiVM::Block();
  PochiSum var = pochiVarMap.at(varname);
  auto tensor = op->tensor.as<Var>();
  auto tensorvar = std::get<PochiVM::Variable<taco_tensor_t *>>(
      pochiVarMap.at(tensor->name));
  if (op->property == TensorProperty::Values) {
// for the values, it's in the last slot
// type varname = (type)tensor->name->vals
#define stmt(kind, type)                                                       \
  {                                                                            \
    auto casted_var = std::get<PochiVM::Variable<type *>>(var);                \
    ret.Append(PochiVM::Assign(                                                \
        casted_var, PochiVM::ReinterpretCast<type *>(tensorvar->vals())));     \
    return ret;                                                                \
  }
    switch_over_Datatype(op->tensor.type().getKind())
#undef stmt
  } else if (op->property == TensorProperty::ValuesSize) {
    // int varname = tensor->name->vals_size
    auto casted_var = std::get<PochiVM::Variable<int>>(var);
    ret.Append(PochiVM::Assign(casted_var, tensorvar->vals_size()));
    return ret;
  }

  // for a Dense level, nnz is an int
  // for a Fixed level, ptr is an int
  // all others are int*
  if (op->property == TensorProperty::Dimension) {
    // int varname = (int)((tensor->name)->dimension[op->mode])
    auto casted_var = std::get<PochiVM::Variable<int>>(var);
    ret.Append(PochiVM::Assign(
        casted_var,
        PochiVM::StaticCast<int>(
            tensorvar->dimensions()[PochiVM::Literal<int>(op->mode)])));
    return ret;
  } else {
    // int *varname = (int *)((tensor->name)->indices[op->mode][nm])
    auto nm = op->index;
    taco_iassert(op->property == TensorProperty::Indices);
    auto casted_var = std::get<PochiVM::Variable<int *>>(var);
    ret.Append(PochiVM::Assign(
        casted_var, PochiVM::ReinterpretCast<int *>(
                        tensorvar->indices()[PochiVM::Literal<int>(op->mode)]
                                            [PochiVM::Literal<int>(nm)])));
    return ret;
  }
}
PochiVM::Block PochiCompiler::emitDecls(
    std::map<taco::ir::Expr, std::string, taco::ir::ExprCompare> varMap,
    std::vector<taco::ir::Expr> inputs, std::vector<taco::ir::Expr> outputs) {
  unordered_set<string> propsAlreadyGenerated;

  vector<const GetProperty *> sortedProps;

  for (auto const &p : varMap) {
    if (p.first.as<GetProperty>())
      sortedProps.push_back(p.first.as<GetProperty>());
  }

  // sort the properties in order to generate them in a canonical order
  sort(sortedProps.begin(), sortedProps.end(),
       [&](const GetProperty *a, const GetProperty *b) -> bool {
         // first, use a total order of outputs,inputs
         auto a_it = find(outputs.begin(), outputs.end(), a->tensor);
         auto b_it = find(outputs.begin(), outputs.end(), b->tensor);
         auto a_pos = distance(outputs.begin(), a_it);
         auto b_pos = distance(outputs.begin(), b_it);
         if (a_it == outputs.end())
           a_pos += distance(inputs.begin(),
                             find(inputs.begin(), inputs.end(), a->tensor));
         if (b_it == outputs.end())
           b_pos += distance(inputs.begin(),
                             find(inputs.begin(), inputs.end(), b->tensor));

         // if total order is same, have to do more, otherwise we know
         // our answer
         if (a_pos != b_pos)
           return a_pos < b_pos;

         // if they're different properties, sort by property
         if (a->property != b->property)
           return a->property < b->property;

         // now either the mode gives order, or index #
         if (a->mode != b->mode)
           return a->mode < b->mode;

         return a->index < b->index;
       });

  PochiVM::Block ret;
  int curr_pos = 0;
  for (auto v : inputs) {
    auto var = v.as<Var>();
    auto pochi_var =
        std::get<PochiVM::Variable<taco_tensor_t *>>(pochiVarMap.at(var->name));
    ret.Append(
        PochiVM::Assign(pochi_var, params[PochiVM::Literal<int>(curr_pos++)]));
  }
  for (auto v : outputs) {
    auto var = v.as<Var>();
    auto pochi_var =
        std::get<PochiVM::Variable<taco_tensor_t *>>(pochiVarMap.at(var->name));
    ret.Append(
        PochiVM::Assign(pochi_var, params[PochiVM::Literal<int>(curr_pos++)]));
  }

  for (auto prop : sortedProps) {
    bool isOutputProp =
        (find(outputs.begin(), outputs.end(), prop->tensor) != outputs.end());

    auto var = prop->tensor.as<Var>();
    if (var->is_parameter) {
    } else {
      ret.Append(unpackTensorProperty(varMap[prop], prop, isOutputProp));
    }
    propsAlreadyGenerated.insert(varMap[prop]);
  }

  return ret;
}
PochiVM::Value<void> PochiCompiler::pointTensorProperty(std::string varname) {
  return std::visit(
      [&](auto &&var) -> PochiVM::Value<void> {
        using OuterType = std::decay_t<decltype(var)>;
        using InnerType = inner_type_t<OuterType>;
        if constexpr (is_pochi_var_v<OuterType> &&
                      !std::is_pointer_v<InnerType>) {
          return PochiVM::Assign(*std::get<PochiVM::Variable<InnerType *>>(
                                     pochiVarMap.at("*" + varname + "_ptr")),
                                 var);
        } else {
          { taco_iassert(false) << "unexpected type"; }
          __builtin_unreachable();
        }
      },
      pochiVarMap.at(varname));
}
PochiVM::Value<void>
PochiCompiler::packTensorProperty(std::string varname, taco::ir::Expr tnsr,
                                  taco::ir::TensorProperty property, int mode,
                                  int index) {

  auto tensor = tnsr.as<Var>();
  if (property == TensorProperty::Values) {
    // (tensor->name)->vals = (uint8_t*)varname
    return std::visit(
        [&](auto &&var) -> PochiVM::Value<void> {
          if constexpr (is_pochi_var_v<std::decay_t<decltype(var)>> &&
                        !std::is_same_v<PochiVM::Variable<void>,
                                        decltype(var)>) {
            return PochiVM::Assign(std::get<PochiVM::Variable<taco_tensor_t *>>(
                                       pochiVarMap.at(tensor->name))
                                       ->vals(),
                                   PochiVM::ReinterpretCast<uint8_t *>(var));
          } else {
            { taco_iassert(false) << "unexpected type"; }
            __builtin_unreachable();
          }
        },
        pochiVarMap.at(varname));
  } else if (property == TensorProperty::ValuesSize) {
    // (tensor->name)->vals->vals_size = varname
    return PochiVM::Assign(
        std::get<PochiVM::Variable<taco_tensor_t *>>(
            pochiVarMap.at(tensor->name))
            ->vals_size(),
        std::get<PochiVM::Variable<int>>(pochiVarMap.at(varname)));
  }

  string tp;

  // for a Dense level, nnz is an int
  // for a Fixed level, ptr is an int
  // all others are int*
  if (property == TensorProperty::Dimension) {
    return PochiVM::Block();
  } else {
    taco_iassert(property == TensorProperty::Indices);
    // (tensor->name)->indices[mode][nm] = (uint8_t*)varname
    auto nm = index;
    return std::visit(
        [&](auto &&var) -> PochiVM::Value<void> {
          if constexpr (is_pochi_var_v<std::decay_t<decltype(var)>> &&
                        !std::is_same_v<PochiVM::Variable<void>,
                                        decltype(var)>) {
            return PochiVM::Assign(std::get<PochiVM::Variable<taco_tensor_t *>>(
                                       pochiVarMap.at(tensor->name))
                                       ->indices()[PochiVM::Literal<int>(mode)]
                                                  [PochiVM::Literal<int>(nm)],
                                   PochiVM::ReinterpretCast<uint8_t *>(var));
          } else {
            { taco_iassert(false) << "unexpected type"; }
            __builtin_unreachable();
          }
        },
        pochiVarMap.at(varname));
  }
}
PochiVM::Value<void> PochiCompiler::emitPack(
    std::map<std::tuple<taco::ir::Expr, taco::ir::TensorProperty, int, int>,
             std::string>
        outputProperties,
    std::vector<taco::ir::Expr> outputs) {

  PochiVM::Block ret = PochiVM::Block();
  vector<tuple<Expr, TensorProperty, int, int>> sortedProps;

  for (auto &prop : outputProperties) {
    sortedProps.push_back(prop.first);
  }
  sort(sortedProps.begin(), sortedProps.end(),
       [&](const tuple<Expr, TensorProperty, int, int> &a,
           const tuple<Expr, TensorProperty, int, int> &b) -> bool {
         // first, use a total order of outputs,inputs
         auto a_it = find(outputs.begin(), outputs.end(), get<0>(a));
         auto b_it = find(outputs.begin(), outputs.end(), get<0>(b));
         auto a_pos = distance(outputs.begin(), a_it);
         auto b_pos = distance(outputs.begin(), b_it);

         // if total order is same, have to do more, otherwise we know
         // our answer
         if (a_pos != b_pos)
           return a_pos < b_pos;

         // if they're different properties, sort by property
         if (get<1>(a) != get<1>(b))
           return get<1>(a) < get<1>(b);

         // now either the mode gives order, or index #
         if (get<2>(a) != get<2>(b))
           return get<2>(a) < get<2>(b);

         return get<3>(a) < get<3>(b);
       });

  for (auto prop : sortedProps) {
    auto var = get<0>(prop).as<Var>();
    if (var->is_parameter) {
      ret.Append(pointTensorProperty(outputProperties[prop]));
    } else {
      ret.Append(packTensorProperty(outputProperties[prop], get<0>(prop),
                                    get<1>(prop), get<2>(prop), get<3>(prop)));
    }
  }
  return ret;
}
// codegen_c.cpp
PochiCompiler::PochiSum
PochiCompiler::visit_pochi(const taco::ir::Function *func) {
  PochiVM::Block body = PochiVM::Block();
  int numYields = countYields(func);
  emittingCoroutine = (numYields > 0);
  funcName = func->name;
  labelCount = 0;

  resetUniqueNameCounters();
  FindVars inputVarFinder(func->inputs, {}, this, &pochiVarMap, body);
  func->body.accept(&inputVarFinder);
  FindVars outputVarFinder({}, func->outputs, this, &pochiVarMap, body);
  func->body.accept(&outputVarFinder);

  // Get each of the input/output param variables to point to the correct part
  // of the param array

  // find all the vars that are not inputs or outputs and declare them
  resetUniqueNameCounters();
  FindVars varFinder(func->inputs, func->outputs, this, &pochiVarMap, body);
  func->body.accept(&varFinder);
  varMap = varFinder.varMap;
  localVars = varFinder.localVars;

  // Declare varibles in function body

  body.Append(emitDecls(varFinder.varMap, func->inputs, func->outputs));
  body.Append(std::get<PochiVM::Value<void>>(visit_taco_op(func->body)));
  if (checkForAlloc(func))
    body.Append(emitPack(varFinder.outputProperties, func->outputs));
  fn.SetBody(body);
  body.Append(PochiVM::Return(PochiVM::Literal<int>(0)));
  return body;
}
#endif
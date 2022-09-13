#pragma once

#define switch_over_Datatype(expr)                      \
  switch (expr) {                                       \
    case Datatype::Kind::Bool:                          \
      stmt(Datatype::Kind::Bool, bool);                 \
      break;                                            \
    case Datatype::Kind::UInt8:                         \
      stmt(Datatype::Kind::UInt8, uint8_t);             \
      break;                                            \
    case Datatype::Kind::UInt16:                        \
      stmt(Datatype::Kind::UInt16, uint16_t);           \
      break;                                            \
    case Datatype::Kind::UInt32:                        \
      stmt(Datatype::Kind::UInt32, uint32_t);           \
      break;                                            \
    case Datatype::Kind::UInt64:                        \
      stmt(Datatype::Kind::UInt64, uint64_t);           \
      break;                                            \
    case Datatype::Kind::UInt128:                       \
      taco_not_supported_yet;                           \
      stmt(Datatype::Kind::UInt64, uint64_t);           \
      break;                                            \
    case Datatype::Kind::Int8:                          \
      stmt(Datatype::Kind::Int8, int8_t);               \
      break;                                            \
    case Datatype::Kind::Int16:                         \
      stmt(Datatype::Kind::Int16, int16_t);             \
      break;                                            \
    case Datatype::Kind::Int32:                         \
      stmt(Datatype::Kind::Int32, int32_t);             \
      break;                                            \
    case Datatype::Kind::Int64:                         \
      stmt(Datatype::Kind::Int64, int64_t);             \
      break;                                            \
    case Datatype::Kind::Int128:                        \
      taco_not_supported_yet;                           \
      stmt(Datatype::Kind::UInt64, uint64_t);           \
      break;                                            \
    case Datatype::Kind::Float32:                       \
      stmt(Datatype::Kind::Float32, float);             \
      break;                                            \
    case Datatype::Kind::Float64:                       \
      stmt(Datatype::Kind::Float64, double);            \
      break;                                            \
    case Datatype::Kind::Complex64:                     \
      taco_uerror << "Complex not supported right now"; \
      stmt(Datatype::Kind::Float64, double);            \
      break;                                            \
    case Datatype::Kind::Complex128:                    \
      taco_uerror << "Complex not supported right now"; \
      stmt(Datatype::Kind::Float64, double);            \
      break;                                            \
    case Datatype::Kind::Undefined:                     \
      taco_not_supported_yet;                           \
      stmt(Datatype::Kind::Float64, double);            \
      break;                                            \
  }

// Dispatch `stmt` if `expr` has a numeric(non-bool) type
// If error_on_failure == true, throw if expr is not numeric
#define dispatch_numeric(error_on_failure) \
switch((expr).index()) {\
  case 1: \
    stmt(std::get<PochiVM::Value<uint8_t>>(expr), uint8_t, PochiVM::Value<uint8_t>); \
    break; \
  case 2: \
    stmt(std::get<PochiVM::Value<uint16_t>>(expr), uint16_t, PochiVM::Value<uint16_t>); \
    break; \
  case 3: \
    stmt(std::get<PochiVM::Value<uint32_t>>(expr), uint32_t, PochiVM::Value<uint32_t>); \
    break; \
  case 4: \
    stmt(std::get<PochiVM::Value<uint64_t>>(expr), uint64_t, PochiVM::Value<uint64_t>); \
    break; \
  case 5: \
    stmt(std::get<PochiVM::Value<int8_t>>(expr), int8_t, PochiVM::Value<int8_t>); \
    break; \
  case 6: \
    stmt(std::get<PochiVM::Value<int16_t>>(expr), int16_t, PochiVM::Value<int16_t>); \
    break; \
  case 7: \
    stmt(std::get<PochiVM::Value<int32_t>>(expr), int32_t, PochiVM::Value<int32_t>); \
    break; \
  case 8: \
    stmt(std::get<PochiVM::Value<int64_t>>(expr), int64_t, PochiVM::Value<int64_t>); \
    break; \
  case 9: \
    stmt(std::get<PochiVM::Value<float>>(expr), float, PochiVM::Value<float>); \
    break; \
  case 10: \
    stmt(std::get<PochiVM::Value<double>>(expr), double, PochiVM::Value<double>); \
    break; \
\
  case 24: \
    stmt(std::get<PochiVM::Variable<uint8_t>>(expr), uint8_t, PochiVM::Variable<uint8_t>); \
    break; \
  case 25: \
    stmt(std::get<PochiVM::Variable<uint16_t>>(expr), uint16_t, PochiVM::Variable<uint16_t>); \
    break; \
  case 26: \
    stmt(std::get<PochiVM::Variable<uint32_t>>(expr), uint32_t, PochiVM::Variable<uint32_t>); \
    break; \
  case 27: \
    stmt(std::get<PochiVM::Variable<uint64_t>>(expr), uint64_t, PochiVM::Variable<uint64_t>); \
    break; \
  case 28: \
    stmt(std::get<PochiVM::Variable<int8_t>>(expr), int8_t, PochiVM::Variable<int8_t>); \
    break; \
  case 29: \
    stmt(std::get<PochiVM::Variable<int16_t>>(expr), int16_t, PochiVM::Variable<int16_t>); \
    break; \
  case 30: \
    stmt(std::get<PochiVM::Variable<int32_t>>(expr), int32_t, PochiVM::Variable<int32_t>); \
    break; \
  case 31: \
    stmt(std::get<PochiVM::Variable<int64_t>>(expr), int64_t, PochiVM::Variable<int64_t>); \
    break; \
  case 32: \
    stmt(std::get<PochiVM::Variable<float>>(expr), float, PochiVM::Variable<float>); \
    break; \
  case 33: \
    stmt(std::get<PochiVM::Variable<double>>(expr), double, PochiVM::Variable<double>); \
    break; \
  default: \
    if(error_on_failure) taco_uerror << "Expected a number"; \
}


// Dispatch `stmt` if `expr` has a numeric, signed  type
// If error_on_failure == true, throw if expr is not numeric
#define dispatch_bool(error_on_failure) \
switch((expr).index()) {\
  case 0: \
    stmt(std::get<PochiVM::Value<bool>>(expr), bool, PochiVM::Value<bool>); \
    break; \
  case 23: \
    stmt(std::get<PochiVM::Variable<bool>>(expr), bool, PochiVM::Value<bool>); \
    break; \
  default: \
    if(error_on_failure) taco_uerror << "Expected a bool"; \
}


// Dispatch `stmt` if `expr` has a numeric, signed  type
// If error_on_failure == true, throw if expr is not numeric
#define dispatch_signed_numeric(error_on_failure) \
switch((expr).index()) {\
  case 5: \
    stmt(std::get<PochiVM::Value<int8_t>>(expr), int8_t, PochiVM::Value<int8_t>); \
    break; \
  case 6: \
    stmt(std::get<PochiVM::Value<int16_t>>(expr), int16_t, PochiVM::Value<int16_t>); \
    break; \
  case 7: \
    stmt(std::get<PochiVM::Value<int32_t>>(expr), int32_t, PochiVM::Value<int32_t>); \
    break; \
  case 8: \
    stmt(std::get<PochiVM::Value<int64_t>>(expr), int64_t, PochiVM::Value<int64_t>); \
    break; \
  case 9: \
    stmt(std::get<PochiVM::Value<float>>(expr), float, PochiVM::Value<float>); \
    break; \
  case 10: \
    stmt(std::get<PochiVM::Value<double>>(expr), double, PochiVM::Value<double>); \
    break; \
\
  case 28: \
    stmt(std::get<PochiVM::Variable<int8_t>>(expr), int8_t, PochiVM::Variable<int8_t>); \
    break; \
  case 29: \
    stmt(std::get<PochiVM::Variable<int16_t>>(expr), int16_t, PochiVM::Variable<int16_t>); \
    break; \
  case 30: \
    stmt(std::get<PochiVM::Variable<int32_t>>(expr), int32_t, PochiVM::Variable<int32_t>); \
    break; \
  case 31: \
    stmt(std::get<PochiVM::Variable<int64_t>>(expr), int64_t, PochiVM::Variable<int64_t>); \
    break; \
  case 32: \
    stmt(std::get<PochiVM::Variable<float>>(expr), float, PochiVM::Variable<float>); \
    break; \
  case 33: \
    stmt(std::get<PochiVM::Variable<double>>(expr), double, PochiVM::Variable<double>); \
    break; \
  default: \
    if(error_on_failure) taco_uerror << "Expected a signed number"; \
}



// Dispatch `stmt` if `expr` has a numeric(non-bool) type
// If error_on_failure == true, throw if expr is not numeric
#define dispatch_integral(error_on_failure) \
switch((expr).index()) {\
  case 1: \
    stmt(std::get<PochiVM::Value<uint8_t>>(expr), uint8_t, PochiVM::Value<uint8_t>); \
    break; \
  case 2: \
    stmt(std::get<PochiVM::Value<uint16_t>>(expr), uint16_t, PochiVM::Value<uint16_t>); \
    break; \
  case 3: \
    stmt(std::get<PochiVM::Value<uint32_t>>(expr), uint32_t, PochiVM::Value<uint32_t>); \
    break; \
  case 4: \
    stmt(std::get<PochiVM::Value<uint64_t>>(expr), uint64_t, PochiVM::Value<uint64_t>); \
    break; \
  case 5: \
    stmt(std::get<PochiVM::Value<int8_t>>(expr), int8_t, PochiVM::Value<int8_t>); \
    break; \
  case 6: \
    stmt(std::get<PochiVM::Value<int16_t>>(expr), int16_t, PochiVM::Value<int16_t>); \
    break; \
  case 7: \
    stmt(std::get<PochiVM::Value<int32_t>>(expr), int32_t, PochiVM::Value<int32_t>); \
    break; \
  case 8: \
    stmt(std::get<PochiVM::Value<int64_t>>(expr), int64_t, PochiVM::Value<int64_t>); \
    break; \
  case 24: \
    stmt(std::get<PochiVM::Variable<uint8_t>>(expr), uint8_t, PochiVM::Variable<uint8_t>); \
    break; \
  case 25: \
    stmt(std::get<PochiVM::Variable<uint16_t>>(expr), uint16_t, PochiVM::Variable<uint16_t>); \
    break; \
  case 26: \
    stmt(std::get<PochiVM::Variable<uint32_t>>(expr), uint32_t, PochiVM::Variable<uint32_t>); \
    break; \
  case 27: \
    stmt(std::get<PochiVM::Variable<uint64_t>>(expr), uint64_t, PochiVM::Variable<uint64_t>); \
    break; \
  case 28: \
    stmt(std::get<PochiVM::Variable<int8_t>>(expr), int8_t, PochiVM::Variable<int8_t>); \
    break; \
  case 29: \
    stmt(std::get<PochiVM::Variable<int16_t>>(expr), int16_t, PochiVM::Variable<int16_t>); \
    break; \
  case 30: \
    stmt(std::get<PochiVM::Variable<int32_t>>(expr), int32_t, PochiVM::Variable<int32_t>); \
    break; \
  case 31: \
    stmt(std::get<PochiVM::Variable<int64_t>>(expr), int64_t, PochiVM::Variable<int64_t>); \
    break; \
  default: \
    if(error_on_failure) taco_uerror << "Expected a (u)int"; \
}

// Dispatch `stmt` if `expr` has a numeric(non-bool) type
// If error_on_failure == true, throw if expr is not numeric
#define dispatch_pointer(error_on_failure) \
switch((expr).index()) {\
  case 12: \
    stmt(std::get<PochiVM::Value<uint8_t*>>(expr), uint8_t*, PochiVM::Value<uint8_t*>); \
    break; \
  case 13: \
    stmt(std::get<PochiVM::Value<uint16_t*>>(expr), uint16_t*, PochiVM::Value<uint16_t*>); \
    break; \
  case 14: \
    stmt(std::get<PochiVM::Value<uint32_t*>>(expr), uint32_t*, PochiVM::Value<uint32_t*>); \
    break; \
  case 15: \
    stmt(std::get<PochiVM::Value<uint64_t*>>(expr), uint64_t*, PochiVM::Value<uint64_t*>); \
    break; \
  case 16: \
    stmt(std::get<PochiVM::Value<int8_t*>>(expr), int8_t*, PochiVM::Value<int8_t*>); \
    break; \
  case 17: \
    stmt(std::get<PochiVM::Value<int16_t*>>(expr), int16_t*, PochiVM::Value<int16_t*>); \
    break; \
  case 18: \
    stmt(std::get<PochiVM::Value<int32_t*>>(expr), int32_t*, PochiVM::Value<int32_t*>); \
    break; \
  case 19: \
    stmt(std::get<PochiVM::Value<int64_t*>>(expr), int64_t*, PochiVM::Value<int64_t*>); \
    break; \
  case 20: \
    stmt(std::get<PochiVM::Value<float*>>(expr), float*, PochiVM::Value<float*>); \
    break; \
  case 21: \
    stmt(std::get<PochiVM::Value<double*>>(expr), double*, PochiVM::Value<double*>); \
    break; \
\
  case 35: \
    stmt(std::get<PochiVM::Variable<uint8_t*>>(expr), uint8_t*, PochiVM::Variable<uint8_t*>); \
    break; \
  case 36: \
    stmt(std::get<PochiVM::Variable<uint16_t*>>(expr), uint16_t*, PochiVM::Variable<uint16_t*>); \
    break; \
  case 37: \
    stmt(std::get<PochiVM::Variable<uint32_t*>>(expr), uint32_t*, PochiVM::Variable<uint32_t*>); \
    break; \
  case 38: \
    stmt(std::get<PochiVM::Variable<uint64_t*>>(expr), uint64_t*, PochiVM::Variable<uint64_t*>); \
    break; \
  case 39: \
    stmt(std::get<PochiVM::Variable<int8_t*>>(expr), int8_t*, PochiVM::Variable<int8_t*>); \
    break; \
  case 40: \
    stmt(std::get<PochiVM::Variable<int16_t*>>(expr), int16_t*, PochiVM::Variable<int16_t*>); \
    break; \
  case 41: \
    stmt(std::get<PochiVM::Variable<int32_t*>>(expr), int32_t*, PochiVM::Variable<int32_t*>); \
    break; \
  case 42: \
    stmt(std::get<PochiVM::Variable<int64_t*>>(expr), int64_t*, PochiVM::Variable<int64_t*>); \
    break; \
  case 43: \
    stmt(std::get<PochiVM::Variable<float*>>(expr), float*, PochiVM::Variable<float*>); \
    break; \
  case 44: \
    stmt(std::get<PochiVM::Variable<double*>>(expr), double*, PochiVM::Variable<double*>); \
    break; \
  default: \
    if(error_on_failure) taco_uerror << "Expected a number"; \
}
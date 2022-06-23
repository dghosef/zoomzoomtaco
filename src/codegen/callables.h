#include "pochivm.h"
#include <map>
#include <string>
#include <functional>
std::map<std::string, std::function<PochiVM::ValueVT(std::vector<PochiVM::ValueVT>&)>> fns = {
  std::make_pair("taco_binarySearchAfter", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::taco_binarySearchAfter_wrapper(PochiVM::StaticCast<int *>(params[0]), PochiVM::StaticCast<int>(params[1]), PochiVM::StaticCast<int>(params[2]), PochiVM::StaticCast<int>(params[3]));
}),
  std::make_pair("taco_binarySearchBefore", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::taco_binarySearchBefore_wrapper(PochiVM::StaticCast<int *>(params[0]), PochiVM::StaticCast<int>(params[1]), PochiVM::StaticCast<int>(params[2]), PochiVM::StaticCast<int>(params[3]));
}),
  std::make_pair("calloc", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::calloc_wrapper(PochiVM::StaticCast<size_t>(params[0]), PochiVM::StaticCast<size_t>(params[1]));
}),
  std::make_pair("abs", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::abs_wrapper(PochiVM::StaticCast<int>(params[0]));
}),
  std::make_pair("labs", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::labs_wrapper(PochiVM::StaticCast<int64_t>(params[0]));
}),
  std::make_pair("log10", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::log10_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("fmod", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::fmod_wrapper(PochiVM::StaticCast<double>(params[0]), PochiVM::StaticCast<double>(params[1]));
}),
  std::make_pair("fmodf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::fmodf_wrapper(PochiVM::StaticCast<float>(params[0]), PochiVM::StaticCast<float>(params[1]));
}),
  std::make_pair("pow", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::pow_wrapper(PochiVM::StaticCast<double>(params[0]), PochiVM::StaticCast<double>(params[1]));
}),
  std::make_pair("powf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::powf_wrapper(PochiVM::StaticCast<float>(params[0]), PochiVM::StaticCast<float>(params[1]));
}),
  std::make_pair("atan2", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::atan2_wrapper(PochiVM::StaticCast<double>(params[0]), PochiVM::StaticCast<double>(params[1]));
}),
  std::make_pair("atan2f", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::atan2f_wrapper(PochiVM::StaticCast<float>(params[0]), PochiVM::StaticCast<float>(params[1]));
}),
  std::make_pair("fabs", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::fabs_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("fabsf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::fabsf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("sqrt", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::sqrt_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("sqrtf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::sqrtf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("exp", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::exp_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("expf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::expf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("log", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::log_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("logf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::logf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("sin", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::sin_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("sinf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::sinf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("cos", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::cos_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("cosf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::cosf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("tan", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::tan_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("tanf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::tanf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("asin", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::asin_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("asinf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::asinf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("acos", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::acos_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("acosf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::acosf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("atan", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::atan_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("atanf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::atanf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("sinh", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::sinh_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("sinhf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::sinhf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("cosh", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::cosh_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("coshf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::coshf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("tanh", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::tanh_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("tanhf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::tanhf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("asinh", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::asinh_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("asinhf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::asinhf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("acosh", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::acosh_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("acoshf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::acoshf_wrapper(PochiVM::StaticCast<float>(params[0]));
}),
  std::make_pair("atanh", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::atanh_wrapper(PochiVM::StaticCast<double>(params[0]));
}),
  std::make_pair("atanhf", [](std::vector<PochiVM::ValueVT> &params) -> PochiVM::ValueVT {
  return PochiVM::CallFreeFn::atanhf_wrapper(PochiVM::StaticCast<float>(params[0]));
})
};

#pragma once
#include "pochivm.h"
#include <type_traits>

template<typename T>
struct inner_type;

template<typename T>
struct inner_type<PochiVM::Variable<T>> {
  using type = T;
};

template<typename T>
struct inner_type<PochiVM::Reference<T>> {
  using type = T;
};

template<typename T>
struct inner_type<PochiVM::Value<T>> {
  using type = T;
};

template<typename T>
struct is_pochi_reference : std::false_type {};

template<typename T>
struct is_pochi_reference<PochiVM::Reference<T>> : std::true_type {};

template<typename T>
struct is_pochi_reference<PochiVM::Variable<T>> : std::true_type {};

template< class T >
inline constexpr bool is_pochi_reference_v = is_pochi_reference<T>::value;

template<typename T>
struct is_pochi_var : std::false_type {};

template<typename T>
struct is_pochi_var<PochiVM::Variable<T>> : std::true_type {};

template< class T >
inline constexpr bool is_pochi_var_v = is_pochi_var<T>::value;

template<typename T>
inline constexpr bool has_value_ptr_v = !std::is_same_v<T, PochiVM::Reference<taco_tensor_t>>;

template<typename T>
using inner_type_t = typename inner_type<T>::type;

static_assert(std::is_same<inner_type<PochiVM::Variable<int>>::type, int>::value);
static_assert(std::is_same<inner_type<PochiVM::Value<int>>::type, int>::value);
static_assert(std::is_same<inner_type<PochiVM::Reference<int>>::type, int>::value);

static_assert(std::is_same_v<inner_type_t<PochiVM::Value<int>>, int>);
static_assert(std::is_same_v<inner_type_t<PochiVM::Variable<int>>, int>);
static_assert(std::is_same_v<inner_type_t<PochiVM::Reference<int>>, int>);

static_assert(is_pochi_reference<PochiVM::Reference<int>>::value);
static_assert(!is_pochi_reference<PochiVM::Value<int>>::value);
static_assert(!is_pochi_reference<int>::value);

static_assert(is_pochi_reference_v<PochiVM::Reference<int>>);
static_assert(!is_pochi_reference_v<PochiVM::Value<int>>);
static_assert(!is_pochi_reference_v<int>);

static_assert(is_pochi_var<PochiVM::Variable<int>>::value);
static_assert(!is_pochi_var<PochiVM::Value<int>>::value);
static_assert(!is_pochi_var<int>::value);

static_assert(is_pochi_var_v<PochiVM::Variable<int>>);
static_assert(!is_pochi_var_v<PochiVM::Value<int>>);
static_assert(!is_pochi_var_v<int>);

#pragma once
#include <tuple>

template <typename T>
struct function_traits;

template <typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template <typename T, typename R, typename... Args>
struct function_traits<R(T::*)(Args...)> {
    using args = std::tuple<Args...>;
    using args_raw = std::tuple<std::remove_cvref_t<Args>...>;
    using return_type = R;
};

template <typename T, typename R, typename... Args>
struct function_traits<R(T::*)(Args...) const> {
    using args = std::tuple<Args...>;
    using args_raw = std::tuple<std::remove_cvref_t<Args>...>;
    using return_type = R;
};

template <typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    using args = std::tuple<Args...>;
    using args_raw = std::tuple<std::remove_cvref_t<Args>...>;
    using return_type = R;
};

template <typename F>
using function_args = function_traits<F>::args;

template <typename F>
using function_args_raw = function_traits<F>::args_raw;

template <typename T, template <typename...> typename To>
struct tuple_to;

template <typename... Args, template<typename...> typename To>
struct tuple_to<std::tuple<Args...>, To> {
    using result = To<Args...>;
};

template <typename T, template <typename...> typename To>
using tuple_to_t = tuple_to<T, To>::result;;

template <typename T>
struct tuple_tail;

template <typename Head, typename... Tail>
struct tuple_tail<std::tuple<Head, Tail...>> {
    using result = std::tuple<Tail...>;
};

template <typename Tuple>
using tuple_index_sequence = std::make_index_sequence<std::tuple_size_v<Tuple>>;

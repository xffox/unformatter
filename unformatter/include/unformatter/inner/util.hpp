#ifndef UNFORMATTER_INNER_UTIL_HPP
#define UNFORMATTER_INNER_UTIL_HPP

#include <cstddef>
#include <type_traits>

namespace unformatter::inner::util
{
template<typename S, template<std::size_t...> typename T>
struct IsSizedType : std::bool_constant<false>
{
};
template<template<std::size_t...> typename T, std::size_t... Sizes>
struct IsSizedType<T<Sizes...>, T> : std::bool_constant<true>
{
};
}

#endif

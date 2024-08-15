#ifndef UNFORMATTER_INNER_UTIL_HPP
#define UNFORMATTER_INNER_UTIL_HPP

#include <array>
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

template<std::size_t... Indices>
consteval bool areNondecreasingIndices()
{
    if constexpr(sizeof...(Indices) > 0)
    {
        constexpr auto indices = std::to_array({Indices...});
        for(std::size_t i = 0; i + 1 < indices.size(); ++i)
        {
            if(indices[i] > indices[i + 1])
            {
                return false;
            }
        }
    }
    return true;
}
}

#endif

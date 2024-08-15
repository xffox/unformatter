#ifndef UNFORMATTER_UTIL_HPP
#define UNFORMATTER_UTIL_HPP

#include <cstddef>
#include <tuple>

#include "unformatter/inner/util.hpp"

namespace unformatter::util
{
namespace inner
{
    template<std::size_t Offset, typename U>
    constexpr auto split(const U &unformatter)
    {
        return std::tuple{unformatter.template subs<Offset>()};
    }
    template<std::size_t Offset1, std::size_t Offset2, std::size_t... Offsets,
             typename U>
    constexpr auto split(const U &unformatter)
    {
        return std::tuple_cat(
            std::tuple{unformatter.template subs<Offset1, Offset2 - Offset1>()},
            split<Offset2, Offsets...>(unformatter));
    }
}

template<std::size_t... Offsets, typename U>
constexpr auto split(const U &unformatter)
requires requires(const U &unformatter) {
    (unformatter.template subs<0, Offsets>(), ...);
} && (unformatter::inner::util::areNondecreasingIndices<Offsets...>())
{
    return inner::split<0, Offsets...>(unformatter);
}
}

#endif

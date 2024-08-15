#ifndef UNFORMATTER_SIZE_HPP
#define UNFORMATTER_SIZE_HPP

#include <concepts>
#include <cstddef>

#include "unformatter/inner/util.hpp"

namespace unformatter
{
struct DynamicSize
{
};
template<std::size_t Start, std::size_t Size>
struct RangeSize
{
    static constexpr auto start = Start;
    static constexpr auto size = Size;
};
template<std::size_t Size>
using StaticSize = RangeSize<Size, 1>;

template<typename S>
concept SizeType = std::same_as<S, DynamicSize> ||
                   inner::util::IsSizedType<S, RangeSize>::value;

}

#endif

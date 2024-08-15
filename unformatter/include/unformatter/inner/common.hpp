#ifndef UNFORMATTER_INNER_COMMON_HPP
#define UNFORMATTER_INNER_COMMON_HPP

#include <bit>
#include <cstddef>
#include <optional>
#include <ranges>

namespace unformatter::inner::common
{
template<typename T>
struct LiteralOptional
{
    constexpr LiteralOptional(T value) : value(value), null(false)
    {
    }
    LiteralOptional() = default;

    T value{};
    bool null = true;
};

constexpr std::optional<std::size_t> subsSize(
    const std::size_t offset, const std::optional<std::size_t> maybeSize,
    const std::size_t dataSize)
{
    if(offset > dataSize)
    {
        return std::nullopt;
    }
    std::size_t size{};
    if(!maybeSize)
    {
        size = maybeSize.value_or(dataSize - offset);
    }
    else
    {
        size = *maybeSize;
        if(dataSize - offset < size)
        {
            return std::nullopt;
        }
    }
    return size;
}

constexpr bool isValidSubs(const std::size_t offset,
                           const LiteralOptional<std::size_t> &subsSize,
                           const std::size_t sizeRangeStart)
{
    return offset <= sizeRangeStart &&
           (subsSize.null || sizeRangeStart - offset >= subsSize.value);
}

template<std::endian Endian>
consteval bool isNativeEndianness()
{
    return Endian == std::endian::native;
}

// TODO: maybe pass by reference
template<std::endian Endian>
constexpr auto asEndianBytes(const auto bytes)
{
    if constexpr(isNativeEndianness<Endian>())
    {
        return bytes;
    }
    else
    {
        return bytes | std::views::reverse;
    }
}

}

#endif

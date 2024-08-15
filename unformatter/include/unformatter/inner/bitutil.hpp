#ifndef UNFORMATTER_INNER_BITUTIL_HPP
#define UNFORMATTER_INNER_BITUTIL_HPP

#include <climits>
#include <cstddef>
#include <optional>

namespace unformatter::inner::bitutil
{
namespace inner
{
    inline constexpr auto mask = ~std::byte{0};
}

inline constexpr std::size_t BYTE_BIT = CHAR_BIT;

constexpr std::byte selectBits(const std::byte val, const std::size_t offset,
                               std::optional<std::size_t> maybeSize = {})
{
    if(!maybeSize)
    {
        maybeSize = BYTE_BIT - offset;
    }
    return val & ((inner::mask << (BYTE_BIT - *maybeSize)) >> offset);
}

constexpr std::byte combineBits(const std::byte val)
{
    return val;
}
template<typename... Args>
constexpr std::byte combineBits(const std::byte cur, const std::size_t chunk,
                                const std::byte nxt, Args... args)
{
    return combineBits((cur & (inner::mask << (BYTE_BIT - chunk))) |
                           (nxt & (inner::mask >> chunk)),
                       args...);
}
}

#endif

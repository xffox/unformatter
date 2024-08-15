#ifndef UNFORMATTER_BIT_HPP
#define UNFORMATTER_BIT_HPP

#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>

namespace unformatter
{
namespace inner
{
    struct BaseBit
    {
    };
}
struct Bit : inner::BaseBit
{
    using Byte = std::byte;

    template<typename T>
    requires(!std::is_const_v<T>)
    static constexpr std::span<Byte> asBytes(std::span<T> data)
    {
        return std::as_writable_bytes(data);
    }
};
struct ConstBit : inner::BaseBit
{
    using Byte = const std::byte;

    template<typename T>
    static constexpr std::span<Byte> asBytes(std::span<T> data)
    {
        return std::as_bytes(data);
    }
};

namespace inner
{
    template<typename T, typename = void>
    struct ToBit;
    template<typename T>
    struct ToBit<T, std::enable_if_t<!std::is_const_v<T>>>
    {
        using Type = Bit;
    };
    template<typename T>
    struct ToBit<T, std::enable_if_t<std::is_const_v<T>>>
    {
        using Type = ConstBit;
    };
}

template<typename T>
concept BitType = std::derived_from<T, inner::BaseBit>;
}

#endif

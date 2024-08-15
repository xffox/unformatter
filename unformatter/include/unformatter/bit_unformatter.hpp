#ifndef UNFORMATTER_BITUNFORMATTER_HPP
#define UNFORMATTER_BITUNFORMATTER_HPP

#include <algorithm>
#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>

#include "unformatter/bit.hpp"
#include "unformatter/inner/bitutil.hpp"
#include "unformatter/inner/common.hpp"
#include "unformatter/size.hpp"
#include "unformatter/unformatter.hpp"

namespace unformatter
{
template<BitType B, SizeType S>
class BitUnformatter;

template<BitType B>
class BitUnformatter<B, DynamicSize>
{
    using MaxIntegralType = unsigned long long int;

    template<typename Bt>
    struct BitSpan
    {
        std::span<Bt> data;
        std::size_t bitOffset;
    };

    template<typename ST>
    struct DataSpan
    {
        ST data;
        std::size_t bitOffset;
    };

public:
    template<typename V>
    requires std::is_trivial_v<V>
    explicit constexpr BitUnformatter(V &val)
        : BitUnformatter(inner::ToBit<V>::Type::asBytes(std::span(&val, 1)))
    {
    }

    template<typename V>
    explicit constexpr BitUnformatter(const Unformatter<V, DynamicSize> &other)
        : BitUnformatter(inner::ToBit<V>::Type::asBytes(other.data_))
    {
    }

    [[nodiscard]]
    constexpr std::size_t size() const
    {
        return bitSize;
    }

    [[nodiscard]]
    constexpr std::optional<BitUnformatter> subs(
        std::size_t offset, std::optional<std::size_t> maybeSize = {}) const
    {
        if(const auto maybeSubsSize =
               inner::common::subsSize(offset, maybeSize, size()))
        {
            auto resultData = data_;
            if(resultData.bitOffset != 0)
            {
                const auto leftOffset =
                    inner::bitutil::BYTE_BIT - resultData.bitOffset;
                if(offset >= leftOffset)
                {
                    resultData.data = resultData.data.subspan(1);
                    resultData.bitOffset = 0;
                    offset -= leftOffset;
                }
                else
                {
                    resultData.bitOffset += offset;
                    offset = 0;
                }
            }
            resultData = advanceBit(resultData, offset);
            return BitUnformatter(resultData, *maybeSubsSize);
        }
        return std::nullopt;
    }

    template<typename V>
    requires std::is_trivial_v<V>
    [[nodiscard]]
    constexpr bool write(const V &val)
    {
        const BitUnformatter that(val);
        return writeCollection(that);
    }

    template<typename V>
    requires std::is_trivial_v<V>
    [[nodiscard]]
    constexpr bool read(V &val)
    {
        const BitUnformatter that(val);
        return readCollection(that);
    }

    template<BitType BitArg>
    [[nodiscard]]
    constexpr bool writeCollection(
        const BitUnformatter<BitArg, DynamicSize> &other) const
    {
        if(size() != other.size())
        {
            return false;
        }
        copyBits(data_, other.data_, bitSize);
        return true;
    }

    template<BitType BitArg>
    [[nodiscard]]
    constexpr bool readCollection(
        const BitUnformatter<BitArg, DynamicSize> &other) const
    {
        if(size() != other.size())
        {
            return false;
        }
        copyBits(other.data_, data_, bitSize);
        return true;
    }

    template<std::integral V>
    bool writeRepr(V val) const
    {
        constexpr auto bytesRoundUp = [](const std::size_t bits) {
            return (bits + inner::bitutil::BYTE_BIT - 1) /
                   inner::bitutil::BYTE_BIT;
        };
        if(!isRepresentable(val, bitSize))
        {
            return false;
        }
        const MaxIntegralType cur = val;
        const auto curBitSize = std::min(
            sizeof(MaxIntegralType) * inner::bitutil::BYTE_BIT, bitSize);
        const auto byteSize = bytesRoundUp(curBitSize);
        const auto src = [&] {
            const auto bytes = std::as_bytes(std::span(&cur, 1));
            if constexpr(inner::common::isNativeEndianness<
                             std::endian::little>())
            {
                return bytes.subspan(0, byteSize) | std::views::reverse;
            }
            else
            {
                return bytes.subspan(bytes.size() - byteSize);
            }
        }();
        auto curSpan = data_;
        if(const auto extraZeroBits = bitSize - curBitSize)
        {
            copyBits(curSpan,
                     DataSpan{std::views::iota(bytesRoundUp(extraZeroBits)) |
                                  std::views::transform(
                                      [](const auto) { return std::byte{0}; }),
                              0},
                     extraZeroBits);
            curSpan = advanceBit(curSpan, extraZeroBits);
        }
        copyBits(curSpan,
                 DataSpan{src, (inner::bitutil::BYTE_BIT -
                                curBitSize % inner::bitutil::BYTE_BIT) %
                                   inner::bitutil::BYTE_BIT},
                 curBitSize);
        return true;
    }

protected:
    template<std::integral V>
    static constexpr bool isRepresentable(const V value, const std::size_t bits)
    {
        auto limit = ~(static_cast<MaxIntegralType>(0));
        const auto limitSize = sizeof(limit) * inner::bitutil::BYTE_BIT;
        if(bits > limitSize)
        {
            return true;
        }
        limit >>= (limitSize - bits);
        return value >= 0 && static_cast<MaxIntegralType>(value) <= limit;
    }

private:
    explicit constexpr BitUnformatter(std::span<typename B::Byte> data)
        : BitUnformatter(BitSpan{data, 0},
                         data.size() * inner::bitutil::BYTE_BIT)
    {
    }

    constexpr BitUnformatter(BitSpan<typename B::Byte> data,
                             std::size_t bitSize)
        : data_(data), bitSize(bitSize)
    {
    }

    template<typename Dst, typename Src>
    static constexpr void copyBits(const Dst &dst, const Src &src,
                                   std::size_t bitSize)
    {
        if(dst.bitOffset == 0 && src.bitOffset == 0)
        {
            std::ranges::copy(src.data, std::ranges::begin(dst.data));
        }
        else
        {
            auto srcIter = std::ranges::begin(src.data);
            auto dstIter = std::ranges::begin(dst.data);
            auto leftSize = bitSize;
            for(; leftSize > 0;
                leftSize -= std::min(leftSize, inner::bitutil::BYTE_BIT),
                ++srcIter, ++dstIter)
            {
                const auto srcValChunk =
                    inner::bitutil::BYTE_BIT - src.bitOffset;
                const auto val = inner::bitutil::combineBits(
                    *srcIter << src.bitOffset, srcValChunk,
                    (leftSize > srcValChunk && src.bitOffset != 0
                         ? (*(srcIter + 1) >> srcValChunk)
                         : std::byte{0}));
                *(dstIter) = inner::bitutil::combineBits(
                    *(dstIter), dst.bitOffset, val >> dst.bitOffset,
                    std::min(dst.bitOffset + leftSize,
                             inner::bitutil::BYTE_BIT),
                    *(dstIter));
                const auto dstValChunk =
                    inner::bitutil::BYTE_BIT - dst.bitOffset;
                if(leftSize > dstValChunk && dst.bitOffset != 0)
                {
                    *(dstIter + 1) = inner::bitutil::combineBits(
                        val << dstValChunk,
                        std::min(leftSize - dstValChunk,
                                 inner::bitutil::BYTE_BIT),
                        *(dstIter + 1));
                }
            }
        }
    }

    template<typename Bt>
    static constexpr BitSpan<Bt> advanceBit(const BitSpan<Bt> &span,
                                            const std::size_t offset)
    {
        return {span.data.subspan(offset / inner::bitutil::BYTE_BIT),
                span.bitOffset + offset % inner::bitutil::BYTE_BIT};
    }

public:
    BitSpan<typename B::Byte> data_;
    std::size_t bitSize;
};

template<BitType B, std::size_t RngStart, std::size_t RngSize>
class BitUnformatter<B, RangeSize<RngStart, RngSize>>
    : public BitUnformatter<B, DynamicSize>
{
    template<BitType, SizeType>
    friend class BitUnformatter;

public:
    template<typename V>
    requires std::is_trivial_v<V> &&
             (RngSize == 1 && sizeof(V) * inner::bitutil::BYTE_BIT == RngStart)
    explicit constexpr BitUnformatter(V &val)
        : BitUnformatter<B, DynamicSize>(val)
    {
    }
    template<std::size_t OtherRngStart, std::size_t OtherRngSize, typename V>
    requires(OtherRngStart *inner::bitutil::BYTE_BIT == RngStart &&
             OtherRngSize * inner::bitutil::BYTE_BIT == RngSize)
    explicit constexpr BitUnformatter(
        const Unformatter<V, RangeSize<OtherRngStart, OtherRngSize>> &other)
        : BitUnformatter<B, DynamicSize>(other)
    {
    }

    using BitUnformatter<B, DynamicSize>::subs;
    template<std::size_t Offset,
             inner::common::LiteralOptional<std::size_t> SubsSize =
                 inner::common::LiteralOptional<std::size_t>{}>
    requires(inner::common::isValidSubs(Offset, SubsSize, RngStart))
    [[nodiscard]]
    constexpr auto subs() const
    {
        if constexpr(SubsSize.null)
        {
            return BitUnformatter<B, RangeSize<RngStart - Offset, RngSize>>(
                *BitUnformatter<B, DynamicSize>::subs(Offset));
        }
        else
        {
            return BitUnformatter<B, RangeSize<SubsSize.value, 1>>(
                *BitUnformatter<B, DynamicSize>::subs(Offset, SubsSize.value));
        }
    }

    using BitUnformatter<B, DynamicSize>::writeCollection;
    template<std::size_t OtherRngStart, std::size_t OtherRngSize>
    constexpr void writeCollection(
        const BitUnformatter<B, RangeSize<OtherRngStart, OtherRngSize>> &other)
        const = delete;
    template<std::size_t OtherRngStart, std::size_t OtherRngSize>
    requires(RngSize == 1 && OtherRngSize == 1 && RngStart == OtherRngStart)
    constexpr void writeCollection(
        const BitUnformatter<B, RangeSize<OtherRngStart, OtherRngSize>> &other)
        const
    {
        [[maybe_unused]]
        const auto res = BitUnformatter<B, DynamicSize>::writeCollection(other);
        assert(res);
    }

    using BitUnformatter<B, DynamicSize>::readCollection;
    template<std::size_t OtherRngStart, std::size_t OtherRngSize>
    constexpr void readCollection(
        const BitUnformatter<B, RangeSize<OtherRngStart, OtherRngSize>> &other)
        const = delete;
    template<std::size_t OtherRngStart, std::size_t OtherRngSize>
    requires(RngSize == 1 && OtherRngSize == 1 && RngStart == OtherRngStart)
    constexpr void readCollection(
        const BitUnformatter<B, RangeSize<OtherRngStart, OtherRngSize>> &other)
        const
    {
        [[maybe_unused]]
        const auto res = BitUnformatter<B, DynamicSize>::readCollection(other);
        assert(res);
    }

    using BitUnformatter<B, DynamicSize>::writeRepr;
    template<auto Value>
    requires(BitUnformatter<B, DynamicSize>::isRepresentable(Value, RngStart) &&
             RngSize == 1)
    constexpr void writeRepr() const
    {
        [[maybe_unused]]
        const auto res = BitUnformatter<B, DynamicSize>::writeRepr(Value);
        assert(res);
    }

private:
    explicit constexpr BitUnformatter(
        const BitUnformatter<B, DynamicSize> &that)
        : BitUnformatter<B, DynamicSize>(that)
    {
    }
};

template<BitType B>
using BitUnformatterDynamic = BitUnformatter<B, DynamicSize>;
template<BitType B, std::size_t Start, std::size_t End>
requires(Start <= End)
using BitUnformatterRanged =
    BitUnformatter<B, RangeSize<Start, End - Start + 1>>;

template<typename V>
requires std::is_trivial_v<V>
constexpr auto createBit(V &val)
{
    return BitUnformatter<typename inner::ToBit<V>::Type,
                          RangeSize<sizeof(V) * inner::bitutil::BYTE_BIT, 1>>(
        val);
}
template<std::size_t RngStart, std::size_t RngSize, typename V>
constexpr auto createBit(
    const Unformatter<V, RangeSize<RngStart, RngSize>> &other)
{
    return BitUnformatter<typename inner::ToBit<V>::Type,
                          RangeSize<RngStart * inner::bitutil::BYTE_BIT,
                                    RngSize * inner::bitutil::BYTE_BIT>>(other);
}
}

#endif

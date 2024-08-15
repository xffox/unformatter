#ifndef UNFORMATTER_UNFORMATTER_HPP
#define UNFORMATTER_UNFORMATTER_HPP

#include <algorithm>
#include <bit>
#include <cassert>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>

#include "unformatter/bit.hpp"
#include "unformatter/inner/common.hpp"
#include "unformatter/inner/util.hpp"
#include "unformatter/size.hpp"

namespace unformatter
{
namespace inner
{
    template<typename D>
    requires std::constructible_from<std::string_view, D>
    constexpr auto prepareSpan(D &&data)
    {
        auto view = std::string_view(data);
        return std::span<const typename decltype(view)::value_type>{view};
    }
    template<typename D>
    requires(!std::constructible_from<std::string_view, D> &&
             requires(D &&data) { std::span(data); })
    constexpr auto prepareSpan(D &&data)
    {
        return std::span(data);
    }

    template<typename Left, typename Right>
    requires(inner::util::IsSizedType<Left, RangeSize>::value &&
             inner::util::IsSizedType<Right, RangeSize>::value)
    consteval bool isIntersectingRanges()
    {
        if(Left::start > Right::start)
        {
            return isIntersectingRanges<Right, Left>();
        }
        return Left::size > 0 && Right::size > 0 &&
               Right::start - Left::start < Left::size;
    }

    template<typename V, std::size_t Size>
    consteval std::size_t bufferSize()
    {
        return sizeof(V) * Size;
    }

    template<typename D>
    concept SpanLike = requires(D &&data) { prepareSpan(data); };

    template<typename V>
    concept StringDataType = requires(std::span<V> data) {
        std::string_view(data.begin(), data.end());
    };

    template<typename R>
    requires inner::util::IsSizedType<R, RangeSize>::value
    constexpr bool isInRange(const std::size_t val)
    {
        return val >= R::start && val - R::start < R::size;
    }
}

template<typename T, SizeType S>
class Unformatter;

template<typename T>
class Unformatter<T, DynamicSize>
{
    template<typename, SizeType>
    friend class Unformatter;

    template<BitType, SizeType>
    friend class BitUnformatter;

public:
    explicit constexpr Unformatter(std::span<T> data) : data_(data)
    {
    }
    template<inner::SpanLike D>
    explicit constexpr Unformatter(D &&data) : data_(inner::prepareSpan(data))
    {
    }

    friend bool operator==(const Unformatter &left, const Unformatter &right)
    {
        return std::ranges::equal(left.data_, right.data_);
    }

    constexpr operator std::span<T>() const
    {
        return data_;
    }
    constexpr operator std::string_view() const
    requires inner::StringDataType<T>
    {
        return std::string_view(data_.begin(), data_.end());
    }
    constexpr std::span<T> operator*() const
    {
        return static_cast<std::span<T>>(*this);
    }

    auto begin() const
    {
        return data_.begin();
    }
    auto end() const
    {
        return data_.end();
    }

    [[nodiscard]] constexpr std::optional<Unformatter> subs(
        std::size_t offset, std::optional<std::size_t> maybeSize = {}) const
    {
        if(const auto maybeSubsSize =
               inner::common::subsSize(offset, maybeSize, data_.size()))
        {
            return Unformatter(data_.subspan(offset, *maybeSubsSize));
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::tuple<Unformatter, Unformatter>> split(
        const std::size_t offset) const
    {
        if(offset > data_.size())
        {
            return std::nullopt;
        }
        return std::tuple{subs(0, offset), subs(offset)};
    }

    template<typename V, std::endian Endian = std::endian::native>
    [[nodiscard]] constexpr std::optional<V> read() const
    {
        if(bufferSize() != sizeof(V))
        {
            return std::nullopt;
        }
        V dst[1]{};
        std::ranges::copy(
            inner::common::asEndianBytes<Endian>(std::as_bytes(data_)),
            std::as_writable_bytes(std::span{dst}).begin());
        return dst[0];
    }
    template<std::endian Endian = std::endian::native, typename V>
    [[nodiscard]] constexpr bool readCollection(
        const Unformatter<V, DynamicSize> &other) const
    {
        if(bufferSize() != other.bufferSize())
        {
            return false;
        }
        copyBytes<Endian>(std::as_writable_bytes(other.data_),
                          std::as_bytes(data_), sizeof(V));
        return true;
    }

    template<std::endian Endian = std::endian::native, typename V>
    [[nodiscard]] constexpr bool write(const V val) const
    {
        if(bufferSize() != sizeof(V))
        {
            return false;
        }
        const V src[1]{val};
        std::ranges::copy(
            inner::common::asEndianBytes<Endian>(std::as_bytes(std::span{src})),
            std::as_writable_bytes(data_).begin());
        return true;
    }
    template<std::endian Endian = std::endian::native, typename V>
    [[nodiscard]] constexpr bool writeCollection(
        const Unformatter<V, DynamicSize> &other) const
    {
        if(bufferSize() != other.bufferSize())
        {
            return false;
        }
        copyBytes<Endian>(std::as_writable_bytes(data_),
                          std::as_bytes(other.data_), sizeof(V));
        return true;
    }

    template<std::integral V>
    requires inner::StringDataType<T>
    [[nodiscard]] std::optional<V> readString(
        const unsigned int base = 10) const
    {
        const auto *first = data_.data();
        const auto *last = first + data_.size();
        V result{};
        const auto res = std::from_chars(first, last, result, base);
        if(res.ec == std::errc{} && res.ptr == last)
        {
            return result;
        }
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return data_.size();
    }

protected:
    [[nodiscard]]
    constexpr std::size_t bufferSize() const
    {
        return data_.size() * sizeof(T);
    }

    std::span<T> data_;

private:
    template<std::endian Endian>
    static constexpr void copyBytes(std::span<std::byte> dst,
                                    std::span<const std::byte> src,
                                    const std::size_t chunkSize)
    {
        assert(src.size() == dst.size());
        assert(src.size() % chunkSize == 0);
        if constexpr(inner::common::isNativeEndianness<Endian>())
        {
            std::ranges::copy(src, dst.begin());
        }
        else
        {
            for(auto offset = decltype(chunkSize){}; offset < src.size();
                offset += chunkSize)
            {
                std::ranges::copy(inner::common::asEndianBytes<Endian>(
                                      src.subspan(offset, chunkSize)),
                                  dst.subspan(offset, chunkSize).begin());
            }
        }
    }
};

template<typename T, std::size_t RngStart, std::size_t RngSize>
class Unformatter<T, RangeSize<RngStart, RngSize>>
    : public Unformatter<T, DynamicSize>
{
    static_assert(std::is_trivial_v<T>);

    template<typename, SizeType>
    friend class Unformatter;

    using SzType = RangeSize<RngStart, RngSize>;

public:
    template<inner::SpanLike D>
    [[nodiscard]] constexpr static std::optional<Unformatter> create(D &&data)
    {
        const auto span = inner::prepareSpan(data);
        if(inner::isInRange<SzType>(span.size()))
        {
            return Unformatter(span);
        }
        return std::nullopt;
    }

    template<typename V, std::endian Endian = std::endian::native>
    requires(std::is_trivial_v<V> && RngSize == 1 &&
             sizeof(V) == inner::bufferSize<T, RngStart>())
    [[nodiscard]] constexpr V read() const
    {
        return *Unformatter<T, DynamicSize>::template read<V, Endian>();
    }

    using Unformatter<T, DynamicSize>::readCollection;

    template<std::endian Endian = std::endian::native, typename V,
             std::size_t OtherRngStart, std::size_t OtherRngSize>
    constexpr void readCollection(
        const Unformatter<V, RangeSize<OtherRngStart, OtherRngSize>> &other) =
        delete;
    template<std::endian Endian = std::endian::native, typename V,
             std::size_t OtherRngStart, std::size_t OtherRngSize>
    requires(std::is_trivial_v<V> && RngSize == 1 && OtherRngSize == 1 &&
             inner::bufferSize<T, RngStart>() ==
                 inner::bufferSize<V, OtherRngStart>())
    constexpr void readCollection(
        const Unformatter<V, RangeSize<OtherRngStart, OtherRngSize>> &other)
        const
    {
        [[maybe_unused]]
        const auto res =
            Unformatter<T, DynamicSize>::template readCollection<Endian>(other);
        assert(res);
    }

    template<std::endian Endian = std::endian::native, typename V>
    requires(std::is_trivial_v<V> && RngSize == 1 &&
             sizeof(V) == inner::bufferSize<T, RngStart>())
    constexpr void write(const V val) const
    {
        [[maybe_unused]]
        const auto res =
            Unformatter<T, DynamicSize>::template write<Endian>(val);
        assert(res);
    }

    using Unformatter<T, DynamicSize>::writeCollection;

    template<std::endian Endian = std::endian::native, typename V,
             std::size_t OtherRngStart, std::size_t OtherRngSize>
    constexpr void writeCollection(
        const Unformatter<V, RangeSize<OtherRngStart, OtherRngSize>> &other)
        const = delete;
    template<std::endian Endian = std::endian::native, typename V,
             std::size_t OtherRngStart, std::size_t OtherRngSize>
    requires(std::is_trivial_v<V> && RngSize == 1 && OtherRngSize == 1 &&
             inner::bufferSize<T, RngStart>() ==
                 inner::bufferSize<V, OtherRngStart>())
    constexpr void writeCollection(
        const Unformatter<V, RangeSize<OtherRngStart, OtherRngSize>> &other)
        const
    {
        [[maybe_unused]]
        const auto res =
            Unformatter<T, DynamicSize>::template writeCollection<Endian>(
                other);
        assert(res);
    }

    using Unformatter<T, DynamicSize>::subs;

    template<std::size_t Offset,
             inner::common::LiteralOptional<std::size_t> SubsSize =
                 inner::common::LiteralOptional<std::size_t>{}>
    requires(inner::common::isValidSubs(Offset, SubsSize, RngStart))
    constexpr auto subs() const
    {
        if constexpr(SubsSize.null)
        {
            return Unformatter<T, RangeSize<RngStart - Offset, RngSize>>(
                Unformatter<T, DynamicSize>::subs(Offset)->data_);
        }
        else
        {
            return Unformatter<T, RangeSize<SubsSize.value, 1>>(
                Unformatter<T, DynamicSize>::subs(Offset, SubsSize.value)
                    ->data_);
        }
    }

    template<std::size_t Offset>
    requires(Offset <= RngStart)
    [[nodiscard]] auto split() const
    {
        return std::tuple{subs<0, Offset>(), subs<Offset>()};
    }

    template<std::size_t NewRngStart, std::size_t NewRngSize>
    requires((NewRngStart != RngStart || NewRngSize != RngSize) &&
             NewRngStart <= RngStart && NewRngSize >= RngSize &&
             RngStart - NewRngStart <= NewRngSize - RngSize)
    constexpr operator Unformatter<T, RangeSize<NewRngStart, NewRngSize>>()
        const
    {
        return Unformatter<T, RangeSize<NewRngStart, NewRngSize>>(this->data_);
    }

private:
    constexpr explicit Unformatter(std::span<T> data)
        : Unformatter<T, DynamicSize>(data)
    {
    }
};

template<typename V, std::size_t LeftStart, std::size_t LeftSize,
         std::size_t RightStart, std::size_t RightSize>
bool operator==(const Unformatter<V, RangeSize<LeftStart, LeftSize>> &left,
                const Unformatter<V, RangeSize<RightStart, RightSize>> &right) =
    delete;
template<typename V, std::size_t LeftStart, std::size_t LeftSize,
         std::size_t RightStart, std::size_t RightSize>
bool operator==(const Unformatter<V, RangeSize<LeftStart, LeftSize>> &left,
                const Unformatter<V, RangeSize<RightStart, RightSize>> &right)
requires(inner::isIntersectingRanges<RangeSize<LeftStart, LeftSize>,
                                     RangeSize<RightStart, RightSize>>())
{
    using U = Unformatter<V, DynamicSize>;
    return static_cast<const U &>(left) == static_cast<const U &>(right);
}

template<typename T>
using UnformatterDynamic = Unformatter<T, DynamicSize>;
template<typename T, std::size_t Size>
using UnformatterStatic = Unformatter<T, StaticSize<Size>>;
template<typename T, std::size_t Start, std::size_t End>
requires(Start <= End)
using UnformatterRanged = Unformatter<T, RangeSize<Start, End - Start + 1>>;

template<std::size_t Start, std::size_t End = Start, inner::SpanLike D>
requires(Start <= End)
constexpr auto create(D &&data)
{
    auto span = inner::prepareSpan(data);
    return UnformatterRanged<typename decltype(span)::element_type, Start,
                             End>::create(span);
}
template<std::size_t Start, std::size_t End = Start, typename T>
requires(Start <= End)
constexpr auto create(Unformatter<T, DynamicSize> data)
{
    return UnformatterRanged<T, Start, End>::create(*data);
}
}

#endif

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "unformatter/unformatter.hpp"

TEST_CASE("unformatter read string value", "[unformatter]")
{
    const auto maybeDataUnfmt = unformatter::create<7>("a423def");
    REQUIRE(maybeDataUnfmt);
    const auto dataUnfmt = *maybeDataUnfmt;
    const auto valueUfmt = dataUnfmt.subs<1, 3>();
    REQUIRE(valueUfmt.readString<unsigned int>() == 423);
}

TEST_CASE("unformatter write value", "[unformatter]")
{
    using Value = std::uint64_t;
    constexpr std::size_t PADDING = 4;
    constexpr auto VALUE_SIZE = sizeof(Value);
    constexpr std::size_t SIZE = VALUE_SIZE + PADDING * 2;
    std::array<unsigned char, SIZE> buf{};
    const auto maybeBufUnfmt = unformatter::create<SIZE>(buf);
    REQUIRE(maybeBufUnfmt);
    const auto bufUnfmt = *maybeBufUnfmt;
    const auto valueUnfmt = bufUnfmt.subs<PADDING, VALUE_SIZE>();
    valueUnfmt.write<std::endian::little>(0x213408f8e23ad259);
    REQUIRE(buf == std::to_array<unsigned char>({
                       0,
                       0,
                       0,
                       0,
                       0x59,
                       0xd2,
                       0x3a,
                       0xe2,
                       0xf8,
                       0x08,
                       0x34,
                       0x21,
                       0,
                       0,
                       0,
                       0,
                   }));
    std::ranges::fill(buf, 0);
    valueUnfmt.write<std::endian::big>(0x87f80d928fd80a31);
    REQUIRE(buf == std::to_array<unsigned char>({
                       0,
                       0,
                       0,
                       0,
                       0x87,
                       0xf8,
                       0x0d,
                       0x92,
                       0x8f,
                       0xd8,
                       0x0a,
                       0x31,
                       0,
                       0,
                       0,
                       0,
                   }));
}

TEST_CASE("unformatter write collection", "[unformatter]")
{
    using Value = std::uint16_t;
    constexpr std::size_t PADDING = 4;
    constexpr std::size_t COUNT = 3;
    constexpr auto VALUE_SIZE = sizeof(Value);
    constexpr std::size_t SIZE = VALUE_SIZE * COUNT + PADDING * 2;
    std::array<unsigned char, SIZE> buf{};
    const auto maybeBufUnfmt = unformatter::create<SIZE>(buf);
    REQUIRE(maybeBufUnfmt);
    const auto bufUnfmt = *maybeBufUnfmt;
    const auto valueUnfmt = bufUnfmt.subs<PADDING, VALUE_SIZE * COUNT>();
    {
        std::array<Value, COUNT> source{
            0x1325,
            0xf38a,
            0xda13,
        };
        const auto maybeSourceUnfmt = unformatter::create<COUNT>(source);
        const auto sourceUnfmt = *maybeSourceUnfmt;
        valueUnfmt.writeCollection<std::endian::little>(sourceUnfmt);
        REQUIRE(buf == std::to_array<unsigned char>({
                           0,
                           0,
                           0,
                           0,
                           0x25,
                           0x13,
                           0x8a,
                           0xf3,
                           0x13,
                           0xda,
                           0,
                           0,
                           0,
                           0,
                       }));
    }
    std::ranges::fill(buf, 0);
    {
        std::array<Value, COUNT> source{
            0xd82a,
            0x4321,
            0x238e,
        };
        const auto maybeSourceUnfmt = unformatter::create<COUNT>(source);
        const auto sourceUnfmt = *maybeSourceUnfmt;
        valueUnfmt.writeCollection<std::endian::big>(sourceUnfmt);
        REQUIRE(buf == std::to_array<unsigned char>({
                           0,
                           0,
                           0,
                           0,
                           0xd8,
                           0x2a,
                           0x43,
                           0x21,
                           0x23,
                           0x8e,
                           0,
                           0,
                           0,
                           0,
                       }));
    }
}

TEST_CASE("unformatter read value", "[unformatter]")
{
    using Value = std::uint64_t;
    constexpr std::size_t PADDING = 4;
    constexpr auto VALUE_SIZE = sizeof(Value);
    constexpr std::size_t SIZE = VALUE_SIZE + PADDING * 2;
    std::array<unsigned char, SIZE> buf = {
        0, 0, 0, 0, 0x68, 0x3d, 0x92, 0x37, 0xde, 0x38, 0xab, 0x79, 0, 0, 0, 0,
    };
    const auto maybeBufUnfmt = unformatter::create<SIZE>(buf);
    REQUIRE(maybeBufUnfmt);
    const auto bufUnfmt = *maybeBufUnfmt;
    const auto valueUnfmt = bufUnfmt.subs<PADDING, VALUE_SIZE>();
    REQUIRE(valueUnfmt.read<Value, std::endian::little>() ==
            0x79ab38de37923d68);
    buf = {
        0, 0, 0, 0, 0xd2, 0x82, 0xc8, 0x73, 0x98, 0x23, 0xe8, 0x31, 0, 0, 0, 0,
    };
    REQUIRE(valueUnfmt.read<Value, std::endian::big>() == 0xd282c8739823e831);
}

TEST_CASE("unformatter read collection", "[unformatter]")
{
    using Value = std::uint16_t;
    constexpr std::size_t PADDING = 4;
    constexpr std::size_t COUNT = 3;
    constexpr auto VALUE_SIZE = sizeof(Value);
    constexpr std::size_t SIZE = VALUE_SIZE * COUNT + PADDING * 2;
    std::array<unsigned char, SIZE> buf{
        0, 0, 0, 0, 0x13, 0xd2, 0x7e, 0x34, 0x13, 0x82, 0, 0, 0, 0,
    };
    const auto maybeBufUnfmt = unformatter::create<SIZE>(buf);
    REQUIRE(maybeBufUnfmt);
    const auto bufUnfmt = *maybeBufUnfmt;
    const auto valueUnfmt = bufUnfmt.subs<PADDING, VALUE_SIZE * COUNT>();
    {
        std::array<Value, COUNT> data{};
        const auto maybeDataUnfmt = unformatter::create<COUNT>(data);
        const auto dataUnfmt = *maybeDataUnfmt;
        valueUnfmt.readCollection<std::endian::little>(dataUnfmt);
        REQUIRE(data == std::to_array<Value>({0xd213, 0x347e, 0x8213}));
    }
    buf = {
        0, 0, 0, 0, 0xb2, 0x89, 0xc1, 0x32, 0x71, 0xde, 0, 0, 0, 0,
    };
    {
        std::array<Value, COUNT> data{};
        const auto maybeDataUnfmt = unformatter::create<COUNT>(data);
        const auto dataUnfmt = *maybeDataUnfmt;
        valueUnfmt.readCollection<std::endian::big>(dataUnfmt);
        REQUIRE(data == std::to_array<Value>({0xb289, 0xc132, 0x71de}));
    }
}

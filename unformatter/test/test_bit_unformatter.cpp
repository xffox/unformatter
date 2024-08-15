#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "unformatter/bit_unformatter.hpp"
#include "unformatter/unformatter.hpp"

TEST_CASE("bit unformatter write", "[bit_unformatter]")
{
    std::uint16_t data{};
    const auto dataUnfmt = unformatter::createBit(data);
    const std::uint8_t val{0b01101101};
    const auto valUnfmt = unformatter::createBit(val);
    const auto maybeTgtUnfmt = dataUnfmt.subs(3, sizeof(val) * CHAR_BIT - 1);
    REQUIRE(maybeTgtUnfmt);
    const auto tgtUnfmt = *maybeTgtUnfmt;
    const auto res = tgtUnfmt.writeCollection(valUnfmt.subs(1).value());
    REQUIRE(res);
    REQUIRE(data == 0b0100000000011011);
}

TEST_CASE("bit unformatter write repr", "[bit_unformatter]")
{
    std::uint16_t data{};
    const auto dataUnfmt = unformatter::createBit(data);
    const auto maybeRngUnfmt = dataUnfmt.subs(5, 9);
    REQUIRE(maybeRngUnfmt);
    const auto rngUnfmt = *maybeRngUnfmt;
    REQUIRE(rngUnfmt.writeRepr(0b101010101));
    REQUIRE(data == 0b0101010000000101);
    REQUIRE_FALSE(rngUnfmt.writeRepr(0b1010101011));
}

TEST_CASE("bit unformatter static write repr", "[bit_unformatter]")
{
    std::uint16_t data{};
    const auto dataUnfmt = unformatter::createBit(data);
    const auto rngUnfmt = dataUnfmt.subs<5, 9>();
    {
        rngUnfmt.writeRepr<0b101010101>();
        REQUIRE(data == 0b0101010000000101);
    }
    data = 0;
    {
        rngUnfmt.writeRepr<0b1101011>();
        REQUIRE(data == 0b1010110000000001);
    }
}

TEST_CASE("bit unformatter from unformatter", "[bit_unformatter]")
{
    constexpr std::size_t SZ = 3;
    std::array<unsigned char, SZ> values{};
    const auto maybeValuesUnfmt = unformatter::create<SZ>(values);
    REQUIRE(maybeValuesUnfmt);
    const auto valuesUnfmt = *maybeValuesUnfmt;
    const auto bitUnfmt = unformatter::createBit(valuesUnfmt);
    {
        const auto res =
            bitUnfmt.subs<5, 19>().writeRepr(0b1010101010101010101U);
        REQUIRE(res);
        REQUIRE(values == std::to_array<unsigned char>({
                              0b00000101,
                              0b01010101,
                              0b01010101,
                          }));
    }
    std::ranges::fill(values, 0);
    {
        const auto res = bitUnfmt.subs<4, 17>().writeRepr(
            static_cast<unsigned char>(0b1010101));
        REQUIRE(res);
        REQUIRE(values == std::to_array<unsigned char>({
                              0b00000000,
                              0b00000010,
                              0b10101000,
                          }));
    }
}

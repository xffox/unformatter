#include <array>

#include <catch2/catch_test_macros.hpp>

#include "unformatter/bit_unformatter.hpp"
#include "unformatter/util.hpp"

TEST_CASE("util split", "[util]")
{
    auto val = std::to_array<unsigned char>({
        0x0,
        0x0,
        0x0,
        0x0,
    });
    const auto valUnfmt = unformatter::createBit(val);
    const auto [fstUnfmt, sndUnfmt, thrdUnfmt] =
        unformatter::util::split<3, 23>(valUnfmt);
    fstUnfmt.writeRepr<0b101>();
    sndUnfmt.writeRepr<0xfd>();
    thrdUnfmt.writeRepr<0b11111111>();
    REQUIRE(val == std::to_array<unsigned char>({
                       0b10100000,
                       0b00000001,
                       0b11111010,
                       0b11111111,
                   }));
}

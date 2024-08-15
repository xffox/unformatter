#include "unformatter/inner/bitutil.hpp"

using namespace unformatter::inner::bitutil;

namespace
{
constexpr std::byte FULL_PATTERN = std::byte{0b11111111};

static_assert(selectBits(FULL_PATTERN, 0, BYTE_BIT) == FULL_PATTERN);
static_assert(selectBits(std::byte{0b11111111}, 4, 2) == std::byte{0b00001100});

constexpr std::byte PATTERN = std::byte{0b01011010};
static_assert(combineBits(PATTERN) == PATTERN);
static_assert(combineBits(std::byte{0b01010101}, 2, std::byte{0b10101010}) ==
              std::byte{0b01101010});
static_assert(combineBits(std::byte{0b01010101}, 3, std::byte{0b10101010}, 5,
                          FULL_PATTERN) == std::byte{0b01001111});
}

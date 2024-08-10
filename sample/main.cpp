#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <span>

#include "unformatter/unformatter.hpp"

namespace
{
class IPv6Addres
{
public:
    using Group = std::uint16_t;
    static constexpr std::size_t OCTETS = 16;

    template<typename... GroupArgs>
    requires(sizeof...(GroupArgs) == OCTETS / sizeof(Group) &&
             (std::convertible_to<GroupArgs, Group> && ...))
    explicit IPv6Addres(const GroupArgs... groups)
    {
        const auto buf = std::to_array({static_cast<Group>(groups)...});
        const auto bufUnfmt = unformatter::UnformatterDynamic<const Group>(buf);
        const auto dataUnfmt =
            unformatter::UnformatterDynamic<std::byte>(data_);
        [[maybe_unused]] const auto res =
            dataUnfmt.writeCollection<std::endian::big>(bufUnfmt);
        assert(res);
    }

    [[nodiscard]]
    std::span<const std::byte, OCTETS> data() const
    {
        return data_;
    }

private:
    std::array<std::byte, OCTETS> data_{};
};

std::optional<std::span<std::byte>> prepareIPv6Packet(
    std::span<std::byte> buf, const IPv6Addres &src, const IPv6Addres &dst,
    std::span<const std::byte> data)
{
    constexpr std::size_t HEADER_SIZE = 40;
    constexpr std::size_t PAYLOAD_SIZE = 2;
    const auto maybeBufUnfmt =
        unformatter::create<HEADER_SIZE, HEADER_SIZE + (1U << (PAYLOAD_SIZE *
                                                               CHAR_BIT))>(buf);
    if(!maybeBufUnfmt)
    {
        return std::nullopt;
    }
    const auto bufUnfmt = *maybeBufUnfmt;
    const unformatter::UnformatterDynamic<const std::byte> dataUnfmt(data);
    const auto [headerUnfmt, dataBufUnfmt] = bufUnfmt.split<HEADER_SIZE>();
    if(!dataBufUnfmt.writeCollection(dataUnfmt))
    {
        return std::nullopt;
    }
    const auto [fieldUnfmt, addressUnfmt] = headerUnfmt.split<8>();
    const auto [prefixUnfmt, controlUnfmt] = fieldUnfmt.split<4>();
    {
        struct Prefix
        {
            std::uint32_t version : 4;
            std::uint32_t trafficClass : 8;
            std::uint32_t flowLabel : 20;
        } prefix{};
        prefixUnfmt.write<std::endian::big>(prefix);
    }
    const auto payloadLengthUnfmt = controlUnfmt.subs<0, 2>();
    payloadLengthUnfmt.write<std::endian::big, std::uint16_t>(data.size());
    const auto nextHeaderUnfmt = controlUnfmt.subs<2, 1>();
    nextHeaderUnfmt.write<std::endian::big, std::uint8_t>(0);
    const auto hopLimitUnfmt = controlUnfmt.subs<3, 1>();
    hopLimitUnfmt.write<std::endian::big, std::uint8_t>(64);
    const auto sourceUnfmt =
        unformatter::create<IPv6Addres::OCTETS>(src.data()).value();
    const auto destinationUnfmt =
        unformatter::create<IPv6Addres::OCTETS>(dst.data()).value();
    const auto [sourceBufUnfmt, destinationBufUnfmt] =
        addressUnfmt.split<IPv6Addres::OCTETS>();
    sourceBufUnfmt.writeCollection(sourceUnfmt);
    destinationBufUnfmt.writeCollection(destinationUnfmt);
    return buf.subspan(0, HEADER_SIZE + destinationUnfmt.size());
}

void testUnformatter()
{
    const IPv6Addres src(0x8023, 0x3333, 0x7777, 0x6655, 0x1234, 0x8888, 0x9999,
                         0xabcd);
    const IPv6Addres dst(0x90ef, 0x5555, 0x3333, 0x6789, 0xfedc, 0x2222, 0x2345,
                         0xdcba);
    constexpr std::size_t DATA_SIZE = 16;
    constexpr std::size_t HEADER_SIZE = 40;
    std::array<std::byte, HEADER_SIZE + DATA_SIZE> buf{};
    std::array<std::byte, DATA_SIZE> data{};
    std::ranges::generate(
        data, [n = 1U]() mutable { return static_cast<std::byte>(n++); });
    const auto res = prepareIPv6Packet(buf, src, dst, data);
    if(!res)
    {
        std::cerr << "packet prepare failed" << '\n';
        return;
    }
    for(const auto val : *res)
    {
        std::cerr << std::format("{:02x}", static_cast<unsigned char>(val));
    }
    std::cerr << '\n';
}

void runUnformatter()
{
    testUnformatter();
}
}

int main()
{
    runUnformatter();
    return 0;
}

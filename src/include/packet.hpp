#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <algorithm>

namespace mc::protocol
{

    struct Packet
    {
        int32_t id;
        std::vector<uint8_t> data;

        static Packet deserialize(std::span<const uint8_t> buffer);

        [[nodiscard]] std::vector<uint8_t> serialize() const;
    };

}

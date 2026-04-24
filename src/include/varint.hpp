#pragma once
#include <vector>
#include <span>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace mc::protocol
{

    constexpr uint8_t CONTINUE_BIT = 0b10000000;
    constexpr uint8_t DATA_BITS = 0b01111111;
    constexpr int MAX_BYTES = 5;

    [[nodiscard]] std::vector<uint8_t> encodeVarInt(int32_t value);

    struct VarIntResult
    {
        int32_t value;
        size_t bytesRead;
    };

    [[nodiscard]] VarIntResult decodeVarInt(std::span<const uint8_t> buffer);
}

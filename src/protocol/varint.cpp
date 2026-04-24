#include "varint.hpp"

[[nodiscard]] std::vector<uint8_t> mc::protocol::encodeVarInt(int32_t value)
{
    std::vector<uint8_t> result;

    // treat as unsigned so right shift behaves predictably
    uint32_t bits = static_cast<uint32_t>(value);

    do
    {
        uint8_t temp = bits & mc::protocol::DATA_BITS; // cut rightmost 7
        bits >>= 7;                                    // throw them away
        if (bits != 0)
        {
            temp |= mc::protocol::CONTINUE_BIT; // mark "more coming"
        }

        result.push_back(temp);
    } while (bits != 0);

    return result;
}
[[nodiscard]] mc::protocol::VarIntResult mc::protocol::decodeVarInt(std::span<const uint8_t> buffer)
{

    int32_t value = 0;
    size_t bytesRead = 0;
    auto shift = 0;
    for (auto i : buffer)
    {

        bytesRead++;

        uint32_t data = i & mc::protocol::DATA_BITS;
        data <<= shift;
        value |= data;
        shift += 7;
        if ((i & mc::protocol::CONTINUE_BIT) == 0)
        {
            break;
        }
        if (bytesRead > mc::protocol::MAX_BYTES)
        {
            throw std::runtime_error("VarInt exceeded maximum of 5 bytes - malformed packet");
        }
    }
    return {static_cast<int32_t>(value), bytesRead};
}
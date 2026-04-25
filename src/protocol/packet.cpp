#include "packet.hpp"
#include "varint.hpp"
namespace mc::protocol
{
    [[nodiscard]] std::vector<uint8_t> mc::protocol::Packet::serialize() const
    {
        auto encodeId = mc::protocol::encodeVarInt(this->id);

        int32_t lengthValue = static_cast<int32_t>(encodeId.size() + this->data.size());
        auto encodeLength = mc::protocol::encodeVarInt(lengthValue);
        std::vector<uint8_t> result;

        result.insert(result.end(), encodeLength.begin(), encodeLength.end());
        result.insert(result.end(), encodeId.begin(), encodeId.end());
        result.insert(result.end(), data.begin(), data.end());

        return result;
    }

    Packet Packet::deserialize(std::span<const uint8_t> buffer)
    {
        auto resultLength = mc::protocol::decodeVarInt(buffer);
        buffer = buffer.subspan(resultLength.bytesRead);

        auto resultID = mc::protocol::decodeVarInt(buffer);
        int32_t packetId = resultID.value;
        buffer = buffer.subspan(resultID.bytesRead);

        std::vector<uint8_t> packetData(buffer.begin(), buffer.end());

        return {packetId, packetData};
    }

    void writeInt32(std::vector<uint8_t> &data, int32_t value)
    {
        data.push_back((value >> 24) & 0xFF);
        data.push_back((value >> 16) & 0xFF);
        data.push_back((value >> 8) & 0xFF);
        data.push_back((value >> 0) & 0xFF);
    }
}
#include <client.hpp>

namespace mc::helper
{
    void writeInt16(std::vector<uint8_t> &data, int16_t value)
    {
        data.push_back((value >> 8) & 0xFF);
        data.push_back((value >> 0) & 0xFF);
    }
    void writeInt32(std::vector<uint8_t> &data, int32_t value)

    {
        data.push_back((value >> 24) & 0xFF);
        data.push_back((value >> 16) & 0xFF);
        data.push_back((value >> 8) & 0xFF);
        data.push_back((value >> 0) & 0xFF);
    }
    void writeInt64(std::vector<uint8_t> &data, int64_t value)
    {
        data.push_back((value >> 56) & 0xFF);
        data.push_back((value >> 48) & 0xFF);
        data.push_back((value >> 40) & 0xFF);
        data.push_back((value >> 32) & 0xFF);
        data.push_back((value >> 24) & 0xFF);
        data.push_back((value >> 16) & 0xFF);
        data.push_back((value >> 8) & 0xFF);
        data.push_back((value >> 0) & 0xFF);
    }
    void writeDouble(std::vector<uint8_t> &data, double value)
    {
        uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        writeInt64(data, static_cast<int64_t>(bits));
    }

    void writeFloat(std::vector<uint8_t> &data, float value)
    {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        writeInt32(data, static_cast<int32_t>(bits));
    }
    void writeLegacyString(std::vector<uint8_t> &data, const std::string &str)
    {
        // write length as big endian short (2 bytes)
        uint16_t len = static_cast<uint16_t>(str.size());
        data.push_back((len >> 8) & 0xFF);
        data.push_back(len & 0xFF);
        // write each character as 2 bytes (UTF-16BE)
        for (char c : str)
        {
            data.push_back(0x00);
            data.push_back(static_cast<uint8_t>(c));
        }
    }
    uint64_t readUint64(const std::vector<uint8_t> &data, size_t &offset)
    {
        std::array<uint8_t, 8> arr;
        std::copy_n(data.data() + offset, 8, arr.begin());
        offset += 8;
        uint64_t val = std::bit_cast<uint64_t>(arr);
        if constexpr (std::endian::native == std::endian::little)
        {
            return std::byteswap(val);
        }
        return val;
    }

    uint32_t readUint32(const std::vector<uint8_t> &data, size_t &offset)
    {
        std::array<uint8_t, 4> arr;
        std::copy_n(data.data() + offset, 4, arr.begin());
        offset += 4;
        uint32_t val = std::bit_cast<uint32_t>(arr);
        if constexpr (std::endian::native == std::endian::little)
        {
            return std::byteswap(val);
        }
        return val;
    }

    uint16_t readShort(const std::vector<uint8_t> &data, size_t &offset)
    {
        std::array<uint8_t, 2> arr;
        std::copy_n(data.data() + offset, 2, arr.begin());
        offset += 2;
        uint16_t val = std::bit_cast<uint16_t>(arr);
        if constexpr (std::endian::native == std::endian::little)
        {
            return std::byteswap(val);
        }
        return val;
    }
    uint8_t readByte(const std::vector<uint8_t> &data, size_t &offset)
    {
        std::array<uint8_t, 1> arr;
        std::copy_n(data.data() + offset, 1, arr.begin());
        offset += 1;
        uint8_t val = std::bit_cast<uint8_t>(arr);
        if constexpr (std::endian::native == std::endian::little)
        {
            return std::byteswap(val);
        }
        return val;
    }
    double readDouble(const std::vector<uint8_t> &data, size_t &offset)
    {
        uint64_t val = readUint64(data, offset);
        return std::bit_cast<double>(val);
    }

    float readFloat(const std::vector<uint8_t> &data, size_t &offset)
    {
        uint32_t val = readUint32(data, offset);
        return std::bit_cast<float>(val);
    }

    bool readBoolean(const std::vector<uint8_t> &data, size_t &offset)
    {
        return data[offset++] != 0;
    }
    [[nodiscard]] std::expected<int32_t, std::string_view> readVarInt(std::span<const uint8_t> data, size_t &offset)
    {
        int32_t value = 0;
        int32_t position = 0;

        while (true)
        {
            if (offset >= data.size())
                return std::unexpected("Buffer overflow");

            uint8_t currentByte = data[offset++];
            value |= static_cast<int32_t>(currentByte & 0x7F) << position;

            if ((currentByte & 0x80) == 0)
                break;

            position += 7;
            if (position >= 32)
                return std::unexpected("VarInt too big");
        }
        return value;
    }
    [[nodiscard]] std::expected<std::string, std::string_view> readString(std::span<const uint8_t> data, size_t &offset)
    {
        auto lengthRes = readVarInt(data, offset);
        if (!lengthRes)
            return std::unexpected(lengthRes.error());

        int32_t length = *lengthRes;
        if (offset + length > data.size())
            return std::unexpected("String out of bounds");

        std::string str(reinterpret_cast<const char *>(data.data() + offset), length);
        offset += length;

        return str;
    }
    void writeVarInt(std::vector<uint8_t> &buffer, int32_t value)
    {
        uint32_t uValue = static_cast<uint32_t>(value);
        while (uValue >= 0x80)
        {
            buffer.push_back(static_cast<uint8_t>(uValue | 0x80));
            uValue >>= 7;
        }
        buffer.push_back(static_cast<uint8_t>(uValue));
    }

    void writeString(std::vector<uint8_t> &buffer, std::string_view str)
    {
        writeVarInt(buffer, static_cast<int32_t>(str.size()));
        buffer.insert(buffer.end(), str.begin(), str.end());
    }
}

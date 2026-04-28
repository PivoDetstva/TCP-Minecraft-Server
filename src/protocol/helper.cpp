#include <client.hpp>

namespace mc::helper
{
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

        uint32_t val = std::bit_cast<uint32_t>(arr);
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
}
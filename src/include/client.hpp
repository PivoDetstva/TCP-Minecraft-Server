#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <bit>
#include <cmath>
#include <expected>
#include "packet.hpp"
#include "varint.hpp"
#include "world.hpp"

namespace mc::network
{
    enum class ConnectionState
    {
        Handshaking,
        Status,
        Login,
        Play
    };

    class TcpClient
    {
    public:
        explicit TcpClient(int socketFd);
        ~TcpClient();

        void handle();

    private:
        int socketFd_;
        std::vector<uint8_t> buffer_;
        World world_;
        double playerX_, playerY_, playerZ_;
        int lastChunkX_, lastChunkZ_;

        bool readBytes();
        bool tryReadPacket(mc::protocol::Packet &out);
        void sendChunk(int chunkX, int chunkZ);
        void sendChunksAround(int chunkX, int chunkZ);
    };

}
namespace mc::helper
{
    void writeInt16(std::vector<uint8_t> &data, int16_t value);
    void writeInt32(std::vector<uint8_t> &data, int32_t value);
    void writeInt64(std::vector<uint8_t> &data, int64_t value);
    void writeDouble(std::vector<uint8_t> &data, double value);
    void writeFloat(std::vector<uint8_t> &data, float value);
    void writeLegacyString(std::vector<uint8_t> &data, const std::string &str);
    double readDouble(const std::vector<uint8_t> &data, size_t &offset);
    float readFloat(const std::vector<uint8_t> &data, size_t &offset);
    bool readBoolean(const std::vector<uint8_t> &data, size_t &offset);
    uint64_t readUint64(const std::vector<uint8_t> &data, size_t &offset);
    uint32_t readUint32(const std::vector<uint8_t> &data, size_t &offset);
    [[nodiscard]] std::expected<int32_t, std::string_view> readVarInt(std::span<const uint8_t> data, size_t &offset);
    [[nodiscard]] std::expected<std::string, std::string_view> readString(std::span<const uint8_t> data, size_t &offset);
    void writeVarInt(std::vector<uint8_t> &buffer, int32_t value);
    void writeString(std::vector<uint8_t> &buffer, std::string_view str);
}
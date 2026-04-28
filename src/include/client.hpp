#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <bit>
#include "packet.hpp"
#include "varint.hpp"

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

        bool readBytes();
        bool tryReadPacket(mc::protocol::Packet &out);
    };

}
namespace mc::helper
{
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

}
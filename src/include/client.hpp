#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
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
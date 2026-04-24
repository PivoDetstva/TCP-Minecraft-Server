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

        void handle(); // main loop — reads and processes packets

    private:
        int socketFd_;
        std::vector<uint8_t> buffer_; // accumulates incoming bytes

        // reads raw bytes from socket into buffer_
        bool readBytes();

        // tries to extract one complete packet from buffer_
        // returns true if a complete packet was found
        bool tryReadPacket(mc::protocol::Packet &out);
    };

}
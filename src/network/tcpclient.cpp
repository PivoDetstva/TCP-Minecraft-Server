#include "client.hpp"
namespace mc::network
{
    mc::network::TcpClient::TcpClient(int socketFd) : socketFd_(socketFd) {}

    TcpClient::~TcpClient()
    {
        if (socketFd_ >= 0)
        {
            close(socketFd_);
            socketFd_ = -1;
        }
    }

    bool TcpClient::readBytes()
    {
        uint8_t temp[1024];
        int received = read(socketFd_, temp, sizeof(temp));

        if (received <= 0)
        {
            return false;
        }
        buffer_.insert(buffer_.end(), temp, temp + received);
        return true;
    }

    bool TcpClient::tryReadPacket(mc::protocol::Packet &out)
    {
        if (buffer_.empty())
            return false;

        int bytesRead = 0;
        auto lengthResult = mc::protocol::decodeVarInt(buffer_);

        int totalSize = lengthResult.bytesRead + lengthResult.value;
        if ((int)buffer_.size() < totalSize)
        {
            return false; // not enough data yet, wait for more
        }

        // extract exactly one packet's worth of bytes
        std::span<const uint8_t> packetSpan(buffer_.data(), totalSize);
        out = mc::protocol::Packet::deserialize(packetSpan);

        // remove those bytes from the front of the buffer
        buffer_.erase(buffer_.begin(), buffer_.begin() + totalSize);

        return true;
    }
    void TcpClient::handle()
    {
        ConnectionState state = ConnectionState::Handshaking;
        while (true)
        {
            if (!readBytes())
            {
                std::cout << "Client disconnected\n";
                break;
            }

            mc::protocol::Packet packet;
            while (tryReadPacket(packet))
            {
                std::cout << "Received packet ID: " << packet.id << "\n";

                switch (state)
                {
                case ConnectionState::Handshaking:
                    state = ConnectionState::Status;
                    break;
                case ConnectionState::Status:
                    if (packet.id == 0x00)
                    {
                        std::string json = R"({"version":{"name":"1.7.10","protocol":5},"players":{"max":5,"online":0},"description":{"text":"My Server"}})";
                        auto jsonLength = mc::protocol::encodeVarInt(json.size());
                        std::vector<uint8_t> data;
                        data.insert(data.end(), jsonLength.begin(), jsonLength.end());
                        data.insert(data.end(), json.begin(), json.end());

                        mc::protocol::Packet paketok(0b00000000, data);
                        auto bytes = paketok.serialize();

                        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
                    }
                    if (packet.id == 0x01)
                    { /* ping response */
                    }
                    break;
                }
            }
        }
    }
}

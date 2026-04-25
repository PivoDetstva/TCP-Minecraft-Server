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

        std::cout << "Raw bytes: ";
        for (auto b : buffer_)
        {
            std::cout << std::hex << (int)b << " ";
        }
        std::cout << std::dec << "\n";

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
            return false;
        }

        std::span<const uint8_t> packetSpan(buffer_.data(), totalSize);
        out = mc::protocol::Packet::deserialize(packetSpan);

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
                    if (packet.data.back() == 0x01)
                    {
                        state = ConnectionState::Status;
                    }
                    else if (packet.data.back() == 0x02)
                    {
                        state = ConnectionState::Login;
                    }
                    break;
                case ConnectionState::Status:
                    if (packet.id == 0x00)
                    {
                        std::string json = R"({"version":{"name":"1.7.10","protocol":5},"players":{"max":5,"online":0,"sample":[]},"description":"KarumanAdventures"})";
                        auto jsonLength = mc::protocol::encodeVarInt(json.size());
                        std::vector<uint8_t> data;
                        data.insert(data.end(), jsonLength.begin(), jsonLength.end());
                        data.insert(data.end(), json.begin(), json.end());

                        mc::protocol::Packet paketok(0b00000000, data);
                        auto bytes = paketok.serialize();

                        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
                    }
                    if (packet.id == 0x01)
                    {
                        mc::protocol::Packet response(0x01, packet.data);
                        auto answer = response.serialize();
                        send(socketFd_, answer.data(), answer.size(), MSG_NOSIGNAL);
                    }
                    break;
                case ConnectionState::Login:
                    if (packet.id == 0x00)
                    {
                        auto lengthResult = mc::protocol::decodeVarInt(packet.data);
                        std::string username(
                            packet.data.begin() + lengthResult.bytesRead,
                            packet.data.begin() + lengthResult.bytesRead + lengthResult.value);
                        std::cout << "Player joining: " << username << "\n";

                        std::string uuid = "00000000-0000-0000-0000-000000000000";
                        std::vector<uint8_t> data;

                        auto uuidLen = mc::protocol::encodeVarInt(uuid.size());
                        data.insert(data.end(), uuidLen.begin(), uuidLen.end());
                        data.insert(data.end(), uuid.begin(), uuid.end());
                        auto usernameLen = mc::protocol::encodeVarInt(username.size());
                        data.insert(data.end(), usernameLen.begin(), usernameLen.end());
                        data.insert(data.end(), username.begin(), username.end());

                        mc::protocol::Packet loginSuccess(0x02, data);
                        auto bytes = loginSuccess.serialize();
                        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
                    }
                    break;
                }
            }
        }
    }
}

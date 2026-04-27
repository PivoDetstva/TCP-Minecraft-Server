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
    void TcpClient::writeInt32(std::vector<uint8_t> &data, int32_t value)

    {
        data.push_back((value >> 24) & 0xFF);
        data.push_back((value >> 16) & 0xFF);
        data.push_back((value >> 8) & 0xFF);
        data.push_back((value >> 0) & 0xFF);
    }
    void TcpClient::writeInt64(std::vector<uint8_t> &data, int64_t value)
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
    void TcpClient::writeDouble(std::vector<uint8_t> &data, double value)
    {
        uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits)); // reinterpret double bytes as uint64
        writeInt64(data, static_cast<int64_t>(bits));
    }

    void TcpClient::writeFloat(std::vector<uint8_t> &data, float value)
    {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits)); // reinterpret float bytes as uint32
        writeInt32(data, static_cast<int32_t>(bits));
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

                        std::vector<uint8_t> data;
                        // UUID with dashes — VarInt string
                        std::string uuid = "00000000-0000-0000-0000-000000000000";
                        auto uuidLen = mc::protocol::encodeVarInt(uuid.size());
                        data.insert(data.end(), uuidLen.begin(), uuidLen.end());
                        data.insert(data.end(), uuid.begin(), uuid.end());

                        // username — VarInt string
                        auto usernameLen = mc::protocol::encodeVarInt(username.size());
                        data.insert(data.end(), usernameLen.begin(), usernameLen.end());
                        data.insert(data.end(), username.begin(), username.end());
                        mc::protocol::Packet loginSuccess(0x02, data);
                        auto bytes = loginSuccess.serialize();
                        std::cout << "LoginSuccess bytes: ";
                        for (auto b : bytes)
                        {
                            std::cout << std::hex << (int)b << " ";
                        }
                        std::cout << std::dec << "\n";

                        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
                        usleep(100000);

                        state = ConnectionState::Play;

                        std::vector<uint8_t> joinData;
                        writeInt32(joinData, 1);

                        joinData.push_back(0); // gamemode: survival
                        joinData.push_back(0); // dimension: overworld
                        joinData.push_back(0); // difficulty: peaceful
                        joinData.push_back(5); // max players
                        std::string levelType = "default";
                        auto levelLen = mc::protocol::encodeVarInt(levelType.size());
                        joinData.insert(joinData.end(), levelLen.begin(), levelLen.end());
                        joinData.insert(joinData.end(), levelType.begin(), levelType.end());

                        mc::protocol::Packet joinGame(0x01, joinData);
                        auto joinBytes = joinGame.serialize();
                        std::cout << "JoinGame bytes: ";
                        for (auto b : joinBytes)
                        {
                            std::cout << std::hex << (int)b << " ";
                        }
                        std::cout << std::dec << "\n";
                        send(socketFd_, joinBytes.data(), joinBytes.size(), MSG_NOSIGNAL);

                        std::vector<uint8_t> spawnData;

                        writeInt32(spawnData, 0);  // X
                        writeInt32(spawnData, 64); // Y
                        writeInt32(spawnData, 0);  // Z

                        mc::protocol::Packet spawnPos(0x05, spawnData);
                        auto spawnBytes = spawnPos.serialize();
                        send(socketFd_, spawnBytes.data(), spawnBytes.size(), MSG_NOSIGNAL);

                        std::vector<uint8_t> posLookData;
                        writeDouble(posLookData, 0.0);  // X
                        writeDouble(posLookData, 64.0); // Y (высота ног)
                        writeDouble(posLookData, 0.0);  // Z
                        writeFloat(posLookData, 0.0f);  // Yaw
                        writeFloat(posLookData, 0.0f);  // Pitch
                        posLookData.push_back(1);       // On Ground

                        mc::protocol::Packet posLook(0x08, posLookData);
                        auto posLookBytes = posLook.serialize();
                        send(socketFd_, posLookBytes.data(), posLookBytes.size(), MSG_NOSIGNAL);
                    }
                    break;
                }
            }
        }
    }
}

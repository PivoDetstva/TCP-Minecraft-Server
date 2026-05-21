#include "client.hpp"
namespace mc::network
{
    TcpClient::TcpClient(int socketFd, World &world)
        : socketFd_(socketFd), playerX_(0.0), playerY_(64.0), playerZ_(0.0), lastChunkX_(0), lastChunkZ_(0)
    {
    }

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
            return false;
        }

        std::span<const uint8_t> packetSpan(buffer_.data(), totalSize);
        out = mc::protocol::Packet::deserialize(packetSpan);

        buffer_.erase(buffer_.begin(), buffer_.begin() + totalSize);

        return true;
    }
    void TcpClient::sendChunk(int chunkX, int chunkZ)
    {
        Chunk loaded = mc::world::loadChunk(chunkX, chunkZ);
        Chunk *chunk = &loaded;
        auto compressed = chunk->serialize();

        std::vector<uint8_t> data;
        mc::helper::writeInt32(data, chunkX);            // chunk X
        mc::helper::writeInt32(data, chunkZ);            // chunk Z
        data.push_back(1);                               // ground up continuous
        mc::helper::writeInt16(data, 0xFFFF);            // primary bitmap
        mc::helper::writeInt16(data, 0x0000);            // add bitmap
        mc::helper::writeInt32(data, compressed.size()); // compressed size
        data.insert(data.end(), compressed.begin(), compressed.end());

        mc::protocol::Packet chunkPacket(0x21, data);
        auto bytes = chunkPacket.serialize();
        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
    }
    void TcpClient::sendChunksAround(int chunkX, int chunkZ)
    {
        for (int x = chunkX - 3; x <= chunkX + 3; x++)
        {
            for (int z = chunkZ - 3; z <= chunkZ + 3; z++)
            {
                std::cout << "send chunk triggered\n";
                sendChunk(x, z);
            }
        }
    }
    void TcpClient::sendTabListEntry(std::string_view name, bool online, int16_t ping)
    {
        std::vector<uint8_t> data;
        mc::helper::writeString(data, name);
        data.push_back(online ? 1 : 0);

        uint16_t netPing = htons(ping);
        uint8_t *pingBytes = reinterpret_cast<uint8_t *>(&netPing);
        data.push_back(pingBytes[0]);
        data.push_back(pingBytes[1]);

        mc::protocol::Packet tabPacket(0x38, data);
        auto bytes = tabPacket.serialize();
        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
    }
    void TcpClient::sendFullInventory()
    {
        std::vector<uint8_t> data;
        data.push_back(0); // Window ID 0 (Inventory)

        uint16_t count = 45;
        data.push_back((count >> 8) & 0xFF);
        data.push_back(count & 0xFF);

        for (int i = 0; i < 45; ++i)
        {
            if (i == 36) // this logic kinda sucks, need to change
            {
                data.push_back(0x01);
                data.push_back(0x08); // ID
                data.push_back(64);   // Count
                data.push_back(0x00);
                data.push_back(0x00); // Damage
                data.push_back(0xFF);
                data.push_back(0xFF); // NBT length -1
            }
            else
            {
                data.push_back(0xFF);
                data.push_back(0xFF);
            }
        }

        mc::protocol::Packet invPacket(0x30, data);
        auto bytes = invPacket.serialize();
        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
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

                        player_ = std::make_unique<mc::logic::Player>(username);
                        std::cout << "Created player object for: " << player_->username << "\n";

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

                        send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);

                        state = ConnectionState::Play;

                        std::vector<uint8_t> joinData;
                        mc::helper::writeInt32(joinData, 1);

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

                        sendTabListEntry(player_->username, 1, 50);

                        std::vector<uint8_t> spawnData;

                        mc::helper::writeInt32(spawnData, 0);  // X
                        mc::helper::writeInt32(spawnData, 64); // Y
                        mc::helper::writeInt32(spawnData, 0);  // Z

                        mc::protocol::Packet spawnPos(0x05, spawnData);
                        auto spawnBytes = spawnPos.serialize();
                        send(socketFd_, spawnBytes.data(), spawnBytes.size(), MSG_NOSIGNAL);

                        std::vector<uint8_t> posLookData;
                        mc::helper::writeDouble(posLookData, 0.0);  // X
                        mc::helper::writeDouble(posLookData, 64.0); // Y
                        mc::helper::writeDouble(posLookData, 0.0);  // Z
                        mc::helper::writeFloat(posLookData, 0.0f);  // Yaw
                        mc::helper::writeFloat(posLookData, 0.0f);  // Pitch
                        posLookData.push_back(1);                   // On Ground

                        mc::protocol::Packet posLook(0x08, posLookData);
                        auto posLookBytes = posLook.serialize();
                        send(socketFd_, posLookBytes.data(), posLookBytes.size(), MSG_NOSIGNAL);

                        sendFullInventory();

                        for (int x = -3; x <= 3; x++)
                        {
                            for (int z = -3; z <= 3; z++)
                            {
                                sendChunk(x, z);
                            }
                        }

                        std::vector<uint8_t> initialKA;
                        mc::helper::writeInt32(initialKA, 12345);
                        mc::protocol::Packet firstKA(0x00, initialKA);
                        auto kaBytes = firstKA.serialize();
                        send(socketFd_, kaBytes.data(), kaBytes.size(), MSG_NOSIGNAL);
                    }
                    break;
                case ConnectionState::Play:
                    switch (packet.id)
                    {
                    case 0x00:
                    {
                        // Keep Alive — send it straight back
                        mc::protocol::Packet keepAlive(0x00, packet.data);
                        auto kaBytes = keepAlive.serialize();
                        send(socketFd_, kaBytes.data(), kaBytes.size(), MSG_NOSIGNAL);
                        break;
                    }
                    case 0x01:
                    {
                        // chat receive packet
                        size_t offset = 0;
                        auto msg = mc::helper::readString(packet.data, offset);
                        if (msg.has_value())
                        {
                            std::string jsonReply = R"({"text": "<", "color": "gray", "extra": [)"
                                                    R"({"text": ")" +
                                                    player_->username +
                                                    R"(", "color": "gold"}, )"
                                                    R"({"text": "> ", "color": "gray"}, )"
                                                    R"({"text": ")" +
                                                    *msg +
                                                    R"(", "color": "white"}]})";
                            std::vector<uint8_t> replyData;
                            mc::helper::writeString(replyData, jsonReply);

                            mc::protocol::Packet chatPacket(0x02, replyData);
                            auto bytes = chatPacket.serialize();
                            send(socketFd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
                        }
                        else
                        {
                            std::cerr << "[Protocol Error] Failed to read chat: " << msg.error() << std::endl;
                        }
                        break;
                    }
                    case 0x03:
                    {
                        // player standing on the groung, that's it.
                        size_t offset = 0;
                        bool onGround = mc::helper::readBoolean(packet.data, offset);
                        break;
                    }
                    case 0x04:
                    {
                        // movement.
                        size_t offset = 0;
                        double x = mc::helper::readDouble(packet.data, offset);
                        double y = mc::helper::readDouble(packet.data, offset);
                        double stance = mc::helper::readDouble(packet.data, offset);
                        double z = mc::helper::readDouble(packet.data, offset);
                        bool onGround = mc::helper::readBoolean(packet.data, offset);
                        int currentChunkX = (int)floor(x) >> 4;
                        int currentChunkZ = (int)floor(z) >> 4;

                        if (currentChunkX != lastChunkX_ || currentChunkZ != lastChunkZ_)
                        {
                            lastChunkX_ = currentChunkX;
                            lastChunkZ_ = currentChunkZ;

                            sendChunksAround(currentChunkX, currentChunkZ);
                        }
                        playerX_ = x;
                        playerY_ = y;
                        playerZ_ = z;
                        std::cout << "4.Player moved to: " << x << " " << y << " " << z << "\n";
                        break;
                    }
                    case 0x06:
                    {
                        // movement and looking position
                        size_t offset = 0;
                        double x = mc::helper::readDouble(packet.data, offset);
                        double y = mc::helper::readDouble(packet.data, offset);
                        double stance = mc::helper::readDouble(packet.data, offset);
                        double z = mc::helper::readDouble(packet.data, offset);
                        float yaw = mc::helper::readFloat(packet.data, offset);
                        float pitch = mc::helper::readFloat(packet.data, offset);
                        bool onGround = mc::helper::readBoolean(packet.data, offset);
                        int currentChunkX = (int)floor(x) >> 4;
                        int currentChunkZ = (int)floor(z) >> 4;

                        if (currentChunkX != lastChunkX_ || currentChunkZ != lastChunkZ_)
                        {
                            lastChunkX_ = currentChunkX;
                            lastChunkZ_ = currentChunkZ;

                            sendChunksAround(currentChunkX, currentChunkZ);
                        }
                        playerX_ = x;
                        playerY_ = y;
                        playerZ_ = z;
                        std::cout << "6.Player moved to: " << x << " " << y << " " << z << "\n";
                        std::cout << "Player looked: " << yaw << " " << pitch << "\n";
                        break;
                    }
                    default:
                    {
                        std::cout << "Unknown packet in Play state: ID " << (int)packet.id << "\n";
                        // all packets to deal with later.
                        // 5 - mouse actions.
                        // 11 animation packet.
                        // 13 player abilities
                        // 22 client settings
                        break;
                    }
                    }
                    break;
                }
            }
        }
    }
}

#include "NBT.hpp"
#include "../include/world.hpp"
#include <stdexcept>

namespace mc::world
{

    // Payload reader (used by readTag)
    static void readPayload(NBTTag &tag, const std::vector<uint8_t> &data, size_t &offset)
    {
        switch (tag.type)
        {
        case TagType::Byte:
            tag.value = static_cast<int8_t>(data[offset++]);
            break;

        case TagType::Short:
            tag.value = static_cast<int16_t>(mc::helper::readShort(data, offset));
            break;

        case TagType::Int:
            tag.value = static_cast<int32_t>(mc::helper::readUint32(data, offset));
            break;

        case TagType::Long:
            tag.value = static_cast<int64_t>(mc::helper::readUint64(data, offset));
            break;

        case TagType::Float:
            tag.value = mc::helper::readFloat(data, offset);
            break;

        case TagType::Double:
            tag.value = mc::helper::readDouble(data, offset);
            break;

        case TagType::ByteArray:
        {
            int32_t len = static_cast<int32_t>(mc::helper::readUint32(data, offset));
            std::vector<int8_t> bytes(data.begin() + offset,
                                      data.begin() + offset + len);
            offset += len;
            tag.value = bytes;
            break;
        }

        case TagType::String:
        {
            uint16_t len = mc::helper::readShort(data, offset);
            std::string str(data.begin() + offset, data.begin() + offset + len);
            offset += len;
            tag.value = str;
            break;
        }

        case TagType::List:
        {
            TagType elementType = static_cast<TagType>(data[offset++]);
            int32_t count = static_cast<int32_t>(mc::helper::readUint32(data, offset));
            std::vector<NBTTag> items;
            items.reserve(count);
            for (int i = 0; i < count; ++i)
            {
                NBTTag item;
                item.type = elementType;
                readPayload(item, data, offset);
                items.push_back(std::move(item));
            }
            tag.value = std::move(items);
            break;
        }

        case TagType::Compound:
        {
            std::map<std::string, NBTTag> children;
            while (true)
            {
                NBTTag child = readTag(data, offset);
                if (child.type == TagType::End)
                    break;
                children[child.name] = std::move(child);
            }
            tag.value = std::move(children);
            break;
        }

        case TagType::IntArray:
        {
            int32_t len = static_cast<int32_t>(mc::helper::readUint32(data, offset));
            std::vector<int32_t> ints;
            ints.reserve(len);
            for (int i = 0; i < len; ++i)
                ints.push_back(static_cast<int32_t>(mc::helper::readUint32(data, offset)));
            tag.value = std::move(ints);
            break;
        }

        default:
            break;
        }
    }

    NBTTag readTag(const std::vector<uint8_t> &data, size_t &offset)
    {
        NBTTag tag;
        tag.type = static_cast<TagType>(data[offset++]);
        if (tag.type == TagType::End)
            return tag;

        uint16_t nameLen = mc::helper::readShort(data, offset);
        tag.name = std::string(data.begin() + offset,
                               data.begin() + offset + nameLen);
        offset += nameLen;

        readPayload(tag, data, offset);
        return tag;
    }

    void writeTag(std::vector<uint8_t> &out, const NBTTag &tag, bool writeHeader)
    {
        if (writeHeader)
        {
            out.push_back(static_cast<uint8_t>(tag.type));
            if (tag.type == TagType::End)
                return;

            // Name: 2-byte big-endian length + UTF-8 bytes
            mc::helper::writeInt16(out, static_cast<uint16_t>(tag.name.size()));
            out.insert(out.end(), tag.name.begin(), tag.name.end());
        }

        switch (tag.type)
        {
        case TagType::End:
            break;

        case TagType::Byte:
            out.push_back(static_cast<uint8_t>(std::get<int8_t>(tag.value)));
            break;

        case TagType::Short:
            mc::helper::writeInt16(out, static_cast<uint16_t>(std::get<int16_t>(tag.value)));
            break;

        case TagType::Int:
            mc::helper::writeInt32(out, std::get<int32_t>(tag.value));
            break;

        case TagType::Long:
            mc::helper::writeInt64(out, std::get<int64_t>(tag.value));
            break;

        case TagType::Float:
        {
            float f = std::get<float>(tag.value);
            uint32_t bits;
            std::memcpy(&bits, &f, 4);
            mc::helper::writeInt32(out, static_cast<int32_t>(bits));
            break;
        }

        case TagType::Double:
        {
            double d = std::get<double>(tag.value);
            uint64_t bits;
            std::memcpy(&bits, &d, 8);
            mc::helper::writeInt64(out, static_cast<int64_t>(bits));
            break;
        }

        case TagType::ByteArray:
        {
            const auto &bytes = std::get<std::vector<int8_t>>(tag.value);
            mc::helper::writeInt32(out, static_cast<int32_t>(bytes.size()));
            for (int8_t b : bytes)
                out.push_back(static_cast<uint8_t>(b));
            break;
        }

        case TagType::String:
        {
            const auto &str = std::get<std::string>(tag.value);
            mc::helper::writeInt16(out, static_cast<uint16_t>(str.size()));
            out.insert(out.end(), str.begin(), str.end());
            break;
        }

        case TagType::List:
        {
            const auto &items = std::get<std::vector<NBTTag>>(tag.value);
            TagType elemType = items.empty() ? TagType::End : items[0].type;
            out.push_back(static_cast<uint8_t>(elemType));
            mc::helper::writeInt32(out, static_cast<int32_t>(items.size()));
            for (const auto &item : items)
                writeTag(out, item, false);
            break;
        }

        case TagType::Compound:
        {
            const auto &children = std::get<std::map<std::string, NBTTag>>(tag.value);
            for (const auto &[key, child] : children)
                writeTag(out, child, true);
            // Terminate with Tag::End
            out.push_back(static_cast<uint8_t>(TagType::End));
            break;
        }

        case TagType::IntArray:
        {
            const auto &ints = std::get<std::vector<int32_t>>(tag.value);
            mc::helper::writeInt32(out, static_cast<int32_t>(ints.size()));
            for (int32_t v : ints)
                mc::helper::writeInt32(out, v);
            break;
        }
        }
    }

    NBTTag makeByte(const std::string &name, int8_t value)
    {
        NBTTag t;
        t.type = TagType::Byte;
        t.name = name;
        t.value = value;
        return t;
    }
    NBTTag makeShort(const std::string &name, int16_t value)
    {
        NBTTag t;
        t.type = TagType::Short;
        t.name = name;
        t.value = value;
        return t;
    }
    NBTTag makeInt(const std::string &name, int32_t value)
    {
        NBTTag t;
        t.type = TagType::Int;
        t.name = name;
        t.value = value;
        return t;
    }
    NBTTag makeLong(const std::string &name, int64_t value)
    {
        NBTTag t;
        t.type = TagType::Long;
        t.name = name;
        t.value = value;
        return t;
    }
    NBTTag makeString(const std::string &name, const std::string &value)
    {
        NBTTag t;
        t.type = TagType::String;
        t.name = name;
        t.value = value;
        return t;
    }
    NBTTag makeByteArray(const std::string &name, std::vector<int8_t> bytes)
    {
        NBTTag t;
        t.type = TagType::ByteArray;
        t.name = name;
        t.value = std::move(bytes);
        return t;
    }
    NBTTag makeIntArray(const std::string &name, std::vector<int32_t> ints)
    {
        NBTTag t;
        t.type = TagType::IntArray;
        t.name = name;
        t.value = std::move(ints);
        return t;
    }
    NBTTag makeCompound(const std::string &name, std::map<std::string, NBTTag> children)
    {
        NBTTag t;
        t.type = TagType::Compound;
        t.name = name;
        t.value = std::move(children);
        return t;
    }
    NBTTag makeList(const std::string &name, TagType /*elementType*/, std::vector<NBTTag> items)
    {
        NBTTag t;
        t.type = TagType::List;
        t.name = name;
        t.value = std::move(items);
        return t;
    }

    // buildChunkNBT
    //
    // Produces the Anvil (1.7.10) chunk NBT structure that the client
    // expects.  The structure is:
    //
    // TAG_Compound('')               <- root (unnamed in vanilla, but we name it)
    //   TAG_Compound('Level')
    //     TAG_Int('xPos')
    //     TAG_Int('zPos')
    //     TAG_Byte('TerrainPopulated')
    //     TAG_Long('InhabitedTime')
    //     TAG_Long('LastUpdate')
    //     TAG_List('Sections')       <- list of TAG_Compound, one per 16-block Y section
    //       TAG_Compound
    //         TAG_Byte('Y')          <- section index 0-15
    //         TAG_ByteArray('Blocks')     <- 4096 bytes
    //         TAG_ByteArray('Data')       <- 2048 bytes (4-bit metadata)
    //         TAG_ByteArray('BlockLight') <- 2048 bytes
    //         TAG_ByteArray('SkyLight')   <- 2048 bytes
    //     TAG_ByteArray('Biomes')    <- 256 bytes (one per XZ column)
    //     TAG_List('Entities')       <- empty
    //     TAG_List('TileEntities')   <- empty
    //
    // The returned bytes are raw (uncompressed) NBT.
    // RegionFile::writeChunk() will compress and store them.

    std::vector<uint8_t> buildChunkNBT(int chunkX, int chunkZ, const Chunk &chunk)
    {

        std::vector<NBTTag> sections;

        for (int sectionY = 0; sectionY < 16; ++sectionY)
        {

            int baseY = sectionY * 16;

            std::vector<int8_t> blocks(4096);
            std::vector<int8_t> data(2048, 0); // metadata (4-bit pairs)
            std::vector<int8_t> blockLight(2048, 0);
            std::vector<int8_t> skyLight(2048, 0xFF); // full sky light by default

            for (int y = 0; y < 16; ++y)
            {
                for (int z = 0; z < 16; ++z)
                {
                    for (int x = 0; x < 16; ++x)
                    {
                        int worldY = baseY + y;
                        int chunkIdx = (worldY * 16 + z) * 16 + x; // index into Chunk arrays
                        int secIdx = y * 256 + z * 16 + x;         // index inside this section

                        blocks[secIdx] = static_cast<int8_t>(chunk.blocks[chunkIdx]);

                        // 4-bit metadata stored in packed nibble pairs
                        int halfIdx = secIdx / 2;
                        uint8_t nibble = chunk.meta[chunkIdx / 2];
                        uint8_t metaVal = (chunkIdx & 1)
                                              ? ((nibble >> 4) & 0x0F)
                                              : (nibble & 0x0F);

                        if (secIdx & 1)
                            data[halfIdx] = static_cast<int8_t>(
                                (data[halfIdx] & 0x0F) | (metaVal << 4));
                        else
                            data[halfIdx] = static_cast<int8_t>(
                                (data[halfIdx] & 0xF0) | (metaVal & 0x0F));

                        // Block light
                        uint8_t blSrc = chunk.blockLight[chunkIdx / 2];
                        uint8_t blVal = (chunkIdx & 1) ? ((blSrc >> 4) & 0x0F)
                                                       : (blSrc & 0x0F);
                        if (secIdx & 1)
                            blockLight[halfIdx] = static_cast<int8_t>(
                                (blockLight[halfIdx] & 0x0F) | (blVal << 4));
                        else
                            blockLight[halfIdx] = static_cast<int8_t>(
                                (blockLight[halfIdx] & 0xF0) | (blVal & 0x0F));

                        // Sky light
                        uint8_t slSrc = chunk.skyLight[chunkIdx / 2];
                        uint8_t slVal = (chunkIdx & 1) ? ((slSrc >> 4) & 0x0F)
                                                       : (slSrc & 0x0F);
                        if (secIdx & 1)
                            skyLight[halfIdx] = static_cast<int8_t>(
                                (skyLight[halfIdx] & 0x0F) | (slVal << 4));
                        else
                            skyLight[halfIdx] = static_cast<int8_t>(
                                (skyLight[halfIdx] & 0xF0) | (slVal & 0x0F));
                    }
                }
            }

            std::map<std::string, NBTTag> sec;
            sec["Y"] = makeByte("Y", static_cast<int8_t>(sectionY));
            sec["Blocks"] = makeByteArray("Blocks", blocks);
            sec["Data"] = makeByteArray("Data", data);
            sec["BlockLight"] = makeByteArray("BlockLight", blockLight);
            sec["SkyLight"] = makeByteArray("SkyLight", skyLight);

            sections.push_back(makeCompound("", std::move(sec)));
        }

        // Biomes (256 bytes, all plains = 1)
        std::vector<int8_t> biomes(256, 1);

        NBTTag entityList;
        entityList.type = TagType::List;
        entityList.name = "Entities";
        entityList.value = std::vector<NBTTag>{};

        NBTTag tileEntityList;
        tileEntityList.type = TagType::List;
        tileEntityList.name = "TileEntities";
        tileEntityList.value = std::vector<NBTTag>{};

        std::map<std::string, NBTTag> level;
        level["xPos"] = makeInt("xPos", chunkX);
        level["zPos"] = makeInt("zPos", chunkZ);
        level["TerrainPopulated"] = makeByte("TerrainPopulated", 1);
        level["InhabitedTime"] = makeLong("InhabitedTime", 0);
        level["LastUpdate"] = makeLong("LastUpdate", 0);
        level["Sections"] = makeList("Sections", TagType::Compound, std::move(sections));
        level["Biomes"] = makeByteArray("Biomes", biomes);
        level["Entities"] = std::move(entityList);
        level["TileEntities"] = std::move(tileEntityList);

        std::map<std::string, NBTTag> root;
        root["Level"] = makeCompound("Level", std::move(level));

        NBTTag rootTag = makeCompound("", std::move(root));

        std::vector<uint8_t> out;
        writeTag(out, rootTag, true);
        return out;
    }

    // Fills a Chunk from the root NBT tag produced by readTag() on
    // the decompressed bytes from RegionFile::readChunk().
    void parseChunkNBT(const NBTTag &root, Chunk &chunk)
    {
        if (root.type != TagType::Compound)
            throw std::runtime_error("[parseChunkNBT] Root is not a Compound tag");

        const auto &rootMap = std::get<std::map<std::string, NBTTag>>(root.value);

        auto levelIt = rootMap.find("Level");
        if (levelIt == rootMap.end())
            throw std::runtime_error("[parseChunkNBT] Missing 'Level' tag");

        const auto &level = std::get<std::map<std::string, NBTTag>>(levelIt->second.value);

        auto sectionsIt = level.find("Sections");
        if (sectionsIt == level.end())
            return; // empty chunk is valid

        const auto &sections = std::get<std::vector<NBTTag>>(sectionsIt->second.value);

        for (const auto &secTag : sections)
        {
            if (secTag.type != TagType::Compound)
                continue;

            const auto &sec = std::get<std::map<std::string, NBTTag>>(secTag.value);

            auto yIt = sec.find("Y");
            if (yIt == sec.end())
                continue;
            int sectionY = static_cast<int>(std::get<int8_t>(yIt->second.value));
            int baseY = sectionY * 16;

            auto getBytes = [&](const std::string &key) -> const std::vector<int8_t> *
            {
                auto it = sec.find(key);
                if (it == sec.end())
                    return nullptr;
                return &std::get<std::vector<int8_t>>(it->second.value);
            };

            const auto *blocks = getBytes("Blocks");
            const auto *data = getBytes("Data");
            const auto *blockLight = getBytes("BlockLight");
            const auto *skyLight = getBytes("SkyLight");

            for (int y = 0; y < 16; ++y)
            {
                for (int z = 0; z < 16; ++z)
                {
                    for (int x = 0; x < 16; ++x)
                    {
                        int worldY = baseY + y;
                        int chunkIdx = (worldY * 16 + z) * 16 + x;
                        int secIdx = y * 256 + z * 16 + x;

                        if (blocks)
                            chunk.blocks[chunkIdx] = static_cast<uint8_t>((*blocks)[secIdx]);

                        auto unpackNibble = [](const std::vector<int8_t> &arr, int idx) -> uint8_t
                        {
                            uint8_t pair = static_cast<uint8_t>(arr[idx / 2]);
                            return (idx & 1) ? ((pair >> 4) & 0x0F)
                                             : (pair & 0x0F);
                        };

                        if (data && static_cast<size_t>(secIdx / 2) < data->size())
                        {
                            uint8_t nibble = unpackNibble(*data, secIdx);

                            int halfIdx = chunkIdx / 2;
                            if (chunkIdx & 1)
                                chunk.meta[halfIdx] = (chunk.meta[halfIdx] & 0x0F) | (nibble << 4);
                            else
                                chunk.meta[halfIdx] = (chunk.meta[halfIdx] & 0xF0) | (nibble & 0x0F);
                        }

                        if (blockLight && static_cast<size_t>(secIdx / 2) < blockLight->size())
                        {
                            uint8_t nibble = unpackNibble(*blockLight, secIdx);
                            int halfIdx = chunkIdx / 2;
                            if (chunkIdx & 1)
                                chunk.blockLight[halfIdx] = (chunk.blockLight[halfIdx] & 0x0F) | (nibble << 4);
                            else
                                chunk.blockLight[halfIdx] = (chunk.blockLight[halfIdx] & 0xF0) | (nibble & 0x0F);
                        }

                        if (skyLight && static_cast<size_t>(secIdx / 2) < skyLight->size())
                        {
                            uint8_t nibble = unpackNibble(*skyLight, secIdx);
                            int halfIdx = chunkIdx / 2;
                            if (chunkIdx & 1)
                                chunk.skyLight[halfIdx] = (chunk.skyLight[halfIdx] & 0x0F) | (nibble << 4);
                            else
                                chunk.skyLight[halfIdx] = (chunk.skyLight[halfIdx] & 0xF0) | (nibble & 0x0F);
                        }
                    }
                }
            }
        }
    }

}
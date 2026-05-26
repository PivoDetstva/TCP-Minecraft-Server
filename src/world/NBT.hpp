#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include "client.hpp"

enum class TagType : uint8_t
{
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    ByteArray = 7,
    String = 8,
    List = 9,
    Compound = 10,
    IntArray = 11
};

namespace mc::world
{
    struct NBTTag
    {
        TagType type;
        std::string name;

        std::variant<
            int8_t,                        // Byte
            int16_t,                       // Short
            int32_t,                       // Int
            int64_t,                       // Long
            float,                         // Float
            double,                        // Double
            std::vector<int8_t>,           // ByteArray
            std::string,                   // String
            std::vector<NBTTag>,           // List
            std::map<std::string, NBTTag>, // Compound
            std::vector<int32_t>           // IntArray
            >
            value;
    };

    NBTTag readTag(const std::vector<uint8_t> &data, size_t &offset);
    void writeTag(std::vector<uint8_t> &out, const NBTTag &tag, bool writeHeader = true);

    NBTTag makeCompound(const std::string &name, std::map<std::string, NBTTag> children);
    NBTTag makeList(const std::string &name, TagType elementType, std::vector<NBTTag> items);
    NBTTag makeByteArray(const std::string &name, std::vector<int8_t> bytes);
    NBTTag makeIntArray(const std::string &name, std::vector<int32_t> ints);
    NBTTag makeByte(const std::string &name, int8_t value);
    NBTTag makeShort(const std::string &name, int16_t value);
    NBTTag makeInt(const std::string &name, int32_t value);
    NBTTag makeLong(const std::string &name, int64_t value);
    NBTTag makeString(const std::string &name, const std::string &value);

    std::vector<uint8_t> buildChunkNBT(int chunkX, int chunkZ, const Chunk &chunk);
    void parseChunkNBT(const NBTTag &root, Chunk &chunk);

}
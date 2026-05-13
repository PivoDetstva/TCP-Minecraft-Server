// NBT.cpp
#include "NBT.hpp"

namespace mc::world
{
    NBTTag readTag(const std::vector<uint8_t> &data, size_t &offset); // forward declaration

    void readPayload(NBTTag &tag, const std::vector<uint8_t> &data, size_t &offset)
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
            std::vector<int8_t> bytes(data.begin() + offset, data.begin() + offset + len);
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
            for (int i = 0; i < count; i++)
            {
                NBTTag item;
                item.type = elementType;
                readPayload(item, data, offset);
                items.push_back(item);
            }
            tag.value = items;
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
                children[child.name] = child;
            }
            tag.value = children;
            break;
        }

        case TagType::IntArray:
        {
            int32_t len = static_cast<int32_t>(mc::helper::readUint32(data, offset));
            std::vector<int32_t> ints;
            for (int i = 0; i < len; i++)
            {
                ints.push_back(static_cast<int32_t>(mc::helper::readUint32(data, offset)));
            }
            tag.value = ints;
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
        tag.name = std::string(data.begin() + offset, data.begin() + offset + nameLen);
        offset += nameLen;

        readPayload(tag, data, offset);
        return tag;
    }

}
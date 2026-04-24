#include <gtest/gtest.h>
#include "packet.hpp"

using namespace mc::protocol;

// serialize produces correct byte order: length, id, data
TEST(PacketSerialize, CorrectByteOrder)
{
    Packet p;
    p.id = 0x00;
    p.data = {0x01, 0x02, 0x03};

    auto bytes = p.serialize();

    // total length = 1 byte (id varint) + 3 bytes (data) = 4
    // length varint of 4 = just 0x04
    EXPECT_EQ(bytes[0], 0x04); // length
    EXPECT_EQ(bytes[1], 0x00); // id
    EXPECT_EQ(bytes[2], 0x01); // data start
}

// round trip — serialize then deserialize gives back original
TEST(PacketRoundTrip, SerializeDeserialize)
{
    mc::protocol::Packet original;
    original.id = 0x05;
    original.data = {0xAA, 0xBB, 0xCC};

    auto bytes = original.serialize();
    auto restored = mc::protocol::Packet::deserialize(bytes);

    EXPECT_EQ(restored.id, original.id);
    EXPECT_EQ(restored.data, original.data);
}

// empty data payload is valid
TEST(PacketSerialize, EmptyData)
{
    Packet p;
    p.id = 0x01;
    p.data = {};

    auto bytes = p.serialize();
    auto restored = mc::protocol::Packet::deserialize(bytes);

    EXPECT_EQ(restored.id, 0x01);
    EXPECT_EQ(restored.data.size(), 0);
}

// large packet ID needing 2 byte varint
TEST(PacketRoundTrip, LargeId)
{
    Packet original;
    original.id = 300;
    original.data = {0x01};

    auto bytes = original.serialize();
    auto restored = mc::protocol::Packet::deserialize(bytes);

    EXPECT_EQ(restored.id, 300);
}
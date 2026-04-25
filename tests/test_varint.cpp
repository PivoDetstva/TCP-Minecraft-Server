#include <gtest/gtest.h>
#include "varint.hpp"

TEST(VarIntEncode, Zero)
{
    auto result = mc::protocol::encodeVarInt(0);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 0x00);
}

TEST(VarIntEncode, SmallNumber)
{
    auto result = mc::protocol::encodeVarInt(5);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 0b00000101);
}

TEST(VarIntEncode, TwoBytes)
{
    auto result = mc::protocol::encodeVarInt(300);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(static_cast<int>(result[0]), 172);
    EXPECT_EQ(result[1], 0b00000010);
}

TEST(VarIntEncode, MaxValue)
{
    auto result = mc::protocol::encodeVarInt(2147483647);
    EXPECT_EQ(result.size(), 5);
}

TEST(VarIntRoundTrip, SmallNumber)
{
    auto encoded = mc::protocol::encodeVarInt(300);
    auto decoded = mc::protocol::decodeVarInt(encoded);
    EXPECT_EQ(decoded.value, 300);
    EXPECT_EQ(decoded.bytesRead, 2);
}

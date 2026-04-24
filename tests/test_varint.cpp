#include <gtest/gtest.h>
#include "varint.hpp"

// zero is a special case — do/while must handle it
TEST(VarIntEncode, Zero)
{
    auto result = mc::protocol::encodeVarInt(0);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 0x00);
}

// single byte — no continue bit needed
TEST(VarIntEncode, SmallNumber)
{
    auto result = mc::protocol::encodeVarInt(5);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 0b00000101);
}

// first number that needs 2 bytes (128 doesn't fit in 7 bits)
TEST(VarIntEncode, TwoBytes)
{
    auto result = mc::protocol::encodeVarInt(300);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(static_cast<int>(result[0]), 172); // 0b10101100 = 172
    EXPECT_EQ(result[1], 0b00000010);            // continue bit OFF + data
}

// maximum int32 — must produce exactly 5 bytes
TEST(VarIntEncode, MaxValue)
{
    auto result = mc::protocol::encodeVarInt(2147483647);
    EXPECT_EQ(result.size(), 5);
}

// round trip — encode then decode gives back the original
TEST(VarIntRoundTrip, SmallNumber)
{
    auto encoded = mc::protocol::encodeVarInt(300);
    auto decoded = mc::protocol::decodeVarInt(encoded);
    EXPECT_EQ(decoded.value, 300);
    EXPECT_EQ(decoded.bytesRead, 2);
}

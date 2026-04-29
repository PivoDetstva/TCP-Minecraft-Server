#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <zlib.h>
#include <utility>
class Chunk
{
public:
    uint8_t blocks[16 * 256 * 16];    // block types
    uint8_t meta[16 * 256 * 8];       // metadata, 4 bits per block
    uint8_t blockLight[16 * 256 * 8]; // light, 4 bits per block
    uint8_t skyLight[16 * 256 * 8];   // sky light, 4 bits per block

    Chunk();
    std::vector<uint8_t> serialize();
};

class World
{
public:
    Chunk *getChunk(int x, int z);

private:
    std::map<std::pair<int, int>, Chunk> chunks_;
};
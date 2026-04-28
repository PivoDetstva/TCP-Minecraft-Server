#include <zlib.h>
#include <vector>

class Chunk
{
public:
    uint8_t blocks[16 * 256 * 16];
    std::vector<uint8_t> serialize();
};

class World
{
public:
    Chunk *getChunk(int x, int z);
};
#include "world.hpp"
#include "WorldSave.hpp"
Chunk::Chunk()
{
    std::fill(std::begin(blocks), std::end(blocks), 0);
    std::fill(std::begin(meta), std::end(meta), 0);
    std::fill(std::begin(blockLight), std::end(blockLight), 0);
    std::fill(std::begin(skyLight), std::end(skyLight), 0);

    for (int y = 0; y < 4; y++)
    {
        for (int z = 0; z < 16; z++)
        {
            for (int x = 0; x < 16; x++)
            {
                blocks[(y * 16 + z) * 16 + x] = 2;
            }
        }
    }
}

Chunk *World::getChunk(int x, int z)
{
    auto key = std::make_pair(x, z);
    return &chunks_[key];
}
std::vector<uint8_t> Chunk::serialize()
{
    std::vector<uint8_t> raw;

    // insert block types
    raw.insert(raw.end(), blocks, blocks + sizeof(blocks));
    raw.insert(raw.end(), meta, meta + sizeof(meta));
    raw.insert(raw.end(), blockLight, blockLight + sizeof(blockLight));
    raw.insert(raw.end(), skyLight, skyLight + sizeof(skyLight));

    std::vector<uint8_t> compressed(raw.size());
    uLongf compressedSize = compressed.size();

    compress2(compressed.data(), &compressedSize,
              raw.data(), raw.size(),
              Z_DEFAULT_COMPRESSION);

    compressed.resize(compressedSize);

    return compressed;
}

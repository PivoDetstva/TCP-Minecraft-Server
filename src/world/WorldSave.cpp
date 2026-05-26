#include "WorldSave.hpp"

void mc::world::saveChunk(int chunkX, int chunkZ, const Chunk &chunk)
{
    int regionX = chunkX >> 5;
    int regionZ = chunkZ >> 5;

    std::filesystem::create_directories("world/region");
    std::string path = "world/region/r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca";

    std::vector<uint8_t> nbtData = mc::world::buildChunkNBT(chunkX, chunkZ, chunk);

    mc::world::RegionFile rgf(path);
    rgf.writeChunk(chunkX, chunkZ, nbtData);
}
Chunk mc::world::loadChunk(int chunkX, int chunkZ)
{

    int regionX = chunkX >> 5;
    int regionZ = chunkZ >> 5;

    std::string path = "world/region/r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca";

    mc::world::RegionFile rgf(path);
    auto data = rgf.readChunk(chunkX, chunkZ);
    if (data.empty())
    {
        Chunk fresh;
        mc::world::saveChunk(chunkX, chunkZ, fresh);
        return fresh;
    }
    size_t offset = 0;
    auto tag = mc::world::readTag(data, offset);
    Chunk ch;
    mc::world::parseChunkNBT(tag, ch);
    return ch;
}
void World::saveAllChunks()
{
    for (const auto &el : chunks_)
    {
        mc::world::saveChunk(el.first.first, el.first.second, el.second);
    }
}
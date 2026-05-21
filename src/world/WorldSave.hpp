#pragma once
#include "world.hpp"
#include "NBT.hpp"
#include "RegionFile.hpp"
#include <filesystem>

namespace mc::world
{
    void saveChunk(int chunkX, int chunkZ, const Chunk &chunk);
    Chunk loadChunk(int chunkX, int chunkZ);
}
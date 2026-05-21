#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "NBT.hpp"
#include "fstream"
#include <filesystem>

namespace mc::world
{

    class RegionFile
    {
    public:
        explicit RegionFile(const std::string &path);

        std::vector<uint8_t> readChunk(int chunkX, int chunkZ);

        void writeChunk(int chunkX, int chunkZ, const std::vector<uint8_t> &data);

    private:
        std::string path_;

        int chunkIndex(int chunkX, int chunkZ);
    };

}
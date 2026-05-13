// RegionFile.hpp
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

        // read a chunk's NBT data from the file
        // chunkX and chunkZ are WORLD chunk coordinates
        std::vector<uint8_t> readChunk(int chunkX, int chunkZ);

        // write a chunk's NBT data to the file
        void writeChunk(int chunkX, int chunkZ, const std::vector<uint8_t> &data);

    private:
        std::string path_;

        // converts world chunk coords to position inside this file
        int chunkIndex(int chunkX, int chunkZ);
    };

}
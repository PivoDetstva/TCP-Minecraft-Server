#include "RegionFile.hpp"

namespace mc::world
{

    std::vector<uint8_t> RegionFile::readChunk(int chunkX, int chunkZ)
    {
        if (!std::filesystem::exists(path_))
        {
            return {};
        }

        std::ifstream file(path_, std::ios::binary);
        if (!file)
        {
            std::cerr << "Cannot open region file: " << path_ << "\n";
            return {};
        }

        int localX = chunkX & 31;
        int localZ = chunkZ & 31;
        int index = localX + localZ * 32;

        file.seekg(index * 4);
        uint8_t entry[4];
        file.read(reinterpret_cast<char *>(entry), 4);

        uint32_t sectorOffset = (entry[0] << 16) | (entry[1] << 8) | entry[2];
        uint8_t sectorCount = entry[3];

        std::cout << "Chunk at sector " << sectorOffset << "\n";

        if (sectorOffset == 0)
        {
            return {};
        }

        return {};
    }

}
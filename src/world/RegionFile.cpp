#include "RegionFile.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <zlib.h>

namespace mc::world
{

    RegionFile::RegionFile(const std::string &path)
        : path_(path)
    {

        if (!std::filesystem::exists(path_))
        {
            std::ofstream file(path_, std::ios::binary);
            if (!file)
                throw std::runtime_error("Cannot create region file: " + path_);

            // 1024 entries * 4 bytes each = 4096 bytes, written twice
            std::vector<uint8_t> zeros(8192, 0);
            file.write(reinterpret_cast<const char *>(zeros.data()), zeros.size());
        }
    }

    int RegionFile::chunkIndex(int chunkX, int chunkZ)
    {
        // Local position within this .mca file (each file covers 32x32 chunks)
        int localX = ((chunkX % 32) + 32) % 32;
        int localZ = ((chunkZ % 32) + 32) % 32;
        return localX + localZ * 32;
    }

    // readChunk
    // Anvil / McRegion format:
    //   Bytes 0..4095  : location table  (1024 x 4-byte entries)
    //   Bytes 4096..8191: timestamp table (1024 x 4-byte entries)
    //   Bytes 8192+    : chunk data sectors (each sector = 4096 bytes)
    //
    // Location entry layout (4 bytes, big-endian):
    //   [0..2]  offset  – sector index where chunk starts (3 bytes)
    //   [3]     count   – number of 4096-byte sectors used (1 byte)
    //
    // Chunk data layout at sectorOffset * 4096:
    //   [0..3]  length in bytes of the following data (big-endian int32)
    //   [4]     compression type: 1 = GZip, 2 = zlib (deflate)
    //   [5..]   compressed NBT data

    std::vector<uint8_t> RegionFile::readChunk(int chunkX, int chunkZ)
    {
        if (!std::filesystem::exists(path_))
            return {};

        std::ifstream file(path_, std::ios::binary);
        if (!file)
        {
            std::cerr << "[RegionFile] Cannot open: " << path_ << "\n";
            return {};
        }

        int index = chunkIndex(chunkX, chunkZ);

        file.seekg(index * 4);
        uint8_t entry[4];
        file.read(reinterpret_cast<char *>(entry), 4);

        uint32_t sectorOffset = (static_cast<uint32_t>(entry[0]) << 16) | (static_cast<uint32_t>(entry[1]) << 8) | static_cast<uint32_t>(entry[2]);

        if (sectorOffset == 0)
        {
            // Chunk not generated yet
            return {};
        }

        file.seekg(static_cast<std::streamoff>(sectorOffset) * 4096);

        uint8_t header[5];
        file.read(reinterpret_cast<char *>(header), 5);

        uint32_t dataLength = (static_cast<uint32_t>(header[0]) << 24) | (static_cast<uint32_t>(header[1]) << 16) | (static_cast<uint32_t>(header[2]) << 8) | static_cast<uint32_t>(header[3]);
        uint8_t compressionType = header[4];

        if (dataLength == 0)
            return {};

        // dataLength includes the compression-type byte, so actual payload = dataLength - 1
        uint32_t payloadLength = dataLength - 1;

        std::vector<uint8_t> compressed(payloadLength);
        file.read(reinterpret_cast<char *>(compressed.data()), payloadLength);

        std::vector<uint8_t> decompressed;
        decompressed.resize(payloadLength * 8);

        uLongf destLen = static_cast<uLongf>(decompressed.size());
        int ret;

        if (compressionType == 2)
        {
            ret = uncompress(decompressed.data(), &destLen,
                             compressed.data(), static_cast<uLong>(compressed.size()));
        }
        else if (compressionType == 1)
        {
            // GZip – use raw inflate with gzip window bits
            z_stream strm{};
            strm.next_in = compressed.data();
            strm.avail_in = static_cast<uInt>(compressed.size());
            inflateInit2(&strm, 16 + MAX_WBITS); // 16 = gzip mode

            strm.next_out = decompressed.data();
            strm.avail_out = static_cast<uInt>(decompressed.size());
            ret = inflate(&strm, Z_FINISH);
            destLen = strm.total_out;
            inflateEnd(&strm);

            if (ret != Z_STREAM_END && ret != Z_OK)
            {
                std::cerr << "[RegionFile] GZip inflate failed: " << ret << "\n";
                return {};
            }
        }
        else
        {
            std::cerr << "[RegionFile] Unknown compression type: "
                      << static_cast<int>(compressionType) << "\n";
            return {};
        }

        if (compressionType == 2 && ret != Z_OK)
        {
            std::cerr << "[RegionFile] zlib uncompress failed: " << ret << "\n";
            return {};
        }

        decompressed.resize(destLen);
        return decompressed; // raw NBT bytes, ready for mc::world::readTag()
    }

    // writeChunk
    // Compresses `data` with zlib and writes it back into the region file,
    // updating the location table entry.
    void RegionFile::writeChunk(int chunkX, int chunkZ, const std::vector<uint8_t> &data)
    {
        // 1. Compress the raw NBT data
        uLongf compressedSize = compressBound(static_cast<uLong>(data.size()));
        std::vector<uint8_t> compressed(compressedSize);

        int ret = compress2(compressed.data(), &compressedSize,
                            data.data(), static_cast<uLong>(data.size()),
                            Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK)
            throw std::runtime_error("[RegionFile] zlib compress failed");

        compressed.resize(compressedSize);

        // 2. Build the 5-byte chunk header  (length includes the type byte)
        uint32_t payloadLen = static_cast<uint32_t>(compressed.size());
        uint32_t storedLen = payloadLen + 1; // +1 for compression-type byte

        uint8_t chunkHeader[5];
        chunkHeader[0] = (storedLen >> 24) & 0xFF;
        chunkHeader[1] = (storedLen >> 16) & 0xFF;
        chunkHeader[2] = (storedLen >> 8) & 0xFF;
        chunkHeader[3] = (storedLen >> 0) & 0xFF;
        chunkHeader[4] = 2; // zlib compression

        // 3. Calculate how many 4096-byte sectors we need
        uint32_t totalBytes = 5 + payloadLen;
        uint32_t sectorsNeeded = (totalBytes + 4095) / 4096;

        // 4. Open the file for read+write
        std::fstream file(path_, std::ios::binary | std::ios::in | std::ios::out);
        if (!file)
            throw std::runtime_error("[RegionFile] Cannot open for writing: " + path_);

        // Find the current end of file to append new sector there
        file.seekg(0, std::ios::end);
        std::streamoff fileSize = file.tellg();

        // First usable sector is 2 (sectors 0 and 1 are the header tables)
        uint32_t firstDataSector = 2;
        uint32_t appendSector = static_cast<uint32_t>(fileSize / 4096);
        if (appendSector < firstDataSector)
            appendSector = firstDataSector;

        // 5. Write chunk data at the end
        file.seekp(static_cast<std::streamoff>(appendSector) * 4096);
        file.write(reinterpret_cast<const char *>(chunkHeader), 5);
        file.write(reinterpret_cast<const char *>(compressed.data()), compressed.size());

        // Pad to sector boundary
        uint32_t written = 5 + payloadLen;
        uint32_t padNeeded = (4096 - (written % 4096)) % 4096;
        if (padNeeded > 0)
        {
            std::vector<uint8_t> pad(padNeeded, 0);
            file.write(reinterpret_cast<const char *>(pad.data()), pad.size());
        }

        // 6. Update the location table entry
        int index = chunkIndex(chunkX, chunkZ);
        file.seekp(index * 4);

        uint8_t entry[4];
        entry[0] = (appendSector >> 16) & 0xFF;
        entry[1] = (appendSector >> 8) & 0xFF;
        entry[2] = (appendSector >> 0) & 0xFF;
        entry[3] = static_cast<uint8_t>(sectorsNeeded);
        file.write(reinterpret_cast<const char *>(entry), 4);
    }

}
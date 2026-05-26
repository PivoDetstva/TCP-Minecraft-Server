#pragma once
#include <string>
#include <cstdint>

namespace mc::logic
{
    struct Slot
    {
        int16_t id = -1;
        int8_t count = 0;
        int16_t damage = 0;
    };

    class Player
    {
    public:
        std::string username;
        double x, y, z;
        float yaw, pitch;
        int16_t ping = 0;
        std::array<Slot, 45> inventory;
    };
}
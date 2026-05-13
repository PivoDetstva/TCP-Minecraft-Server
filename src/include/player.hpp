#pragma once
#include <string>
#include <cstdint>

namespace mc::logic
{
    class Player
    {
    public:
        std::string username;
        double x, y, z;
        float yaw, pitch;
        int16_t ping = 0;
    };
}
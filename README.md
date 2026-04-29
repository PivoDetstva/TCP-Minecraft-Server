# 1.7.10 Local Minecraft TCP Server. 

This code is a bunch of protocols that will connect with Minecraft client, especially on version 1.7.10. 
Later to be done to accept forge packets.

### Compiled using CMake 3.22

The server works on the modern C++ 20 and C++23 features like std::span, std::bit_cast and std::byteswap with std::endian.

##Works specifically on Linux. 
Such headers as <sys/socket.h> is a linux especiality. This code can't be executed on Windows or another system, except if Docker container used, can be added later. 

The basic purpose of the server is to run it on another device, except the machine that will run Minecraft. Reason: more optimization and less resources spent. 

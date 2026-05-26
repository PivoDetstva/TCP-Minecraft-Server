# 1.7.10 Local Minecraft TCP Server. 

This code is a bunch of protocols that will connect with Minecraft client, especially on version 1.7.10. 
Later to be done to accept forge packets.

### Compiled using CMake 3.22

## Works specifically on Linux. 
Such headers as <sys/socket.h> is a linux especiality. This code can't be executed on Windows or another system, except if Docker container used, can be added later. 

The basic purpose of the server is to run it on another device, except the machine that will run Minecraft. Reason: more optimization and less resources spent. 

*Basically it will require using a virtual ip or creating a network with friends using Radmin or any other service, unless your IPS is not blocking a port forwarding. 


## What Works

- Full Minecraft 1.7.10 handshake and login sequence
- Server list ping with custom MOTD
- Player login with UUID assignment  
- World spawn with chunk data (7x7 chunk area)
- Real-time packet handling: movement, chat, keep-alive
- VarInt encoding/decoding per Minecraft protocol spec
- zlib chunk compression
- Big-endian binary serialization with C++23 std::byteswap
- Chunk persistence via Anvil region file format (.mca)
- Block breaking and placement
- World saves on server shutdown via "stop" console command
- Player inventory display

## Technical Highlights

- Implements the complete Minecraft connection state machine
  (Handshaking → Status/Login → Play)
- Partial packet buffering — handles TCP stream fragmentation
- Compile-time endianness detection with std::endian
- Clean namespace hierarchy: mc::network, mc::protocol, mc::helper
- Uses std::span, std::bit_cast, std::byteswap (C++20/23)
- NBT binary format implementation (read/write) for chunk serialization
- Anvil region file format (.mca) with zlib sector compression

## Limitations (In Progress)

- Single-threaded: handles one client at a time(not anymore for server, but still only one client)
- World is flat dirt (no terrain generation yet)
- No persistence: world resets on restart(it does have saving logic, but haven't tested it properly yet)

#pragma once
#include <iostream>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <cstring>
#include <unistd.h>
#include "client.hpp"
#include "world.hpp"
#include <thread>
#include <atomic>
namespace mc::network
{
    class Server
    {
    private:
        uint16_t port_;
        int socketFd_;
        World world_;
        std::atomic<bool> running_;
        std::thread consoleThread_;
        void consoleLoop();

    public:
        Server() = default;
        explicit Server(uint16_t port);
        ~Server();

        void start();
        void stop();
    };
}
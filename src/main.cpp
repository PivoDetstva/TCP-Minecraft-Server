#include <iostream>
#include "server.hpp"
int main()
{
    mc::network::Server server;
    server.start();
    return 0;
}
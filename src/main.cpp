#include <iostream>
#include "server.hpp"
int main()
{
    mc::network::Server server(25565);
    server.start();
    return 0;
}
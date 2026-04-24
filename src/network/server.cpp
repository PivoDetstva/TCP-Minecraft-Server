#include "server.hpp"
namespace mc::network
{

    Server::Server(uint16_t port) : port_(port), socketFd_(-1) {}

    Server::~Server()
    {
        stop();
    }

    void mc::network::Server::start()
    {
        if (socketFd_ < 0)
        {
            std::cerr << "socket hasn't created\n";
            return;
        }
        socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        std::cout << "Socket created\n";

        struct sockaddr_in adress;
        std::memset(&adress, 0, sizeof(adress));

        adress.sin_family = AF_INET;
        adress.sin_addr.s_addr = INADDR_ANY;
        adress.sin_port = htons(port_);

        if (bind(socketFd_, (struct sockaddr *)&adress, sizeof(adress)) < 0)
        {
            std::cerr << "Bind failed, port might be busy" << std::endl;
            return;
        }
        std::cout << "Build succesful on port " << port_ << "\n";

        if (listen(socketFd_, 3) < 0)
        {
            std::cerr << "Listening failed" << std::endl;
            return;
        }
        std::cout << "Listening on port " << ntohs(adress.sin_port) << "\n";

        struct sockaddr_in client_adress;
        socklen_t client_len = sizeof(client_adress);

        std::cout << "Waiting for connection..." << std::endl;

        while (true)
        {
            int clientFd = accept(socketFd_, nullptr, nullptr);
            if (clientFd < 0)
            {
                std::cerr << "Accept failed\n";
                continue;
            }

            std::cout << "Client connected\n";
            TcpClient client(clientFd);
            client.handle();
            while (true)
            {
                std::vector<uint8_t> buffer(1024);
                int bytes_received = read(clientFd, buffer.data(), buffer.size());
                std::cout << bytes_received;

                if (bytes_received <= 0)
                {
                    std::cout << "Client Disconnected";
                    break;
                }

                std::string cmd(buffer.begin(), buffer.begin() + bytes_received); // how to make it into hex? minecraft protocol can cut it in output
                std::cout << "DEBUG MSG: Received " << bytes_received << " bytes: [" << cmd << "]\n";
                std::string response;

                send(clientFd, response.data(), response.size(), MSG_NOSIGNAL);
            }
            close(clientFd);
        }
        close(socketFd_);
    }

    void mc::network::Server::stop()
    {
        if (socketFd_ >= 0)
        {
            close(socketFd_);
            socketFd_ = -1;
        }
    }

}
#include "net.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

constexpr uint32_t port = 8000;

void Server()
{
    auto server = net::ENetServer::Create();
    server->Start(port);
    server->Poll();
}

void ClientInput(uint8_t playerId)
{
    auto client = net::ENetClient::Create(playerId);
    if (client->Connect("localhost", port) != 0) {
        return;
    }

    while (true) {
        std::string message;
        float x, y;
        std::cout << "Message:>";
        std::cin >> message;
        if (message == "q") {
            return;
        }
        std::cout << "x:>";
        std::cin >> x;
        std::cout << "y:>";
        std::cin >> y;
        client->SendPosition(x, y);
    }
}

void ClientSendPos(uint8_t playerId, float x, float y)
{
    auto client = net::ENetClient::Create(playerId);
    if (client->Connect("localhost", port) != 0) {
        return;
    }
    client->SendPosition(x, y);
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    std::thread server_thread(Server);
    std::thread client_thread1(ClientSendPos, 1, 1.0f, 1.0f);
    std::thread client_thread2(ClientSendPos, 2, 2.0f, 2.0f);

    server_thread.join();
    return 0;
}
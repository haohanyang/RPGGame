#include "net.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
constexpr uint32_t port = 8000;


void Server() {
    net::ENetServer server;
    server.Start(port);
    server.Poll();
}

void ClientInput() {
    net::ENetClient client;
    if (client.Connect("localhost", port, 0x2) != 0) {
        return;
    }

    while (true) {
        uint8_t user;
        float x, y;
        std::cout << "Message:>";
        std::cin >> user;
        if (user == 0) {
            return;
        }
        std::cout << "x:>";
        std::cin >> x;
        std::cout << "y:>";
        std::cin >> y;
        client.SendPosition(Player_Player1, x, y);
    }
}

void ClientSendPos(uint8_t user, float x, float y) {
    net::ENetClient client;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (client.Connect("localhost", port, int(x)) != 0) {
        return;
    }
    client.SendPosition(Player_Player1, x, y);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}


int main() {
    spdlog::set_level(spdlog::level::debug);
    std::thread server_thread(Server);
    std::thread client_thread1(ClientSendPos,1, 1.0f, 1.0f);
    std::thread client_thread2(ClientSendPos,1, 2.0f, 1.0f);
    std::thread client_thread3(ClientSendPos,2, 3.0f, 1.0f);

    server_thread.join();
    return 0;
}
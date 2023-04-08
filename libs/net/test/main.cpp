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
    if (client.Connect("localhost", port) != 0) {
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
        client.SendPos(message, x, y);
    }
}

void ClientSendPos(const std::string &message, float x, float y) {
    net::ENetClient client;
    if (client.Connect("localhost", port) != 0) {
        return;
    }
    client.SendPos(message, x, y);
}


int main() {
    spdlog::set_level(spdlog::level::debug);
    std::thread server_thread(Server);
    std::thread client_thread1(ClientSendPos,"ms1", 1.0f, 1.0f);
    std::thread client_thread2(ClientSendPos,"ms2", 1.0f, 1.0f);
    std::thread client_thread3(ClientSendPos,"ms3", 1.0f, 1.0f);

    server_thread.join();
    return 0;
}
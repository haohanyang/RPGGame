#include "net.h"

int main() {
    spdlog::set_level(spdlog::level::debug);

    net::ENetServer server;
    server.Start(8000);
    server.Poll();
}
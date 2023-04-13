#include "net.h"

int main()
{
    spdlog::set_level(spdlog::level::debug);

    auto server = net::ENetServer::Create();
    server->Start(8000);
    server->Poll();
}
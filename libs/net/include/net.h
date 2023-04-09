#ifndef NET_H
#define NET_H

#include "enet/enet.h"
#include "spdlog/spdlog.h"
#include "serialize_generated.h"

#include <array>
namespace net {

constexpr uint32_t SERVER_MAX_CONNECTIONS = 2;
constexpr time_t SERVER_TIMEOUT = 50000;

constexpr uint32_t CLIENT_MAX_CONNECTIONS = 1;
constexpr time_t CLIENT_TIMEOUT = 1000;

constexpr uint8_t RELIABLE_CHANNEL = 0;
constexpr uint8_t UNRELIABLE_CHANNEL = 1;
constexpr uint8_t NUM_CHANNELS = 2;

ENetPacket *SerializePos(int user, float x, float y);

class ENetClient {
public:
    static std::shared_ptr<ENetClient> Create();
    ENetClient();
    ~ENetClient();
    bool IsConnected();
    int Connect(const std::string &host, uint32_t port);
    const Position* GetPosition(PlayerNumber player);
    void SendPosition(PlayerNumber player, float x, float y);
private:
    ENetHost *Client;
    ENetPeer *Server;
    void Disconnect();
};


class ENetServer {
public:
    static std::shared_ptr<ENetServer> Create();
    ENetServer();
    ~ENetServer();
    int Start(uint32_t port);
    void Poll();
private:
    ENetHost *Server;
    std::array<ENetPeer*, 2> Clients;
    void Shutdown();
    bool IsServing();
};

}

#endif //NET_H

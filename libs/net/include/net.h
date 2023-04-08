#ifndef NET_H
#define NET_H

#include "enet/enet.h"
#include "spdlog/spdlog.h"
#include "serialize_generated.h"
#include <unordered_map>

namespace net {

constexpr uint32_t SERVER_MAX_CONNECTIONS = 5;
constexpr time_t SERVER_TIMEOUT = 50000;

constexpr uint32_t CLIENT_MAX_CONNECTIONS = 5;
constexpr time_t CLIENT_TIMEOUT = 50000;

constexpr uint8_t RELIABLE_CHANNEL = 0;
constexpr uint8_t UNRELIABLE_CHANNEL = 1;
constexpr uint8_t NUM_CHANNELS = 2;

ENetPacket* Serialize(std::string message, float x, float y);
const Pos* DeSerialize(void *data);

class ENetClient {
public:
    static std::shared_ptr<ENetClient> Create();
    ENetClient();
    ~ENetClient();
    bool IsConnected();
    int Connect(const std::string &host, uint32_t port);
    void Poll();
    void SendPos(const std::string &message, float x, float y);
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
    std::unordered_map<uint32_t, ENetPeer*> Clients;
    void Shutdown();
    bool IsServing();
};

}

#endif //NET_H

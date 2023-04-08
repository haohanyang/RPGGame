#include "net.h"
#include "serialize_generated.h"

namespace net
{

std::shared_ptr<ENetServer> ENetServer::Create()
{
    return std::make_shared<ENetServer>();
}

ENetServer::ENetServer()
    : Server(nullptr)
{
    if (enet_initialize() != 0) {
        spdlog::error("An error occurred while initializing ENet");
    }
}

bool ENetServer::IsServing()
{
    return Server != nullptr;
}

ENetServer::~ENetServer()
{
    Shutdown();
    enet_deinitialize();
}

int ENetServer::Start(uint32_t port)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    Server = enet_host_create(&address, net::SERVER_MAX_CONNECTIONS, net::NUM_CHANNELS, 0, 0);

    if (Server == nullptr) {
        spdlog::error("An error occurred while trying to create an ENet server host.");
        return 1;
    }
    return 0;
}

void ENetServer::Poll()
{
    ENetEvent event;
    while (true) {
        auto res = enet_host_service(Server, &event, net::SERVER_TIMEOUT);
        if (res > 0) {
            if (event.type == ENET_EVENT_TYPE_CONNECT) {
                spdlog::debug("(Server) A new client connected from {}:{}",
                              event.peer->address.host,
                              event.peer->address.port);
                Clients[event.peer->incomingPeerID] = event.peer;

            }
            else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                // de-serialize
                auto *pos = GetPos(event.packet->data);
                spdlog::debug("(Server) A packet of length {} containing {} was received from {} on channel {}",
                              event.packet->dataLength,
                              pos->message()->str(),
                              event.peer->incomingPeerID,
                              event.channelID);
                // enet_host_broadcast(Server, 0, event.packet);
                // enet_packet_destroy(event.packet);

            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                spdlog::debug("(Server) Client {} disconnected.\n", event.peer->incomingPeerID);
                Clients.erase(event.peer->incomingPeerID);
            }
        }
        else if (res < 0) {
            spdlog::error("Error occurred during polling");
            break;
        }
        else {
            break;
        }
    }
}

void ENetServer::Shutdown()
{

    if (!IsServing()) {
        return;
    }

    for (auto it : Clients) {
        auto *client = it.second;
        spdlog::debug("Disconnecting from client {}", client->incomingPeerID);
        enet_peer_disconnect(client, 0);
    }

    auto start = std::chrono::steady_clock::now();
    bool success = false;

    ENetEvent event;

    while (true) {
        auto res = enet_host_service(Server, &event, 0);
        if (res > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                spdlog::debug("Drop packets from {}", event.peer->incomingPeerID);
                enet_packet_destroy(event.packet);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                Clients.erase(event.peer->incomingPeerID);
                spdlog::debug("Disconnection of client {} is successful, {} clients remaining",
                              event.peer->incomingPeerID,
                              Clients.size());
                break;
            }
            else if (event.type == ENET_EVENT_TYPE_CONNECT) {
                spdlog::debug("Connection accepted from client {} during server shutdown, disconnect",
                              event.peer->incomingPeerID);
                Clients[event.peer->incomingPeerID] = event.peer;
                enet_peer_disconnect(event.peer, 0);
            }
        }
        else if (res < 0) {
            spdlog::error("Error when shutting down");
            break;
        }
        else {
            if (Clients.empty()) {
                // all clients successfully disconnected
                spdlog::debug("Disconnection from all clients successful");
                success = true;
                break;
            }
            // check timeout
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::steady_clock::now() - start).count();
            if (duration > SERVER_TIMEOUT) {
                spdlog::error("Disconnection was not acknowledged by server, shutdown forced");
                break;
            }
        }
    }

    // force disconnect the remaining clients
    for (auto it : Clients) {
        auto id = it.first;
        auto client = it.second;
        spdlog::debug("Forcibly disconnecting client {}", id);
        enet_peer_reset(client);
    }

    Clients = std::unordered_map<uint32_t, ENetPeer *>();
    // destroy the host
    enet_host_destroy(Server);
    Server = nullptr;
}

}
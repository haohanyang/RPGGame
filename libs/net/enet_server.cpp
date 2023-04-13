#include "net.h"
#include "serialize_generated.h"

namespace net
{

std::shared_ptr<ENetServer> ENetServer::Create()
{
    return std::make_shared<ENetServer>();
}

ENetServer::ENetServer()
    : Server(nullptr), Clients({nullptr, nullptr})
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
                spdlog::debug("Player {} connected from {}:{}, peer id {}, peer data {}",
                              event.data,
                              event.peer->address.host,
                              event.peer->address.port, event.peer->incomingPeerID, event.peer->data);
                int playerId = event.data;
                Clients[playerId - 1] = event.peer;
            }
            else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                spdlog::debug("Receive message from {}:{}, peer id {}",
                              event.peer->address.host, event.peer->address.port, event.peer->incomingPeerID);
                auto const *message = GetMessage(event.packet->data);
                auto playerId = message->player_id();

                if (message->content_type() == Content_Position) {
                    if (Clients[2 - playerId] != nullptr && Clients[2 - playerId]->state == ENET_PEER_STATE_CONNECTED) {
                        enet_peer_send(Clients[2 - playerId], RELIABLE_CHANNEL, event.packet);
                    }
                    else {
                        spdlog::error("Failed to forward the message to player {}", 3 - playerId);
                    }
                }

                // enet_host_broadcast(Server, 0, event.packet);
                // enet_packet_destroy(event.packet);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                auto playerId = event.data;
                spdlog::debug("Player {} disconnected", playerId);
                Clients[playerId - 1] = nullptr;
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
        if (it != nullptr) {
            spdlog::debug("Disconnecting from player {}", it->incomingPeerID);
            enet_peer_disconnect(it, 0);
        }
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
                Clients[event.peer->incomingPeerID] = nullptr;
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
        if (it != nullptr) {
            spdlog::debug("Forcibly disconnecting player {}", it->incomingPeerID);
            enet_peer_reset(it);
        }
    }

    Clients = {nullptr, nullptr};
    // destroy the host
    enet_host_destroy(Server);
    Server = nullptr;
}

}

#include "net.h"
#include "serialize_generated.h"

#include <chrono>
namespace net {

std::shared_ptr<ENetClient> ENetClient::Create() {
    return std::make_shared<ENetClient>();
}

ENetClient::ENetClient() : Client(nullptr), Server(nullptr)
{
    if (enet_initialize() != 0) {
        spdlog::error("An error occurred while initializing ENet");
        return;
    }

    Client = enet_host_create(nullptr, 1, NUM_CHANNELS,0,0);
    if (Client == nullptr) {
        spdlog::error("An error occurred while trying to create an ENet client host");
    }
}

ENetClient::~ENetClient()
{
    Disconnect();
    enet_host_destroy(Client);
    enet_deinitialize();
}

bool ENetClient::IsConnected()
{
    return Server != nullptr && Server->state == ENET_PEER_STATE_CONNECTED;
}

int ENetClient::Connect(const std::string& host, uint32_t port)
{
    if (IsConnected()) {
        spdlog::debug("ENetClient is already connected to the server");
        return 0;
    }

    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    Server = enet_host_connect(Client, &address, NUM_CHANNELS,0);
    if (Server == nullptr) {
        spdlog::error("No available peers for initiating an ENet connection");
        return 1;
    }

    ENetEvent event;
    if (enet_host_service(Client, &event, CLIENT_TIMEOUT) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        spdlog::debug("Connection to server succeeded");
        return 0;
    }

    spdlog::error("Connection to server failed");
    enet_peer_reset(Server);
    Server = nullptr;
    return 1;
}


void ENetClient::Poll()
{

    if (!IsConnected()) {
        spdlog::error("ENetClient is not connected to any server");
        return;
    }

    ENetEvent event;
    while (true) {
        auto res = enet_host_service(Client, &event, 1000);
        if (res > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    spdlog::debug("(Client) A new server connected from {}:{}",
                                  event.peer -> address.host,
                                  event.peer -> address.port);
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE: {
                    spdlog::debug("(Client) A packet of length {} containing {} was received from {} on channel {}",
                                  event.packet -> dataLength,
                                  *event.packet -> data,
                                  event.peer -> data,
                                  event.channelID);
                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    spdlog::debug("(Client) Server from {}:{} disconnected.\n", event.peer->address.host, event.peer->address.port);
                    break;
                }
            }
        } else if (res < 0) {
            spdlog::error("(Client) Error occurred during polling");
            break;
        } else {
            break;
        }
    }
}

void ENetClient::SendPos(const std::string &message, float x, float y)
{
    auto *packet = Serialize(message, x, y);
    if (enet_peer_send(Server,RELIABLE_CHANNEL,packet) == 0) {
        spdlog::debug("Message was sent");
        enet_host_flush(Client);
    } else {
        spdlog::error("Message failed to send");
    }
}

void ENetClient::Disconnect()
{
    if (IsConnected()) {
        spdlog::debug("Disconnecting from server");
        ENetEvent event;
        enet_peer_disconnect(Server, 0);

        auto start = std::chrono::steady_clock::now();
        bool success = false;

        while (true) {
            auto res = enet_host_service(Client, &event, 0);
            if (res > 0) {
                if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                    // throw away any received packets
                    enet_packet_destroy(event.packet);
                } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                    spdlog::debug("Disconnection is successful");
                    success = true;
                }
            } else if (res < 0) {
                spdlog::error("Encountered error while polling");
            }
            // check for timeout
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::steady_clock::now() - start).count();
            if (duration > CLIENT_TIMEOUT) {
                break;
            }
        }

        if (!success) {
            // disconnect attempt didn't succeed yet, force close the connection
            spdlog::error("Disconnection was not acknowledged by server, shutdown forced");
            enet_peer_reset(Server);
        }
        Server = nullptr;
    }
}
}
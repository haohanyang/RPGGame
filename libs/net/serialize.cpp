#include "net.h"
#include "serialize_generated.h"
namespace net
{

///
/// \param user 0 : player1, 1 : Player2
/// \param x
/// \param y
/// \return ENetPacket
ENetPacket *SerializePos(int user, float x, float y)
{
    flatbuffers::FlatBufferBuilder builder(1024);
    if (user == 0) {
        auto pos = builder.CreateStruct(Position{x, y});
        auto orc = CreateMessage(builder, PlayerNumber_Player1, Content_Position, pos.Union());
        builder.Finish(orc);
    }
    else {
        auto pos = builder.CreateStruct(Position{x, y});
        auto orc = CreateMessage(builder, PlayerNumber_Player2,Content_Position, pos.Union());
        builder.Finish(orc);
    }

    return enet_packet_create(
        builder.GetBufferPointer(),
        builder.GetSize(),
        ENET_PACKET_FLAG_RELIABLE);
}
}
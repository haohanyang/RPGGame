#include "net.h"
#include "serialize_generated.h"

using namespace Serialize;

namespace net
{

ENetPacket *SerializePosition(uint8_t user, float x, float y)
{
    flatbuffers::FlatBufferBuilder builder(1024);

    auto pos = builder.CreateStruct(Position{x, y});
    auto orc = CreateMessage(builder, user, Content_Position, pos.Union());
    builder.Finish(orc);

    return enet_packet_create(
        builder.GetBufferPointer(),
        builder.GetSize(),
        ENET_PACKET_FLAG_RELIABLE);
}
}
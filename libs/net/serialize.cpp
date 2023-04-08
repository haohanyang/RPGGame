#include "net.h"
#include "serialize_generated.h"

namespace net
{
ENetPacket *Serialize(std::string message, float x, float y)
{
    flatbuffers::FlatBufferBuilder builder(1024);
    auto str = builder.CreateString(message);
    auto orc = CreatePos(builder, str, x, y);
    builder.Finish(orc);

    return enet_packet_create(
        builder.GetBufferPointer(),
        builder.GetSize(),
        0);
}

const Pos *DeSerialize(void *data)
{
    auto const *pos = GetPos(data);
    return pos;
}
}
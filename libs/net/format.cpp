#include "net.h"
#include "serialize_generated.h"

template<>
struct fmt::formatter<Pos> : fmt::formatter<std::string>
{
    auto format(Pos pos, format_context &ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "[pos message={}, x={}, y={}]", pos.message()->str(), pos.x(), pos.y());
    }
};
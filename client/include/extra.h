#pragma once

#include "raylib.h"
#include <string>
struct Exit
{
    Rectangle Bounds;
    std::string Destination;
};

struct Positions
{
    Vector2 Player1Position;
    Vector2 Player2Position;
};

struct Chest
{
    Rectangle Bounds;
    std::string Contents;
    bool Opened = false;
};

struct MobInstance
{
    int MobId = -1;
    Vector2 Position;
    int Health;
    int SpriteId;

    bool Triggered = false;
    float LastAttack = -100;
};

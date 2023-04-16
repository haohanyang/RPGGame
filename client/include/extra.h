#pragma once

#include "raylib.h"
#include <string>

constexpr char VersionString[] = "v 0.5.28122021";

constexpr char CopyrightString[] = "Copyright 2021-22 Jeffery Myers";

enum class ApplicationStates
{
    Startup,
    Loading,
    Menu,
    Running,
    Paused,
    GameOver,
    Quitting
};

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

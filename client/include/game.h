/**********************************************************************************************
 *
 *   Raylib RPG Example * A simple RPG made using raylib
 *
 *    LICENSE: zlib/libpng
 *
 *   Copyright (c) 2020 Jeffery Myers
 *
 *   This software is provided "as-is", without any express or implied warranty. In no event
 *   will the authors be held liable for any damages arising from the use of this software.
 *
 *   Permission is granted to anyone to use this software for any purpose, including commercial
 *   applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 *     1. The origin of this software must not be misrepresented; you must not claim that you
 *     wrote the original software. If you use this software in a product, an acknowledgment
 *     in the product documentation would be appreciated but is not required.
 *
 *     2. Altered source versions must be plainly marked as such, and must not be misrepresented
 *     as being the original software.
 *
 *     3. This notice may not be removed or altered from any source distribution.
 *
 **********************************************************************************************/

#pragma once

#include "player.h"
#include "game.h"

#include "net.h"

class GameState
{
public:
    GameState();
    void InitGame(GameMode mode, uint8_t playerId);
    void QuitGame();
    void UpdateGame();

    void LoadLevel(const char *level);
    void StartLevel();

    void GetPlayerInput();

    void UpdateMobs();
    void CullDeadMobs();
    void UpdateMobSprites();
    Player *GetClosestPlayer(const Vector2 &position);

    const Player &GetPartner(Player &player);

    void UpdateSprites();
    void MovePlayer(Player &player);
    void ApplyAction(Player &player);
    void UseConsumable(Player &player, Item *item);
    MobInstance *GetNearestMobInSight(Vector2 &position);

    void ActivateItem(Player &player, int slotIndex);
    void DropItem(Player &player, int item);
    void PlaceItemDrop(TreasureInstance &item, Vector2 &dropPoint);
    void DropLoot(const char *contents, Vector2 &dropPoint);

    Player Player1;
    Player Player2;

    double GameClock = 0;
    std::vector<Exit> Exits;
    std::vector<Chest> Chests;
    std::vector<TreasureInstance> ItemDrops;
    std::vector<MobInstance> Mobs;

    std::function<void()> PauseGame;
    std::function<void(bool, int)> EndGame;

    float GetGameTime();

    std::shared_ptr<net::ENetClient> ENetClient;
    GameMode Mode;
};

inline float GameState::GetGameTime()
{
    return (float) GameClock;
}
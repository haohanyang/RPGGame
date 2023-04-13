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

#ifndef GAME_H
#define GAME_H


#include "player.h"
#include "game.h"
#include "game_hud.h"

constexpr bool DisableFocus = true;

class Game
{
public:
    PlayerData Player;
    PlayerData Partner;
    GameState State;
    GameHudScreen GameHud;

    Game();
    void InitGame();
    void ActivateGame();
    void QuitGame();
    void UpdateGame();

    void LoadLevel(const char *level);
    void StartLevel();

    void GetPlayerInput();

    void UpdateMobs();
    PlayerData *GetClosestPlayer(const Vector2 &position);

    void UpdateSprites();
};

#endif
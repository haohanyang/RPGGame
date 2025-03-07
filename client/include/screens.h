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

#include "raylib.h"
#include "game.h"

#include <functional>

class Screen
{
public:
    virtual void Draw() = 0;

protected:
    bool RectIsHovered(Rectangle &rect);
    void DrawCenteredText(int y, const char *text, int fontSize, Color color);
    bool CenteredButton(int y, const char *text);

    void DimScreen(float alpha = 0.75f);

protected:
    Color ButtonColor = WHITE;
    Color ButtonHighlight = SKYBLUE;
    Color ButtonPressColor = DARKBLUE;

    int ButtonFontSize = 60;
    int ButtonBorder = 10;
};

class GameOverScreen: public Screen
{
public:
    GameOverScreen(bool isWin, bool gold);
    GameOverScreen() = default;
    void Draw() override;
    std::function<void()> GoToMainMenu;
    std::function<void()> QuitApplication;
    bool IsWin = false;
    int Gold = 0;
};

class MainMenuScreen: public Screen
{
public:
    void Draw() override;
    std::function<void(GameMode mode)> StartGame;
    std::function<void()> QuitApplication;
};

class PauseMenuScreen: public Screen
{
public:
    void Draw() override;

    std::function<void()> ResumeGame;
    std::function<void()> GoToMainMenu;
    std::function<void()> QuitApplication;
};

class LoadingScreen: public Screen
{
public:
    std::string LoadingText = "Loading...";

    LoadingScreen();
    void Draw() override;

    Vector2 Origin = {0, 0};

    Rectangle LeftSpinner = {0, 0};
    Rectangle RightSpinner = {0, 0};

    // Load progress 0 = 0% 1 = 100%
    float Progress = 0;
};
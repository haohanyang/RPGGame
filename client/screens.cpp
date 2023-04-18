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

#include "screens.h"
#include "audio.h"
#include "resource_ids.h"


bool Screen::RectIsHovered(Rectangle &rect)
{
    return CheckCollisionPointRec(GetMousePosition(), rect);
}

void Screen::DrawCenteredText(int y, const char *text, int fontSize, Color color)
{
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, GetScreenWidth() / 2 - textWidth / 2, y - fontSize / 2, fontSize, color);
}

bool Screen::CenteredButton(int y, const char *text)
{
    float rectHeight = ButtonFontSize + (ButtonBorder * 2.0f);
    float textWidth = float(MeasureText(text, ButtonFontSize));

    float textXOrigin = GetScreenWidth() / 2.0f - textWidth / 2.0f;
    float textYOrigin = y - ButtonFontSize / 2.0f;

    Rectangle buttonRect = {textXOrigin - ButtonBorder, textYOrigin - ButtonBorder, textWidth + (ButtonBorder * 2.0f),
        ButtonFontSize + (ButtonBorder * 2.0f)};

    bool hovered = RectIsHovered(buttonRect);
    bool down = hovered & IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    Color color = hovered ? (down ? ButtonPressColor : ButtonHighlight) : (ButtonColor);

    DrawRectangleRec(buttonRect, ColorAlpha(color, 0.25f));
    DrawText(text, int(textXOrigin), int(textYOrigin), ButtonFontSize, color);
    DrawRectangleLinesEx(buttonRect, 2, color);

    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked)
        PlaySound(ClickSoundId);

    return clicked;
}

void Screen::DimScreen(float alpha)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, alpha));
}


// main menu screen

void MainMenuScreen::Draw()
{
    // dim the background
    DimScreen();
    // title
    DrawCenteredText(40, "Raylib RPG Example", 40, BLUE);

    // version and copyright
    DrawText(VersionString, 2, GetScreenHeight() - 10, 10, GRAY);
    DrawText(CopyrightString,
             GetScreenWidth() - 2 - MeasureText("CopyrightString", 10),
             GetScreenHeight() - 10,
             10,
             GRAY);

    // play button
    if (CenteredButton(GetScreenHeight() / 4, "Local Play"))
        StartGame(GameMode::LOCAL);

    // online game button
    if (CenteredButton(GetScreenHeight() / 2, "Online Play"))
        StartGame(GameMode::ONLINE);

    // quit button
    if (CenteredButton(GetScreenHeight() - (GetScreenHeight() / 4), "Quit"))
        QuitApplication();
}

// pause screen

void PauseMenuScreen::Draw()
{
    DimScreen();

    DrawCenteredText(40, "Raylib RPG Example", 40, BLUE);
    DrawCenteredText(105, "Paused", 60, RED);

    DrawText(VersionString, 2, GetScreenHeight() - 10, 10, GRAY);
    DrawText(CopyrightString,
             GetScreenWidth() - 2 - MeasureText("CopyrightString", 10),
             GetScreenHeight() - 10,
             10,
             GRAY);

    if (CenteredButton(GetScreenHeight() / 4, "Resume"))
        ResumeGame();

    if (CenteredButton(GetScreenHeight() / 2, "Quit to Menu"))
        GoToMainMenu();

    if (CenteredButton(GetScreenHeight() - (GetScreenHeight() / 4), "Quit to Desktop"))
        QuitApplication();
}

GameOverScreen::GameOverScreen(bool isWin, bool gold)
    : IsWin(isWin), Gold(gold)
{

}

void GameOverScreen::Draw()
{
    // dim the background
    DimScreen();

    // title
    DrawCenteredText(40, "Raylib RPG Example", 40, BLUE);


    // win state
    if (IsWin)
        DrawCenteredText(120, "Congratulations You WON!", 60, WHITE);
    else
        DrawCenteredText(120, "You died, better luck next time.", 60, RED);

    // score
    DrawCenteredText(200, TextFormat("Score = %d", Gold), 60, YELLOW);

    // version and copyright
    DrawText(VersionString, 2, GetScreenHeight() - 10, 10, GRAY);
    DrawText(CopyrightString,
             GetScreenWidth() - 2 - MeasureText("CopyrightString", 10),
             GetScreenHeight() - 10,
             10,
             GRAY);

    // main menu button
    if (CenteredButton(GetScreenHeight() / 2, "Main Menu"))
        GoToMainMenu();

    // quit button
    if (CenteredButton(GetScreenHeight() - (GetScreenHeight() / 4), "Quit"))
        QuitApplication();
}

LoadingScreen::LoadingScreen()
{
    int size = MeasureText(LoadingText.c_str(), 20);
    Origin.x = GetScreenWidth() * 0.5f - size * 0.5f;
    Origin.y = GetScreenHeight() * 0.5f - 10;

    LeftSpinner.x = Origin.x - 25.0f;
    RightSpinner.x = Origin.x + size + 25.0f;
    LeftSpinner.y = RightSpinner.y = GetScreenHeight() * 0.5f;

    LeftSpinner.width = RightSpinner.width = 20;
    LeftSpinner.height = RightSpinner.height = 20;
}

void LoadingScreen::Draw()
{
    DrawText(LoadingText.c_str(), int(Origin.x), int(Origin.y), 20, WHITE);

    // some spinny things to know that the app hasn't locked up
    DrawRectanglePro(LeftSpinner, Vector2{10, 10}, float(GetTime()) * 180.0f, BLUE);
    DrawRectanglePro(RightSpinner, Vector2{10, 10}, float(GetTime()) * -180.0f, BLUE);

    // progress bar.
    float progressWidth = RightSpinner.x - LeftSpinner.x;
    DrawRectangle(int(LeftSpinner.x), int(LeftSpinner.y + 20), (int) (progressWidth * Progress), 5, RAYWHITE);
}
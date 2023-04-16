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

#include "game.h"
#include "loading.h"
#include "map.h"
#include "screens.h"
#include "game_hud.h"
#include "audio.h"

#include "raylib.h"

// setup the window and icon
void SetupWindow()
{
    // Validate that the window is not taller than the monitor size, if so, set it to a smaller size
    int monitor = GetCurrentMonitor();

    int maxHeight = GetMonitorHeight(monitor) - 40;
    if (GetScreenHeight() > maxHeight)
        SetWindowSize(GetScreenWidth(), maxHeight);

    SetExitKey(0);
    SetTargetFPS(144);

    // load an image for the window icon
    Image icon = LoadImage("icons/Icon.6_98.png");

    // ensure that the picture is in the correct format
    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    // replace the background and border colors with transparent
    ImageColorReplace(&icon, BLACK, BLANK);
    ImageColorReplace(&icon, Color{136, 136, 136, 255}, BLANK);

    // set the icon
    SetWindowIcon(icon);

    // free the image data
    UnloadImage(icon);
}

void DrawScreen(Screen *screen)
{
    if (screen != nullptr) {
        screen->Draw();
    }
}

bool SearchAndSetResourceDir(const char *folderName)
{
    // check the working dir
    if (DirectoryExists(folderName)) {
        ChangeDirectory(TextFormat("%s/%s", GetWorkingDirectory(), folderName));
        return true;
    }

    const char *appDir = GetApplicationDirectory();

    // check the applicationDir
    const char *dir = TextFormat("%s%s", appDir, folderName);
    if (DirectoryExists(dir)) {
        ChangeDirectory(dir);
        return true;
    }

    // check one up from the app dir
    dir = TextFormat("%s../%s", appDir, folderName);
    if (DirectoryExists(dir)) {
        ChangeDirectory(dir);
        return true;
    }

    // check two up from the app dir
    dir = TextFormat("%s../../%s", appDir, folderName);
    if (DirectoryExists(dir)) {
        ChangeDirectory(dir);
        return true;
    }

    // check three up from the app dir
    dir = TextFormat("%s../../../%s", appDir, folderName);
    if (DirectoryExists(dir)) {
        ChangeDirectory(dir);
        return true;
    }

    return false;
}

// the main application loop
int main()
{
    Screen *activeScreen = nullptr;
    auto mainMenuScreen = new MainMenuScreen();
    auto pauseMenuScreen = new PauseMenuScreen();
    auto gameOverScreen = new GameOverScreen();
    auto loadingScreen = new LoadingScreen();

    ApplicationStates applicationStates = ApplicationStates::Startup;
    GameState gameState;

    auto gameHud = new GameHudScreen(gameState.Player1, gameState.Player2);

    // Define functions
    std::function<void()> LoadComplete = [&]() mutable
    {
        applicationStates = ApplicationStates::Menu;
        activeScreen = mainMenuScreen;
        // load background world so we have something to look at behind the menu
        LoadMap("maps/menu_map.tmx");
    };

    std::function<void()> QuitApplication = [&]()
    {
        applicationStates = ApplicationStates::Quitting;
    };

    std::function<void()> UpdateMainMenu = [&]()
    {
        if (IsKeyPressed(KEY_ESCAPE))
            applicationStates = ApplicationStates::Quitting; // quit application
    };

    std::function<void()> UpdateGame = [&]()
    {
        gameState.UpdateGame();
    };

    std::function<void()> ResumeGame = [&]()
    {
        applicationStates = ApplicationStates::Running;
        activeScreen = gameHud;
    };

    std::function<void()> UpdatePaused = [&]() mutable
    {
        activeScreen = pauseMenuScreen;
        if (IsKeyPressed(KEY_ESCAPE)) {
            // Resume the game
            ResumeGame();
        }
    };

    // called when the game wants to go back to the main menu, from pause or game over screens
    std::function<void()> GoToMainMenu = [&]()
    {
        // quit our game, if our game was running
        if (applicationStates == ApplicationStates::Running || applicationStates == ApplicationStates::Paused)
            gameState.QuitGame();

        // start our background music again
        StartBGM("sounds/Flowing Rocks.ogg");

        // go back to the main menu like we did when we started up
        LoadComplete();
    };

    // starts a new game
    std::function<void()> StartGame = [&]() mutable
    {
        applicationStates = ApplicationStates::Running;
        activeScreen = nullptr;
        StopBGM();
        gameState.InitGame();
    };

    std::function<void()> PauseGame = [&]()
    {
        applicationStates = ApplicationStates::Paused;
    };

    // called by the game when it is over, by win or loss
    std::function<void(bool, int)>
        EndGame = [&](bool win, int gold) mutable
    {
        applicationStates = ApplicationStates::GameOver;
        gameOverScreen->IsWin = win;
        gameOverScreen->Gold = gold;
        activeScreen = gameOverScreen;
    };


    gameState.EndGame = EndGame;
    gameState.PauseGame = PauseGame;

    mainMenuScreen->StartGame = StartGame;
    mainMenuScreen->QuitApplication = QuitApplication;

    pauseMenuScreen->QuitApplication = QuitApplication;
    pauseMenuScreen->GoToMainMenu = GoToMainMenu;

    gameOverScreen->GoToMainMenu = GoToMainMenu;
    gameOverScreen->QuitApplication = QuitApplication;

    pauseMenuScreen->QuitApplication = QuitApplication;
    pauseMenuScreen->GoToMainMenu = GoToMainMenu;
    pauseMenuScreen->ResumeGame = ResumeGame;

    // setup the window
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(1280, 700, "RPG Example");
    SetupWindow();

    SearchAndSetResourceDir("_resources");
    InitAudio();

    activeScreen = loadingScreen;
    InitResources();


    applicationStates = ApplicationStates::Loading;


    // game loop
    while (!WindowShouldClose() && applicationStates != ApplicationStates::Quitting) {
        // call the update that goes with our current game state
        switch (applicationStates) {
            case ApplicationStates::Loading: UpdateLoad(LoadComplete, loadingScreen);
                break;

            case ApplicationStates::Menu: UpdateMainMenu();
                break;

            case ApplicationStates::Running: UpdateGame();
                break;

            case ApplicationStates::Paused: UpdatePaused();
                break;
        }

        // update the screen for this frame
        BeginDrawing();
        ClearBackground(BLACK);

        // the map is always first because it is always under the menu
        DrawMap();

        // draw whatever menu or hud screen we have
        DrawScreen(activeScreen);

        UpdateAudio();
        EndDrawing();
    }

    ShutdownAudio();
    CleanupResources();
    CloseWindow();

    return 0;
}

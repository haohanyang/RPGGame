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

#include "loading.h"
#include "resource_ids.h"
#include "screens.h"
#include "sprites.h"
#include "items.h"
#include "monsters.h"
#include "audio.h"

#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <deque>

std::deque<std::string> TexturesToLoad;

std::deque<std::string> SoundsToLoad;

std::vector<Texture> LoadedTextures;

Texture DefaultTexture = {0};

size_t LoadedItems = 0;

size_t TotalToLoad = 0;

void InitResources()
{
    // setup the assets to load
    TexturesToLoad.emplace_back("colored_tilemap.png"); //TileSetTexture
    TexturesToLoad.emplace_back("icons/Icon.5_46.png"); //LogoTexture

    // setup default texture
    Image checkered = GenImageChecked(32, 32, 8, 8, GRAY, RAYWHITE);
    DefaultTexture = LoadTextureFromImage(checkered);
    UnloadImage(checkered);

    SoundsToLoad.emplace_back("sounds/click3.ogg");
    SoundsToLoad.emplace_back("sounds/handleCoins.ogg");
    SoundsToLoad.emplace_back("sounds/doorOpen_1.ogg");
    SoundsToLoad.emplace_back("sounds/metalPot1.ogg");
    SoundsToLoad.emplace_back("sounds/creature1.ogg");
    SoundsToLoad.emplace_back("sounds/woosh4.ogg");
    SoundsToLoad.emplace_back("sounds/knifeSlice2.ogg");
    SoundsToLoad.emplace_back("sounds/chop.ogg");
    SoundsToLoad.emplace_back("sounds/creature5.ogg");
    SoundsToLoad.emplace_back("sounds/powerUp2.ogg");

    TotalToLoad = TexturesToLoad.size() + SoundsToLoad.size();
}

void CleanupResources()
{
    // unload the textures
    UnloadTexture(DefaultTexture);
    for (const Texture &texture : LoadedTextures)
        UnloadTexture(texture);

    // clear any data we stored
    LoadedTextures.clear();
    DefaultTexture.id = 0;
}

void FinalizeLoad()
{
    LoadSpriteFrames(TileSetTexture, 14, 12, 4);

    for (int i = 4; i < 14; i++) {
        CenterSprite(i);
        CenterSprite(i + 14);
    }

    CenterSprite(PlayerSprite);
    CenterSprite(PlayerLeatherSprite);
    CenterSprite(PlayerChainSprite);
    CenterSprite(PlayerPlateSprite);

    CenterSprite(ClickTargetSprite);
    CenterSprite(CoinSprite);

    CenterSprite(AwakeSprite);
    CenterSprite(LootSprite);
    CenterSprite(MobAttackSprite);

    SetSpriteBorders(InventoryBackgroundSprite, 10);
    SetSpriteBorders(ItemBackgroundSprite, 10);

    SetupDefaultItems();
    SetupDefaultMobs();
}

void UpdateLoad(std::function<void()> onFinished, std::shared_ptr<LoadingScreen> screen)
{
    if (TexturesToLoad.empty() && SoundsToLoad.empty()) {
        FinalizeLoad();
        onFinished();
        return;
    }

    // load some resources
    // we don't want to load them all in one shot, that may take some time, and the app will look like it is dead
    // so we only load a few per frame.
    const int maxToLoadPerFrame = 1;

    for (int i = 0; i < maxToLoadPerFrame; ++i) {
        if (!TexturesToLoad.empty()) {
            LoadedTextures.push_back(LoadTexture(TexturesToLoad.front().c_str()));
            TexturesToLoad.pop_front();

            LoadedItems++;
        }
        else if (!SoundsToLoad.empty()) {
            LoadSoundFile(SoundsToLoad.front().c_str());
            SoundsToLoad.pop_front();
            LoadedItems++;
        }
    }

    // then get the inverse to get how much we have loaded
    screen->Progress = LoadedItems / float(TotalToLoad);
}

// gets a texture from an ID. The textures are loaded in ID order.
const Texture &GetTexture(int id)
{
    if (id < 0 || id > int(LoadedTextures.size()))
        return DefaultTexture;

    return LoadedTextures[id];
}
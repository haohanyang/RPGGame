#include "game_state.h"
#include "player.h"

#include "game.h"
#include "main.h"
#include "map.h"
#include "game_hud.h"
#include "items.h"
#include "treasure.h"
#include "monsters.h"
#include "audio.h"
#include "resource_ids.h"

#include "raylib.h"
#include "raymath.h"


float GameState::GetGameTime()
{
    return float(GameClock);
}

void GameState::PlaceItemDrop(Positions &positions,TreasureInstance &item, Vector2 &dropPoint)
{

        Item *itemRecord = GetItem(item.ItemId);
        if (!itemRecord)
            return;

        bool valid = false;
        while (!valid)
        {
            float angle = float(GetRandomValue(-180, 180));
            Vector2 vec = {cosf(angle * DEG2RAD), sinf(angle * DEG2RAD)};
            vec = Vector2Add(dropPoint, Vector2Scale(vec, 45));

            if (PointInMap(vec) && Vector2Distance(vec, positions.Player1Position) > PickupDistance
                && Vector2Distance(vec, positions.Player2Position) > PickupDistance)
            {
                item.Position = vec;
                valid = true;
            }

        }

        auto *sprite = AddSprite(itemRecord->Sprite, item.Position);
        sprite->Shadow = true;
        sprite->Bobble = true;
        item.SpriteId = sprite->Id;
        ItemDrops.push_back(item);
}

void GameState::DropLoot(Positions &positions, const char *contents, Vector2 &dropPoint)
{
    std::vector<TreasureInstance> loot = GetLoot(contents);
    for (TreasureInstance &item : loot)
    {
        PlaceItemDrop(positions,item, dropPoint);
        AddEffect(item.Position, EffectType::ScaleFade, LootSprite, 1);
    }
}

void GameState::CullDeadMobs(Positions &positions)
{
    for (auto mobItr = Mobs.begin(); mobItr != Mobs.end();)
    {
        MOB *monsterInfo = GetMob(mobItr->MobId);
        if (monsterInfo != nullptr && mobItr->Health > 0)
        {
            mobItr++;
            continue;
        }

        if (monsterInfo != nullptr)
            DropLoot(positions,monsterInfo->lootTable.c_str(), mobItr->Position);

        RemoveSprite(mobItr->SpriteId);
        if (monsterInfo != nullptr)
            AddEffect(mobItr->Position, EffectType::RotateFade, monsterInfo->Sprite, 3.5f);

        mobItr = Mobs.erase(mobItr);
    }
}

void GameState::UpdateMobSprites()
{
    for (auto &mob : Mobs)
    {
        UpdateSprite(mob.SpriteId, mob.Position);
    }
}
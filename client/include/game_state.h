#ifndef GAME_STATE_H
#define GAME_STATE_H


#include <vector>
#include "extra.h"
#include "treasure.h"
struct GameState {
    std::vector<Exit> Exits;
    std::vector<Chest> Chests;

    double GameClock = 0;

    std::vector<TreasureInstance> ItemDrops;
    std::vector<MobInstance> Mobs;

    float GetGameTime();
    void DropLoot(Positions &positions,const char *contents, Vector2 &dropPoint);
    void PlaceItemDrop(Positions &positions,TreasureInstance &item, Vector2 &dropPoint);
    void CullDeadMobs(Positions &positions);
    void UpdateMobSprites();
};

#endif //GAME_STATE_H

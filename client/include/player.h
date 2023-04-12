#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"

#include "game_state.h"
#include "map.h"
#include "combat.h"
#include "extra.h"
#include "treasure.h"

#include <string>

struct InventoryContents
{
    int ItemId;
    int Quantity;
};


constexpr int MaxHealth = 100;
constexpr float PickupDistance = 20;

class Player
{
public:
    std::string Name;
    int Id;

    Vector2 Position = {0, 0};
    SpriteInstance *Sprite = nullptr;

    bool TargetActive = false;
    Vector2 Target = {0, 0};

    float Speed = 100;

    // player stats
    int Health = 100;
    int Gold = 0;

    GameState &State;

    float LastAttack = 0;
    float LastConsumeable = 0;
    float AttackCooldown = 0;
    float ItemCooldown = 0;

    int BuffItem = -1;
    float BuffLifetimeLeft = 0;

    int BuffDefense = 0;

    // inventory
    int EquipedWeapon = -1;
    int EquipedArmor = -1;
    std::vector<InventoryContents> BackpackContents;

    Chest *TargetChest = nullptr;
    MobInstance *TargetMob = nullptr;


    Player(std::string name, int id, GameState &state);
    const AttackInfo &GetAttack() const;
    const int GetDefense() const;
    TreasureInstance RemoveInventoryItem(int slot, int quantity);
    bool PickupItem(TreasureInstance &drop);
    void UseConsumable(Item *item);
    void ActivateItem(Positions &positions, int slotIndex);
    void DropItem(Positions &positions,int item);
    std::optional<std::string> Move();
    void ApplyActions(Positions &positions);
    void UpdateSprite();

    MobInstance* GetNearestMobInSight(std::vector<MobInstance> &mobs);

private:
    AttackInfo DefaultAttack = {"Slap", true, 1, 1, 1.0f, 10.0f};
    DefenseInfo DefaultDefense = {0};
};


#endif //PLAYER_H

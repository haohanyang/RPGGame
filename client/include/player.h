#pragma once

#include "raylib.h"

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
    uint8_t Id;

    Vector2 Position = {0, 0};
    SpriteInstance *Sprite = nullptr;

    bool TargetActive = false;
    Vector2 Target = {0, 0};

    // player stats
    int Health = 100;
    int Gold = 0;

    float Speed = 100;

    float LastAttack = 0;
    float LastConsumeable = 0;
    float AttackCooldown = 0;
    float ItemCooldown = 0;

    int BuffItem = -1;
    float BuffLifetimeLeft = 0;

    int BuffDefense = 0;

    // inventory
    int EquippedWeapon = -1;
    int EquippedArmor = -1;

    bool InventoryOpen = false;

    std::vector<InventoryContents> BackpackContents;
    bool Waiting = false;
    Chest *TargetChest = nullptr;
    MobInstance *TargetMob = nullptr;

    Player(uint8_t id, std::string name);
    [[nodiscard]] const AttackInfo &GetAttack() const;
    [[nodiscard]] const int GetDefense() const;
    TreasureInstance RemoveInventoryItem(int slot, int quantity);
    bool PickupItem(TreasureInstance &drop);
    void UpdateSprite();

    std::function<void(int)> ActivateItem;
    std::function<void(int)> DropItem;

private:
    AttackInfo DefaultAttack = {"Slap", true, 1, 1, 1.0f, 10.0f};
    DefenseInfo DefaultDefense = {0};
};

inline const AttackInfo &Player::GetAttack() const
{
    if (EquippedWeapon == -1)
        return DefaultAttack;

    return GetItem(EquippedWeapon)->Attack;
}

inline const int Player::GetDefense() const
{
    if (EquippedArmor == -1)
        return 0 + BuffDefense;

    return GetItem(EquippedArmor)->Defense.Defense + BuffDefense;
}
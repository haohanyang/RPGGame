#include "player.h"

#include "main.h"
#include "map.h"
#include "items.h"
#include "treasure.h"
#include "monsters.h"
#include "audio.h"
#include "resource_ids.h"

#include "raylib.h"
#include "raymath.h"

PlayerData::PlayerData(uint8_t id, GameState &state)
    : State(state), Name("Player" + std::to_string(id)), Id(id)
{

}

PlayerData::~PlayerData()
{
   
};
const AttackInfo &PlayerData::GetAttack() const
{
    if (EquipedWeapon == -1)
        return DefaultAttack;

    return GetItem(EquipedWeapon)->Attack;
}

const int PlayerData::GetDefense() const
{
    if (EquipedArmor == -1)
        return 0 + BuffDefense;

    return GetItem(EquipedArmor)->Defense.Defense + BuffDefense;
}

TreasureInstance PlayerData::RemoveInventoryItem(int slot, int quantity)
{
    TreasureInstance treasure = {-1, 0};

    // is it a valid slot?
    if (slot < 0 || slot >= BackpackContents.size())
        return treasure;

    // can't take more than we have
    InventoryContents &inventory = BackpackContents[slot];
    if (inventory.Quantity < quantity)
        quantity = inventory.Quantity;

    // make an item for what we removed
    treasure.ItemId = inventory.ItemId;
    treasure.Quantity = quantity;

    // reduce quantity in inventory
    inventory.Quantity -= quantity;

    // delete the item in inventory if it's empty
    if (inventory.Quantity <= 0) {
        BackpackContents.erase(BackpackContents.begin() + slot);
    }

    // return the drop instance
    return treasure;
}

bool PlayerData::PickupItem(TreasureInstance &drop)
{
    // special case for bag of gold, because it's not a real item
    if (drop.ItemId == GoldBagItem) {
        PlaySound(CoinSoundId);
        Gold += drop.Quantity;
        return true;
    }

    // find our item
    Item *item = GetItem(drop.ItemId);

    // it's an invalid item, remove it but nobody gets it
    if (item == nullptr)
        return true;

    // see if this is a weapon, and we are unarmed, if so, equip one
    if (item->IsWeapon() && EquipedWeapon == -1) {
        EquipedWeapon = item->Id;
        drop.Quantity--;
        PlaySound(ItemPickupSoundId);
    }

    // see if this is armor, and we are naked, if so, equip one
    if (item->IsArmor() && EquipedArmor == -1) {
        EquipedArmor = item->Id;
        drop.Quantity--;
        PlaySound(ItemPickupSoundId);
    }

    // Try to add items to any stacks we already have
    if (drop.Quantity > 0) {
        // see if we have any already
        for (InventoryContents &content : BackpackContents) {
            if (content.ItemId == item->Id) {
                content.Quantity += drop.Quantity;
                drop.Quantity = 0;
                PlaySound(ItemPickupSoundId);
                break;
            }
        }
    }

    // Try to add items to a new inventory slot
    if (drop.Quantity > 0 && BackpackContents.size() < 20) {
        BackpackContents.emplace_back(InventoryContents{item->Id, drop.Quantity});
        drop.Quantity = 0;

        PlaySound(ItemPickupSoundId);
    }

    // if we picked them all up, we can destroy the item
    return drop.Quantity == 0;
}

// return non empty to load new map
std::optional<std::string> PlayerData::Move()
{
    // does the player want to move
    if (TargetActive) {
        Vector2 movement = Vector2Subtract(Target, Position);
        float distance = Vector2Length(movement);

        float frameSpeed = GetFrameTime() * Speed;

        if (distance <= frameSpeed) {
            Position = Target;
            TargetActive = false;
        }
        else {
            movement = Vector2Normalize(movement);
            Vector2 newPos = Vector2Add(Position, Vector2Scale(movement, frameSpeed));

            if (!PointInMap(newPos)) {
                TargetActive = false;
            }
            else {
                Position = newPos;
            }
        }
    }

    // see if the player entered an exit
    for (auto exit : State.Exits) {
        if (CheckCollisionPointRec(Position, exit.Bounds)) {
            Waiting = true;
            TargetMob = nullptr;
            TargetChest = nullptr;
            return exit.Destination;
        }
    }

    return {};
}

void PlayerData::ApplyActions(Positions &positions)
{
    // see if we want to attack any mobs
    if (TargetMob != nullptr) {
        // see if we can even attack.
        if (State.GetGameTime() - LastAttack >= GetAttack().Cooldown) {
            float distance = Vector2Distance(TargetMob->Position, Position);
            if (distance < GetAttack().Range + 40) {
                MOB *monsterInfo = GetMob(TargetMob->MobId);
                if (monsterInfo != nullptr) {
                    AddEffect(TargetMob->Position, EffectType::ScaleFade, ClickTargetSprite);
                    if (!GetAttack().Melee)
                        AddEffect(Position, EffectType::ToTarget, ProjectileSprite, TargetMob->Position, 0.25f);

                    int damage = ResolveAttack(GetAttack(), monsterInfo->Defense.Defense);
                    if (damage == 0) {
                        PlaySound(MissSoundId);
                    }
                    else {
                        PlaySound(HitSoundId);
                        PlaySound(CreatureDamageSoundId);
                        AddEffect(Vector2{TargetMob->Position.x, TargetMob->Position.y - 16},
                                  EffectType::RiseFade,
                                  DamageSprite);
                        TargetMob->Health -= damage;

                        // if you hit them, they wake up!
                        TargetMob->Triggered = true;
                    }
                }
            }

            TargetMob = nullptr;
        }
    }

    // see if the player is near the last clicked chest, if so open it
    if (TargetChest != nullptr) {
        Vector2 center = {TargetChest->Bounds.x + TargetChest->Bounds.width / 2,
            TargetChest->Bounds.y + TargetChest->Bounds.height / 2};
        float distance = Vector2Distance(center, Position);
        if (distance <= 50) {
            if (!TargetChest->Opened) {
                PlaySound(ChestOpenSoundId);
                TargetChest->Opened = true;

                State.DropLoot(positions, TargetChest->Contents.c_str(), center);
            }
            TargetChest = nullptr;
        }
    }

    auto &itemDrops = State.ItemDrops;
    // see if we are under any items to pickup
    for (auto item = itemDrops.begin(); item != itemDrops.end();) {
        float distance = Vector2Distance(item->Position, Position);
        if (distance <= PickupDistance) {
            if (PickupItem(*item)) {
                RemoveSprite(item->SpriteId);
                item = itemDrops.erase(item);
                continue;
            }
        }

        item++;
    }

    float time = State.GetGameTime();

    float attackTime = time - LastAttack;
    float itemTime = time - LastConsumeable;

    if (attackTime >= GetAttack().Cooldown)
        AttackCooldown = 0;
    else
        AttackCooldown = 1.0f - (attackTime / AttackCooldown);

    float itemCooldown = 1;

    if (itemTime >= itemCooldown)
        ItemCooldown = 0;
    else
        ItemCooldown = 1.0f - (itemTime / itemCooldown);

    if (BuffLifetimeLeft > 0) {
        BuffLifetimeLeft -= GetFrameTime();
        if (BuffLifetimeLeft <= 0) {
            BuffDefense = 0;
            BuffItem = -1;
            BuffLifetimeLeft = 0;
        }
    }
}

void PlayerData::UpdateSprite()
{

    if (Sprite != nullptr)
        Sprite->Position = Position;

    if (EquipedArmor == ChainArmorItem)
        Sprite->SpriteFrame = PlayerChainSprite;
    else if (EquipedArmor == PlateArmorItem)
        Sprite->SpriteFrame = PlayerPlateSprite;
    else if (EquipedArmor == LeatherArmorItem)
        Sprite->SpriteFrame = PlayerLeatherSprite;
    else
        Sprite->SpriteFrame = PlayerSprite;
}

MobInstance *PlayerData::GetNearestMobInSight(std::vector<MobInstance> &mobs)
{
    MobInstance *nearest = nullptr;
    float nearestDistance = 9999999.9f;

    for (auto &mob : mobs) {
        if (Ray2DHitsMap(mob.Position, Position))
            continue;

        float dist = Vector2Distance(mob.Position, Position);

        if (dist < nearestDistance) {
            nearest = &mob;
            nearestDistance = dist;
        }
    }

    return nearest;
}

void PlayerData::UseConsumable(Item *item)
{
    if (item == nullptr || !item->IsActivatable())
        return;

    float time = State.GetGameTime() - LastConsumeable;
    if (time < 1)
        return;

    LastConsumeable = State.GetGameTime();

    switch (item->Effect) {
        case ActivatableEffects::Healing:Health += item->Value;
            if (Health > MaxHealth)
                Health = MaxHealth;

            PlaySound(PlayerHealSoundId);
            AddEffect(Position, EffectType::RiseFade, HealingSprite, 2);
            break;

        case ActivatableEffects::Defense:BuffDefense = item->Value;
            BuffLifetimeLeft = item->Durration;
            BuffItem = item->Sprite;
            break;

        case ActivatableEffects::Damage: {
            MobInstance *mob = GetNearestMobInSight(State.Mobs);
            if (mob != nullptr) {
                mob->Health -= item->Value;
                PlaySound(CreatureDamageSoundId);
                AddEffect(Position, EffectType::ToTarget, item->Sprite, mob->Position, 1);
                AddEffect(mob->Position, EffectType::RotateFade, item->Sprite, 1);
            }
            break;
        }
    }
}

void PlayerData::ActivateItem(Positions &positions, int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= BackpackContents.size())
        return;

    InventoryContents &inventorySlot = BackpackContents[slotIndex];

    Item *item = GetItem(inventorySlot.ItemId);
    if (item == nullptr)
        return;

    TreasureInstance removedItem = RemoveInventoryItem(slotIndex, 1);

    if (removedItem.Quantity == 0)
        return;

    switch (item->ItemType) {
        case ItemTypes::Activatable:UseConsumable(item);
            removedItem.ItemId = -1;
            removedItem.Quantity = 0;
            break;

        case ItemTypes::Weapon: {
            // save our current weapon
            int weapon = EquipedWeapon;

            // equip new weapon
            EquipedWeapon = removedItem.ItemId;

            // replace the removed item with the old weapon
            removedItem.ItemId = weapon;
            break;
        }

        case ItemTypes::Armor: {
            // save our current armor
            int armor = EquipedArmor;

            // equip new weapon
            EquipedArmor = removedItem.ItemId;

            // replace the removed item with the old weapon
            removedItem.ItemId = armor;
            break;
        }
    }

    // put whatever we have back, or drop it
    if (removedItem.ItemId != -1) {
        // stick it back in our bag
        if (!PickupItem(removedItem)) {
            // no room, drop it
            State.PlaceItemDrop(positions, removedItem, Position);
        }
    }
}

void PlayerData::DropItem(Positions &positions, int item)
{
    TreasureInstance drop = RemoveInventoryItem(item, 999);
    State.PlaceItemDrop(positions, drop, Position);
}
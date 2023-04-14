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

Player::Player(uint8_t id, std::string name)
    : Name(name), Id(id)
{

}

const AttackInfo &Player::GetAttack() const
{
    if (EquipedWeapon == -1)
        return DefaultAttack;

    return GetItem(EquipedWeapon)->Attack;
}

const int Player::GetDefense() const
{
    if (EquipedArmor == -1)
        return 0 + BuffDefense;

    return GetItem(EquipedArmor)->Defense.Defense + BuffDefense;
}

TreasureInstance Player::RemoveInventoryItem(int slot, int quantity)
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

bool Player::PickupItem(TreasureInstance &drop)
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

void Player::UpdateSprite()
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


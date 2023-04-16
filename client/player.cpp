#include "player.h"

#include "items.h"
#include "treasure.h"
#include "audio.h"
#include "resource_ids.h"

#include "raymath.h"

Player::Player(uint8_t id, std::string name)
    : Name(name), Id(id)
{

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
    if (item->IsWeapon() && EquippedWeapon == -1) {
        EquippedWeapon = item->Id;
        drop.Quantity--;
        PlaySound(ItemPickupSoundId);
    }

    // see if this is armor, and we are naked, if so, equip one
    if (item->IsArmor() && EquippedArmor == -1) {
        EquippedArmor = item->Id;
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

    if (EquippedArmor == ChainArmorItem)
        Sprite->SpriteFrame = PlayerChainSprite;
    else if (EquippedArmor == PlateArmorItem)
        Sprite->SpriteFrame = PlayerPlateSprite;
    else if (EquippedArmor == LeatherArmorItem)
        Sprite->SpriteFrame = PlayerLeatherSprite;
    else
        Sprite->SpriteFrame = PlayerSprite;
}


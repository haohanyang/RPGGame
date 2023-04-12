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

#include "game_hud.h"
#include "items.h"
#include "resource_ids.h"

#include "raylib.h"

GameHudScreen::GameHudScreen(Player &player1, Player &player2)
	: Screen(), Player1(player1), Player2(player2)
{
}

void GameHudScreen::ShowItemToolTip(const Item *item, const Rectangle &rect)
{
	if (item == nullptr || !CheckCollisionPointRec(GetMousePosition(), rect))
		return;

	DrawRectangleRec(Rectangle{rect.x, rect.y, 100, 100}, ColorAlpha(BLACK, 0.75f));
	DrawText(item->Name.c_str(), int(rect.x), int(rect.y), 10, WHITE);
}


void GameHudScreen::DrawInventory(Player &player)
{
    Rectangle inventoryWindowRect = {GetScreenWidth() - 475.0f, GetScreenHeight() - 500.0f, 354, 400.0f};
    Rectangle shadowRect = inventoryWindowRect;
    shadowRect.x += 10;
    shadowRect.y += 10;
    DrawRectangleRec(shadowRect, ColorAlpha(DARKBROWN, 0.5f));
    FillRectWithSprite(InventoryBackgroundSprite, inventoryWindowRect);

    // equipment
    Item *weaponItem = GetItem(player.EquipedWeapon);
    if (DrawButton(inventoryWindowRect.x + 20, inventoryWindowRect.y + 20, weaponItem != nullptr ? weaponItem->Sprite : -1, 0, DARKGRAY, GRAY))
    {
        HoveredItem = weaponItem;
    }
    DrawText(player.Name.c_str(), int(inventoryWindowRect.x) + 8, int(inventoryWindowRect.y) + 3, 15, DARKBROWN);
    DrawText("Weapon", int(inventoryWindowRect.x + 20 + ButtonSize + 2), int(inventoryWindowRect.y + 20), 20, DARKBROWN);
    DrawText(TextFormat("%d - %d", player.GetAttack().MinDamage, player.GetAttack().MaxDamage), int(inventoryWindowRect.x + 20 + ButtonSize + 2), int(inventoryWindowRect.y + 40), 20, WHITE);

    Item *armorItem = GetItem(player.EquipedArmor);
    if (DrawButton(inventoryWindowRect.x + inventoryWindowRect.width - (20 + ButtonSize), inventoryWindowRect.y + 20, armorItem != nullptr ? armorItem->Sprite : -1, 0, DARKBROWN, BROWN))
    {
        HoveredItem = armorItem;
    }
    DrawText("Armor", int(inventoryWindowRect.x + inventoryWindowRect.width - (20 + ButtonSize + 62)), int(inventoryWindowRect.y + ButtonSize), 20, DARKBROWN);
    DrawText(TextFormat("%d", player.GetDefense()), int(inventoryWindowRect.x + inventoryWindowRect.width - (20 + ButtonSize + 22)), int(inventoryWindowRect.y + ButtonSize - 20), 20, WHITE);

    // backpack contents
    constexpr int inventoryItemSize = 64;
    constexpr int inventoryItemPadding = 4;

    DrawText("Backpack (LMB)Use/Equip (RMB)Drop", int(inventoryWindowRect.x + 10), int(inventoryWindowRect.y + 100), 10, DARKBROWN);

    int itemIndex = 0;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            float itemY = inventoryWindowRect.y + (inventoryWindowRect.height - inventoryItemPadding) - ((inventoryItemPadding + inventoryItemSize) * (4 - y));
            float itemX = inventoryWindowRect.x + (inventoryItemPadding * 2) + ((inventoryItemSize + inventoryItemPadding) * x);

            Rectangle itemRect = {itemX, itemY, inventoryItemSize, inventoryItemSize};
            Rectangle shadowRect = itemRect;
            shadowRect.x += 2;
            shadowRect.y += 2;

            DrawRectangleRec(shadowRect, ColorAlpha(BLACK, 0.5f));
            FillRectWithSprite(ItemBackgroundSprite, itemRect);

            if (itemIndex < player.BackpackContents.size())
            {
                Item *item = GetItem(player.BackpackContents[itemIndex].ItemId);
                if (item != nullptr)
                {
                    DrawSprite(item->Sprite, itemRect.x + itemRect.width / 2, itemRect.y + itemRect.height / 2);

                    if (player.BackpackContents[itemIndex].Quantity > 1)
                        DrawText(TextFormat("%d", player.BackpackContents[itemIndex].Quantity), int(itemRect.x) + 2, int(itemRect.y + itemRect.height - 10), 10, WHITE);

                    bool hovered = CheckCollisionPointRec(GetMousePosition(), itemRect);

                    if (hovered)
                    {
                        HoveredItem = item;
                        Positions positions {Player1.Position, Player2.Position};

                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        {
                            if (item->IsActivatable())
                                player.ActivateItem(positions, itemIndex);
                            else if (item->IsWeapon())
                                player.ActivateItem(positions,itemIndex);
                            else if (item->IsArmor())
                                player.ActivateItem(positions,itemIndex);
                        }
                        else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                        {
                            player.DropItem(positions,itemIndex);
                        }
                    }
                }
            }
            itemIndex++;
        }
    }
}


void GameHudScreen::Draw() {
    Draw(Player1, GetScreenHeight() - 160.0f); // upper
    Draw(Player2, GetScreenHeight() - 80.0f); // lower
}

void GameHudScreen::Draw(Player &player, float barHeight)
{
    // background
    DrawRectangleRec(Rectangle{0, barHeight, float(GetScreenWidth()), 80}, ColorAlpha(DARKGRAY, 0.25f));

    // score
    DrawSprite(CoinSprite, GetScreenWidth() - 200.0f, barHeight + 40.0f, 4);
    DrawText(TextFormat("x %03d", player.Gold), GetScreenWidth() - 170, int(barHeight + 20), 40, WHITE);

    // health bar
    DrawText("Health", 20, int(barHeight + 5), 20, RED);

    float healthBarWidth = 300;
    DrawRectangleLinesEx(Rectangle{20, barHeight + 30, healthBarWidth, 32}, 1, WHITE);

    float healthPram = player.Health / float(MaxHealth);
    DrawRectangleRec(Rectangle{22, barHeight + 32, healthBarWidth * healthPram - 4, 28}, RED);

    // clear the hover item from last frame
    HoveredItem = nullptr;

    // action buttons
    float buttonX = 20 + healthBarWidth + 10;
    float buttonY = barHeight + 4;

    Item *weapon = GetItem(player.EquipedWeapon);
    // equipped weapon
    DrawButton(buttonX, buttonY, weapon != nullptr ? weapon->Sprite : -1, 0, DARKGRAY, GRAY);

    if (player.AttackCooldown > 0)
    {
        float height = ButtonSize * player.AttackCooldown;
        DrawRectangleRec(Rectangle{buttonX, buttonY + (ButtonSize - height), ButtonSize, height}, ColorAlpha(RED, 0.5f));
    }

    std::vector<int> activatableItems;
    for (int i = 0; i < player.BackpackContents.size(); i++)
    {
        Item *item = GetItem(player.BackpackContents[i].ItemId);
        if (item != nullptr && item->IsActivatable())
            activatableItems.push_back(i);
    }

    int activatedItem = -1;

    // activatable items
    int backpackSlot = 0;
    for (int i = 0; i < 7; i++)
    {
        buttonX += ButtonSize + 4;

        if (i < activatableItems.size())
        {
            bool shortcutPressed = IsKeyPressed(KEY_ONE + i);

            Item *item = GetItem(player.BackpackContents[activatableItems[i]].ItemId);
            if ((DrawButton(buttonX, buttonY, item->Sprite, player.BackpackContents[activatableItems[i]].Quantity) || shortcutPressed) && item != nullptr)
            {
                if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || shortcutPressed) && player.ItemCooldown == 0)
                {
                    activatedItem = activatableItems[i];
                }
                else
                {
                    HoveredItem = item;
                }
            }

            DrawText(TextFormat("%d", i + 1), int(buttonX), int(buttonY), 20, WHITE);

            if (player.ItemCooldown > 0)
            {
                float height = ButtonSize * player.ItemCooldown;
                DrawRectangleRec(Rectangle{buttonX, buttonY + (ButtonSize - height), ButtonSize, height}, ColorAlpha(BLACK, 0.5f));
            }
        }
    }
    Positions positions {Player1.Position, Player2.Position};

    if (activatedItem != -1) {
        player.ActivateItem(positions, activatedItem);
    }


    // backpack buttons
    buttonX += ButtonSize + 4;

    if ((DrawButton(buttonX, buttonY, BagSprite, 0, GRAY, LIGHTGRAY) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_I))
    {
        if (player.Id == 1)
            Player1InventoryOpen = !Player1InventoryOpen;
        else
            Player2InventoryOpen = !Player2InventoryOpen;
    }

    buttonX += ButtonSize + 4;

    // buff icon
    if (player.BuffLifetimeLeft > 0)
    {
        DrawSprite(player.BuffItem, buttonX + ButtonSize / 2, buttonY + ButtonSize / 2, 0, 2);
        DrawText(TextFormat("%0.0f", player.BuffLifetimeLeft), int(buttonX), int(buttonY + ButtonSize - 30), 30, RED);
    }

    if (player.Id == 1 && Player1InventoryOpen)
        DrawInventory(player);

    if (player.Id == 2 && Player2InventoryOpen)
        DrawInventory(player);

    if (HoveredItem != nullptr)
    {
        Vector2 size = MeasureTextEx(GetFontDefault(), HoveredItem->Name.c_str(), 20, 2);
        Rectangle toolTipRect = {GetMousePosition().x - (size.x / 2 + 2), GetMousePosition().y - (size.y + 2), size.x + 4, size.y + 4};

        DrawRectangleRec(toolTipRect, ColorAlpha(BLACK, 0.5f));
        DrawText(HoveredItem->Name.c_str(), int(toolTipRect.x) + 2, int(toolTipRect.y) + 2, 20, WHITE);
    }

}


bool GameHudScreen::DrawButton(float x, float y, int sprite, int quantity, Color border, Color center)
{
	Rectangle buttonRect = {x, y, ButtonSize, ButtonSize};
	DrawRectangleRec(buttonRect, border);
	DrawRectangleRec(Rectangle{x + ButtonInset, y + ButtonInset, ButtonSize - ButtonInset * 2, ButtonSize - ButtonInset * 2}, center);

	if (sprite != -1)
	{
		Vector2 center = {x + ButtonSize / 2, y + ButtonSize / 2};
		DrawSprite(sprite, center.x + 2, center.y + 2, 0, 2, BLACK);
		DrawSprite(sprite, center.x, center.y, 0, 2);
	}

	if (quantity > 1)
	{
		DrawText(TextFormat("X%d", quantity), int(x + ButtonSize / 2), int(y + ButtonSize - 22), 20, WHITE);
	}

	return CheckCollisionPointRec(GetMousePosition(), buttonRect);
}

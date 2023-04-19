#include "game.h"
#include "map.h"
#include "items.h"
#include "monsters.h"
#include "audio.h"
#include "resource_ids.h"

#include "raylib.h"
#include "raymath.h"

constexpr bool disableLostFocusPause = true;

GameState::GameState()
    : Player1(1, "Player1"), Player2(2, "Player2")
{
    Player1.ActivateItem = [this](int item)
    { ActivateItem(Player1, item); };
    Player1.DropItem = [this](int item)
    { DropItem(Player1, item); };

    Player2.ActivateItem = [this](int item)
    { ActivateItem(Player2, item); };
    Player2.DropItem = [this](int item)
    { DropItem(Player2, item); };
}

void GameState::LoadLevel(const char *level)
{
    LoadMap(level);
    Player1.Sprite = AddSprite(PlayerSprite, Player1.Position);
    Player1.Sprite->Bobble = true;
    Player1.Sprite->Shadow = true;

    Player2.Sprite = AddSprite(PlayerSprite, Player2.Position);
    Player2.Sprite->Bobble = true;
    Player2.Sprite->Shadow = true;
}

void GameState::StartLevel()
{
    GameClock = 0;

    Player1.LastConsumeable = -100;
    Player1.LastAttack = -100;

    Player2.LastConsumeable = -100;
    Player2.LastAttack = -100;

    Player1.Waiting = false;
    Player2.Waiting = false;

    auto *spawn = GetFirstMapObjectOfType(PlayerSpawnType);
    if (spawn != nullptr) {
        Player1.Position.x = spawn->Bounds.x;
        Player1.Position.y = spawn->Bounds.y;

        Player2.Position.x = spawn->Bounds.x;
        Player2.Position.y = spawn->Bounds.y;
    }

    Player1.TargetActive = false;
    Player2.TargetActive = false;

    Exits.clear();
    for (const TileObject *exit : GetMapObjectsOfType(ExitType)) {
        const Property *level = exit->GetProperty("target_level");
        if (level != nullptr) {
            if (level->Value == "-1")
                Exits.emplace_back(Exit{exit->Bounds, "endgame"});
            else
                Exits.emplace_back(Exit{exit->Bounds, "level" + level->Value + ".tmx"});
        }
    }

    Chests.clear();

    Player1.TargetChest = nullptr;
    Player2.TargetChest = nullptr;
    Player1.TargetMob = nullptr;
    Player2.TargetMob = nullptr;

    for (const TileObject *chest : GetMapObjectsOfType(ChestType)) {
        const Property *contents = chest->GetProperty("contents");
        if (contents != nullptr)
            Chests.emplace_back(Chest{chest->Bounds, contents->Value});
    }

    ItemDrops.clear();

    for (const TileObject *mobSpawn : GetMapObjectsOfType(MobSpawnType)) {
        const Property *mobType = mobSpawn->GetProperty("mob_type");

        MOB *monster = GetMob(mobType->GetInt());
        if (monster == nullptr)
            continue;

        Vector2 pos = Vector2{mobSpawn->Bounds.x, mobSpawn->Bounds.y};
        auto *sprite = AddSprite(monster->Sprite, pos);
        sprite->Bobble = true;
        sprite->Shadow = true;

        Mobs.push_back(MobInstance{monster->Id, pos, monster->Health, sprite->Id});
    }
}

void GameState::InitGame(GameMode mode, uint8_t id)
{
    Mode = mode;
    if (mode == GameMode::ONLINE) {
        ENetClient = net::ENetClient::Create(id);
        ENetClient->TraceLog = TraceLog;
        if (ENetClient->Connect("localhost", 8000) != 0) {
            TraceLog(LOG_ERROR, "Error connect");
            Mode = GameMode::LOCAL;
        }

        Player1.Id = id;
        Player2.Id = 3 - id;
    }
    else {
        Player1.Id = 1;
        Player2.Id = 2;
    }
    // load start level
    LoadLevel("maps/level0.tmx");
    StartLevel();
}

void GameState::QuitGame()
{
    ClearMap();
}

void GameState::GetPlayerInput()
{
    float moveUnit = 2.0f;

    // User1 input
    bool player1KeyPressed = false;
    Vector2 player1TargetPosition = Player1.Position;

    if (IsKeyDown(KEY_LEFT)) {
        player1TargetPosition.x -= moveUnit;
        player1KeyPressed = true;
    }

    if (IsKeyDown(KEY_RIGHT)) {
        player1TargetPosition.x += moveUnit;
        player1KeyPressed = true;
    }

    if (IsKeyDown(KEY_UP)) {
        player1TargetPosition.y -= moveUnit;
        player1KeyPressed = true;
    }

    if (IsKeyDown(KEY_DOWN)) {
        player1TargetPosition.y += moveUnit;
        player1KeyPressed = true;
    }

    // check for key inputs
    if (!Player1.Waiting && player1KeyPressed) {
        if (PointInMap(player1TargetPosition)) {
            Player1.TargetActive = true;
            Player1.Target = player1TargetPosition;
            if (Mode == GameMode::ONLINE)
                ENetClient->SendPosition(player1TargetPosition.x, player1TargetPosition.y);
        }

        Player1.TargetChest = nullptr;
        for (auto &chest : Chests) {
            if (CheckCollisionPointRec(player1TargetPosition, chest.Bounds)) {
                Player1.TargetChest = &chest;
            }
        }

        // if player is close to any mob
        if (!Player1.Waiting) {
            for (auto &mob : Mobs) {
                if (CheckCollisionPointCircle(player1TargetPosition, mob.Position, 20)) {
                    Player1.TargetMob = &mob;

                    if (Vector2Distance(Player1.Position, mob.Position) <= Player1.GetAttack().Range + 40)
                        Player1.TargetActive = false;
                    break;
                }
            }
        }
    }

    // User2 input
    bool player2KeyPressed = false;
    Vector2 player2TargetPosition;

    if (Mode != GameMode::ONLINE) {
        player2TargetPosition = Player2.Position;
        if (IsKeyDown(KEY_A)) {
            player2TargetPosition.x -= moveUnit;
            player2KeyPressed = true;
        }

        if (IsKeyDown(KEY_D)) {
            player2TargetPosition.x += moveUnit;
            player2KeyPressed = true;
        }

        if (IsKeyDown(KEY_W)) {
            player2TargetPosition.y -= moveUnit;
            player2KeyPressed = true;
        }

        if (IsKeyDown(KEY_S)) {
            player2TargetPosition.y += moveUnit;
            player2KeyPressed = true;
        }
    }
    else {
        auto pos = ENetClient->GetPosition(Player2.Id);
        if (pos != nullptr) {
            player2KeyPressed = true;
            player2TargetPosition.x = pos->x();
            player2TargetPosition.y = pos->y();
        }
    }

    if (!Player2.Waiting && player2KeyPressed) {
        if (PointInMap(player2TargetPosition)) {
            Player2.TargetActive = true;
            Player2.Target = player2TargetPosition;
        }

        Player2.TargetChest = nullptr;
        for (auto &chest : Chests) {
            if (CheckCollisionPointRec(player2TargetPosition, chest.Bounds)) {
                Player2.TargetChest = &chest;
            }
        }

        if (!Player2.Waiting) {
            for (auto &mob : Mobs) {
                if (CheckCollisionPointCircle(player2TargetPosition, mob.Position, 20)) {
                    Player2.TargetMob = &mob;

                    if (Vector2Distance(Player2.Position, mob.Position) <= Player2.GetAttack().Range + 40)
                        Player2.TargetActive = false;
                    break;
                }
            }
        }
    }
}

/// <summary>
/// Get user's target position. Set set TargetChest and TargetMob if players is close to it.
/// </summary>
/// <param name="player"></param>
void GameState::GetPlayerInput(Player& player)
{

    if (player.Id == Player1.Id) {
        // Player1
        bool keyPressed = false;
        Vector2 targetPosition = player.Position;

        if (IsKeyDown(KEY_LEFT)) {
            targetPosition.x -= MoveUnit;
            keyPressed = true;
        }

        if (IsKeyDown(KEY_RIGHT)) {
            targetPosition.x += MoveUnit;
            keyPressed = true;
        }

        if (IsKeyDown(KEY_UP)) {
            targetPosition.y -= MoveUnit;
            keyPressed = true;
        }

        if (IsKeyDown(KEY_DOWN)) {
            targetPosition.y += MoveUnit;
            keyPressed = true;
        }

        // check for key inputs
        if (!player.Waiting && keyPressed) {
            if (PointInMap(targetPosition)) {
                player.TargetActive = true;
                player.Target = targetPosition;
                if (Mode == GameMode::ONLINE)
                    ENetClient->SendPosition(targetPosition.x, targetPosition.y);
            }

            // if player is close to any chest
            player.TargetChest = nullptr;
            for (auto& chest : Chests) {
                if (CheckCollisionPointRec(targetPosition, chest.Bounds)) {
                    player.TargetChest = &chest;
                }
            }

            // if player is close to any mob
            if (!player.Waiting) {
                for (auto& mob : Mobs) {
                    if (CheckCollisionPointCircle(targetPosition, mob.Position, 20)) {
                        player.TargetMob = &mob;

                        if (Vector2Distance(player.Position, mob.Position) <= player.GetAttack().Range + 40)
                            player.TargetActive = false;
                        break;
                    }
                }
            }
        }
    }
    else {
        // Player2
        bool keyPressed = false;
        Vector2 targetPosition = player.Position;

        if (Mode == GameMode::LOCAL) {
            if (IsKeyDown(KEY_A)) {
                targetPosition.x -= MoveUnit;
                keyPressed = true;
            }

            if (IsKeyDown(KEY_D)) {
                targetPosition.x += MoveUnit;
                keyPressed = true;
            }

            if (IsKeyDown(KEY_W)) {
                targetPosition.y -= MoveUnit;
                keyPressed = true;
            }

            if (IsKeyDown(KEY_S)) {
                targetPosition.y += MoveUnit;
                keyPressed = true;
            }
        }
        else {
            auto pos = ENetClient->GetPosition(player.Id);
            if (pos != nullptr) {
                keyPressed = true;
                targetPosition.x = pos->x();
                targetPosition.y = pos->y();
            }
        }
           
        if (!player.Waiting && keyPressed) {
            // Try to get the position from network
            auto pos = ENetClient->GetPosition(player.Id);
            if (pos != nullptr) {
                Vector2 targetPosition = { pos->x(), pos->y() };
                player.TargetActive = true;
                player.Target = targetPosition;

                // if player is close to any chest
                player.TargetChest = nullptr;
                for (auto& chest : Chests) {
                    if (CheckCollisionPointRec(targetPosition, chest.Bounds)) {
                        player.TargetChest = &chest;
                    }
                }

                // if player is close to any mob
                if (!player.Waiting) {
                    for (auto& mob : Mobs) {
                        if (CheckCollisionPointCircle(targetPosition, mob.Position, 20)) {
                            player.TargetMob = &mob;

                            if (Vector2Distance(player.Position, mob.Position) <= player.GetAttack().Range + 40)
                                player.TargetActive = false;
                            break;
                        }
                    }
                }
            }
        }
    }
}

Player *GameState::GetClosestPlayer(const Vector2 &position)
{
    auto vecToPlayer1 = Vector2Subtract(Player1.Position, position);
    auto distance1 = Vector2Length(vecToPlayer1);

    auto vecToPlayer2 = Vector2Subtract(Player2.Position, position);
    float distance2 = Vector2Length(vecToPlayer2);

    if (distance1 < distance2)
        return &Player1;

    return &Player2;
}

void GameState::CullDeadMobs()
{
    for (auto mobItr = Mobs.begin(); mobItr != Mobs.end();) {
        MOB *monsterInfo = GetMob(mobItr->MobId);
        if (monsterInfo != nullptr && mobItr->Health > 0) {
            mobItr++;
            continue;
        }

        if (monsterInfo != nullptr)
            DropLoot(monsterInfo->lootTable.c_str(), mobItr->Position);

        RemoveSprite(mobItr->SpriteId);
        if (monsterInfo != nullptr)
            AddEffect(mobItr->Position, EffectType::RotateFade, monsterInfo->Sprite, 3.5f);

        mobItr = Mobs.erase(mobItr);
    }
}

void GameState::UpdateMobSprites()
{
    for (auto &mob : Mobs) {
        UpdateSprite(mob.SpriteId, mob.Position);
    }
}

void GameState::UpdateMobs()
{
    Positions positions{Player1.Position, Player2.Position};
    CullDeadMobs();

    // check for mob actions
    for (auto &mob : Mobs) {
        auto *player = GetClosestPlayer(mob.Position);
        if (player->Waiting)
            continue;

        auto vecToPlayer = Vector2Subtract(player->Position, mob.Position);
        auto distance = Vector2Length(vecToPlayer);

        MOB *monsterInfo = GetMob(mob.MobId);

        if (monsterInfo == nullptr)
            continue;

        if (!mob.Triggered) {
            // see if the mob should wake up
            if (distance > monsterInfo->DetectionRadius) // too far away
                continue;

            if (Ray2DHitsMap(player->Position, mob.Position))
                continue; // something is blocking line of sight

            // we see our prey, wake up and get em.
            mob.Triggered = true;

            PlaySound(AlertSoundId);
            AddEffect(mob.Position, EffectType::RiseFade, AwakeSprite, 1);
        }

        if (mob.Triggered) {
            if (distance < monsterInfo->Attack.Range) {
                // try to attack the player
                if (GetGameTime() - mob.LastAttack >= monsterInfo->Attack.Cooldown) {
                    mob.LastAttack = GetGameTime();
                    int damage = ResolveAttack(monsterInfo->Attack, player->GetDefense());

                    if (monsterInfo->Attack.Melee)
                        AddEffect(player->Position, EffectType::RotateFade, MobAttackSprite);
                    else
                        AddEffect(mob.Position, EffectType::ToTarget, ProjectileSprite, player->Position, 0.5f);

                    if (damage == 0) {
                        PlaySound(MissSoundId);
                    }
                    else {
                        PlaySound(HitSoundId);
                        PlaySound(PlayerDamageSoundId);
                        AddEffect(Vector2{player->Position.x, player->Position.y - 16},
                                  EffectType::RiseFade,
                                  DamageSprite);
                        player->Health -= damage;
                    }
                }
            }
            else {
                // try to move
                Vector2 movement = Vector2Normalize(vecToPlayer);

                float frameSpeed = monsterInfo->Speed * GetFrameTime();
                Vector2 newPos = Vector2Add(mob.Position, Vector2Scale(movement, frameSpeed));

                if (PointInMap(newPos))
                    mob.Position = newPos;
            }
        }
    }
}

void GameState::UpdateSprites()
{
    Player1.UpdateSprite();
    Player2.UpdateSprite();
    UpdateMobSprites();
}

void GameState::UpdateGame()
{
    if (IsKeyPressed(KEY_ESCAPE)) {

        if (Player1.InventoryOpen || Player2.InventoryOpen) {
            Player1.InventoryOpen = false;
            Player2.InventoryOpen = false;
        }
        else {
            PauseGame();
            return;
        }
    }

    if (!disableLostFocusPause && !IsWindowFocused()) {
        PauseGame();
        return;
    }

    // only update our game clock when we are unpaused
    GameClock += GetFrameTime();
    GetPlayerInput();
    //GetPlayerInput(Player1);
    //GetPlayerInput(Player2);

    MovePlayer(Player1);
    MovePlayer(Player2);

    ApplyAction(Player1);
    ApplyAction(Player2);

    UpdateMobs();

    if (Player1.Health < 0 || Player2.Health < 0) {
        // you died, change to the end screen
        EndGame(false, Player1.Gold + Player2.Gold);
    }

    UpdateSprites();

    SetVisiblePoint(Player1.Position);
    SetVisiblePoint(Player2.Position);
}

MobInstance *GameState::GetNearestMobInSight(Vector2 &position)
{
    MobInstance *nearest = nullptr;
    float nearestDistance = 9999999.9f;

    for (auto &mob : Mobs) {
        if (Ray2DHitsMap(mob.Position, position))
            continue;

        float dist = Vector2Distance(mob.Position, position);

        if (dist < nearestDistance) {
            nearest = &mob;
            nearestDistance = dist;
        }
    }

    return nearest;
}

void GameState::UseConsumable(Player &player, Item *item)
{
    if (item == nullptr || !item->IsActivatable())
        return;

    float time = GetGameTime() - player.LastConsumeable;
    if (time < 1)
        return;

    player.LastConsumeable = GetGameTime();

    switch (item->Effect) {
        case ActivatableEffects::Healing:player.Health += item->Value;
            if (player.Health > MaxHealth)
                player.Health = MaxHealth;

            PlaySound(PlayerHealSoundId);
            AddEffect(player.Position, EffectType::RiseFade, HealingSprite, 2);
            break;

        case ActivatableEffects::Defense:player.BuffDefense = item->Value;
            player.BuffLifetimeLeft = item->Durration;
            player.BuffItem = item->Sprite;
            break;

        case ActivatableEffects::Damage: {
            MobInstance *mob = GetNearestMobInSight(player.Position);
            if (mob != nullptr) {
                mob->Health -= item->Value;
                PlaySound(CreatureDamageSoundId);
                AddEffect(player.Position, EffectType::ToTarget, item->Sprite, mob->Position, 1);
                AddEffect(mob->Position, EffectType::RotateFade, item->Sprite, 1);
            }
            break;
        }
    }
}

void GameState::PlaceItemDrop(TreasureInstance &item, Vector2 &dropPoint)
{
    Item *itemRecord = GetItem(item.ItemId);
    if (!itemRecord)
        return;

    bool valid = false;
    while (!valid) {
        float angle = float(GetRandomValue(-180, 180));
        Vector2 vec = {cosf(angle * DEG2RAD), sinf(angle * DEG2RAD)};
        vec = Vector2Add(dropPoint, Vector2Scale(vec, 45));

        if (PointInMap(vec) && Vector2Distance(vec, Player1.Position) > PickupDistance &&
            Vector2Distance(vec, Player2.Position) > PickupDistance) {
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

void GameState::ActivateItem(Player &player, int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= player.BackpackContents.size())
        return;

    InventoryContents &inventorySlot = player.BackpackContents[slotIndex];

    Item *item = GetItem(inventorySlot.ItemId);
    if (item == nullptr)
        return;

    TreasureInstance removedItem = player.RemoveInventoryItem(slotIndex, 1);

    if (removedItem.Quantity == 0)
        return;

    switch (item->ItemType) {
        case ItemTypes::Activatable:UseConsumable(player, item);
            removedItem.ItemId = -1;
            removedItem.Quantity = 0;
            break;

        case ItemTypes::Weapon: {
            // save our current weapon
            int weapon = player.EquippedWeapon;

            // equip new weapon
            player.EquippedWeapon = removedItem.ItemId;

            // replace the removed item with the old weapon
            removedItem.ItemId = weapon;
            break;
        }

        case ItemTypes::Armor: {
            // save our current armor
            int armor = player.EquippedArmor;

            // equip new weapon
            player.EquippedArmor = removedItem.ItemId;

            // replace the removed item with the old weapon
            removedItem.ItemId = armor;
            break;
        }
    }

    // put whatever we have back, or drop it
    if (removedItem.ItemId != -1) {
        // stick it back in our bag
        if (!player.PickupItem(removedItem)) {
            // no room, drop it
            PlaceItemDrop(removedItem, player.Position);
        }
    }
}

void GameState::DropItem(Player &player, int item)
{
    TreasureInstance drop = player.RemoveInventoryItem(item, 999);
    PlaceItemDrop(drop, player.Position);
}

const Player &GameState::GetPartner(Player &player)
{
    if (player.Id == 1)
        return Player2;
    return Player1;
}

void GameState::MovePlayer(Player &player)
{
    // does the player want to move
    if (player.TargetActive) {
        Vector2 movement = Vector2Subtract(player.Target, player.Position);
        float distance = Vector2Length(movement);

        float frameSpeed = GetFrameTime() * player.Speed;

        if (distance <= frameSpeed) {
            player.Position = player.Target;
            player.TargetActive = false;
        }
        else {
            movement = Vector2Normalize(movement);
            Vector2 newPos = Vector2Add(player.Position, Vector2Scale(movement, frameSpeed));

            if (!PointInMap(newPos)) {
                player.TargetActive = false;
            }
            else {
                player.Position = newPos;
            }
        }
    }

    // see if the player entered an exit
    for (auto &exit : Exits) {
        if (CheckCollisionPointRec(player.Position, exit.Bounds)) {
            player.Waiting = true;
            player.TargetChest = nullptr;
            player.TargetChest = nullptr;

            if (GetPartner(player).Waiting) {
                if (exit.Destination == "endgame") {
                    EndGame(true, player.Gold + 100);
                }
                else {
                    std::string map = "maps/" + exit.Destination;
                    LoadLevel(map.c_str());
                    StartLevel();
                }
            }
            break;
        }
    }
}

void GameState::DropLoot(const char *contents, Vector2 &dropPoint)
{
    std::vector<TreasureInstance> loot = GetLoot(contents);
    for (TreasureInstance &item : loot) {
        PlaceItemDrop(item, dropPoint);
        AddEffect(item.Position, EffectType::ScaleFade, LootSprite, 1);
    }
}

void GameState::ApplyAction(Player &player)
{

    // see if we want to attack any mobs
    if (player.TargetMob != nullptr) {
        // see if we can even attack.
        if (GetGameTime() - player.LastAttack >= player.GetAttack().Cooldown) {
            float distance = Vector2Distance(player.TargetMob->Position, player.Position);
            if (distance < player.GetAttack().Range + 40) {
                MOB *monsterInfo = GetMob(player.TargetMob->MobId);
                if (monsterInfo != nullptr) {
                    AddEffect(player.TargetMob->Position, EffectType::ScaleFade, ClickTargetSprite);
                    if (!player.GetAttack().Melee)
                        AddEffect(player.Position,
                                  EffectType::ToTarget,
                                  ProjectileSprite,
                                  player.TargetMob->Position,
                                  0.25f);

                    int damage = ResolveAttack(player.GetAttack(), monsterInfo->Defense.Defense);
                    if (damage == 0) {
                        PlaySound(MissSoundId);
                    }
                    else {
                        PlaySound(HitSoundId);
                        PlaySound(CreatureDamageSoundId);
                        AddEffect(Vector2{player.TargetMob->Position.x, player.TargetMob->Position.y - 16},
                                  EffectType::RiseFade,
                                  DamageSprite);
                        player.TargetMob->Health -= damage;

                        // if you hit them, they wake up!
                        player.TargetMob->Triggered = true;
                    }
                }
            }

            player.TargetMob = nullptr;
        }
    }

    // see if the player is near the last clicked chest, if so open it
    if (player.TargetChest != nullptr) {
        Vector2 center = {player.TargetChest->Bounds.x + player.TargetChest->Bounds.width / 2,
            player.TargetChest->Bounds.y + player.TargetChest->Bounds.height / 2};
        float distance = Vector2Distance(center, player.Position);
        if (distance <= 50) {
            if (!player.TargetChest->Opened) {
                PlaySound(ChestOpenSoundId);
                player.TargetChest->Opened = true;

                DropLoot(player.TargetChest->Contents.c_str(), center);
            }
            player.TargetChest = nullptr;
        }
    }


    // see if we are under any items to pickup
    for (auto item = ItemDrops.begin(); item != ItemDrops.end();) {
        float distance = Vector2Distance(item->Position, player.Position);
        if (distance <= PickupDistance) {
            if (player.PickupItem(*item)) {
                RemoveSprite(item->SpriteId);
                item = ItemDrops.erase(item);
                continue;
            }
        }

        item++;
    }

    float time = GetGameTime();

    float attackTime = time - player.LastAttack;
    float itemTime = time - player.LastConsumeable;

    if (attackTime >= player.GetAttack().Cooldown)
        player.AttackCooldown = 0;
    else
        player.AttackCooldown = 1.0f - (attackTime / player.AttackCooldown);

    float itemCooldown = 1;

    if (itemTime >= itemCooldown)
        player.ItemCooldown = 0;
    else
        player.ItemCooldown = 1.0f - (itemTime / itemCooldown);

    if (player.BuffLifetimeLeft > 0) {
        player.BuffLifetimeLeft -= GetFrameTime();
        if (player.BuffLifetimeLeft <= 0) {
            player.BuffDefense = 0;
            player.BuffItem = -1;
            player.BuffLifetimeLeft = 0;
        }
    }
}
#include "game.h"
#include "main.h"
#include "map.h"
#include "items.h"
#include "monsters.h"
#include "audio.h"
#include "resource_ids.h"

#include "raylib.h"
#include "raymath.h"

Game::Game()
    : State(), Player(1, State), Partner(2, State), GameHud(Player, Partner)
{

}

void Game::LoadLevel(const char *level)
{
    LoadMap(level);
    Player.Sprite = AddSprite(PlayerSprite, Player.Position);
    Player.Sprite->Bobble = true;
    Player.Sprite->Shadow = true;

    Partner.Sprite = AddSprite(PlayerSprite, Partner.Position);
    Partner.Sprite->Bobble = true;
    Partner.Sprite->Shadow = true;
}

void Game::StartLevel()
{
    State.GameClock = 0;

    Player.LastConsumeable = -100;
    Player.LastAttack = -100;

    Partner.LastConsumeable = -100;
    Partner.LastAttack = -100;

    Player.Waiting = false;
    Partner.Waiting = false;

    auto *spawn = GetFirstMapObjectOfType(PlayerSpawnType);
    if (spawn != nullptr) {
        Player.Position.x = spawn->Bounds.x;
        Player.Position.y = spawn->Bounds.y;

        Partner.Position.x = spawn->Bounds.x;
        Partner.Position.y = spawn->Bounds.y;
    }

    Player.TargetActive = false;
    Partner.TargetActive = false;

    State.Exits.clear();
    for (const TileObject *exit : GetMapObjectsOfType(ExitType)) {
        const Property *level = exit->GetProperty("target_level");
        if (level != nullptr) {
            if (level->Value == "-1")
                State.Exits.emplace_back(Exit{exit->Bounds, "endgame"});
            else
                State.Exits.emplace_back(Exit{exit->Bounds, "level" + level->Value + ".tmx"});
        }
    }

    State.Chests.clear();

    Player.TargetChest = nullptr;
    Partner.TargetChest = nullptr;
    Player.TargetMob = nullptr;
    Partner.TargetMob = nullptr;

    for (const TileObject *chest : GetMapObjectsOfType(ChestType)) {
        const Property *contents = chest->GetProperty("contents");
        if (contents != nullptr)
            State.Chests.emplace_back(Chest{chest->Bounds, contents->Value});
    }

    State.ItemDrops.clear();

    for (const TileObject *mobSpawn : GetMapObjectsOfType(MobSpawnType)) {
        const Property *mobType = mobSpawn->GetProperty("mob_type");

        MOB *monster = GetMob(mobType->GetInt());
        if (monster == nullptr)
            continue;

        Vector2 pos = Vector2{mobSpawn->Bounds.x, mobSpawn->Bounds.y};
        auto *sprite = AddSprite(monster->Sprite, pos);
        sprite->Bobble = true;
        sprite->Shadow = true;

        State.Mobs.push_back(MobInstance{monster->Id, pos, monster->Health, sprite->Id});
    }
}

void Game::InitGame()
{
    ActivateGame();

    // load start level
    LoadLevel("maps/level0.tmx");
    StartLevel();
}

void Game::QuitGame()
{
    ClearMap();
}

void Game::ActivateGame()
{
    SetActiveScreen(&GameHud);
}

void Game::GetPlayerInput()
{
    float moveUnit = 2.0f;

    // User1 input
    bool playerKeyPressed = false;
    Vector2 playerTargetPosition = Player.Position;


    if (IsKeyDown(KEY_LEFT)) {
        playerTargetPosition.x -= moveUnit;
        playerKeyPressed = true;
    }

    if (IsKeyDown(KEY_RIGHT)) {
        playerTargetPosition.x += moveUnit;
        playerKeyPressed = true;
    }

    if (IsKeyDown(KEY_UP)) {
        playerTargetPosition.y -= moveUnit;
        playerKeyPressed = true;
    }

    if (IsKeyDown(KEY_DOWN)) {
        playerTargetPosition.y += moveUnit;
        playerKeyPressed = true;
    }

    // check for key inputs
    if (!Player.Waiting && playerKeyPressed) {

        if (PointInMap(playerTargetPosition)) {
            Player.TargetActive = true;
            Player.Target = playerTargetPosition;
            Player.ENetClient->SendPosition(playerTargetPosition.x, playerTargetPosition.y);
        }

        Player.TargetChest = nullptr;
        for (auto &chest : State.Chests) {
            if (CheckCollisionPointRec(playerTargetPosition, chest.Bounds)) {
                Player.TargetChest = &chest;
            }
        }

        // if player is close to any mob
        if (!Player.Waiting) {
            for (auto &mob : State.Mobs) {
                if (CheckCollisionPointCircle(playerTargetPosition, mob.Position, 20)) {
                    Player.TargetMob = &mob;

                    if (Vector2Distance(Player.Position, mob.Position) <= Player.GetAttack().Range + 40)
                        Player.TargetActive = false;
                    break;
                }
            }
        }
    }

    Vector2 partnerTargetPosition = Partner.Position;

    if (!Partner.Waiting) {
        auto packet = Player.ENetClient->GetPosition(Partner.Id);
        if (packet != nullptr) {
            partnerTargetPosition.x = packet->x();
            partnerTargetPosition.y = packet->y();
            if (PointInMap(partnerTargetPosition)) {
                Partner.TargetActive = true;
                Partner.Target = partnerTargetPosition;
            }
        }

        Partner.TargetChest = nullptr;
        for (auto &chest : State.Chests) {
            if (CheckCollisionPointRec(partnerTargetPosition, chest.Bounds)) {
                Partner.TargetChest = &chest;
            }
        }

        if (!Partner.Waiting) {
            for (auto &mob : State.Mobs) {
                if (CheckCollisionPointCircle(partnerTargetPosition, mob.Position, 20)) {
                    Partner.TargetMob = &mob;

                    if (Vector2Distance(Partner.Position, mob.Position) <= Partner.GetAttack().Range + 40)
                        Partner.TargetActive = false;
                    break;
                }
            }
        }
    }
}

PlayerData *Game::GetClosestPlayer(const Vector2 &position)
{
    auto vecToPlayer1 = Vector2Subtract(Player.Position, position);
    auto distance1 = Vector2Length(vecToPlayer1);

    auto vecToPlayer2 = Vector2Subtract(Partner.Position, position);
    float distance2 = Vector2Length(vecToPlayer2);

    if (distance1 < distance2)
        return &Player;

    return &Partner;
}

void Game::UpdateMobs()
{
    Positions positions{Player.Position, Partner.Position};
    State.CullDeadMobs(positions);

    // check for mob actions
    for (auto &mob : State.Mobs) {
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
                if (State.GetGameTime() - mob.LastAttack >= monsterInfo->Attack.Cooldown) {
                    mob.LastAttack = State.GetGameTime();
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

void Game::UpdateSprites()
{
    Player.UpdateSprite();
    Partner.UpdateSprite();
    State.UpdateMobSprites();
}

void Game::UpdateGame()
{
    if (IsKeyPressed(KEY_ESCAPE)) {

        if (GameHud.Player1InventoryOpen || GameHud.Player2InventoryOpen) {
            GameHud.Player1InventoryOpen = false;
            GameHud.Player2InventoryOpen = false;
        }
        else {
            PauseGame();
            return;
        }
    }

    if (!DisableFocus && !IsWindowFocused()) {
        PauseGame();
        return;
    }

    // only update our game clock when we are unpaused
    State.GameClock += GetFrameTime();

    GetPlayerInput();

    auto destination1 = Player.Move();
    if (destination1.has_value()) {
        if (Player.Waiting && Partner.Waiting) {
            if (destination1.value() == "endgame") {
                EndGame(true, Player.Gold + 100);
            }
            else {
                std::string map = "maps/" + destination1.value();
                LoadLevel(map.c_str());
                StartLevel();
            }
        }
    }

    auto destination2 = Partner.Move();
    if (destination2.has_value()) {
        if (Player.Waiting && Partner.Waiting) {
            if (destination2.value() == "endgame") {
                EndGame(true, Player.Gold + 100);
            }
            else {
                std::string map = "maps/" + destination2.value();
                LoadLevel(map.c_str());
                StartLevel();
            }
        }
    }

    Positions positions{Player.Position, Partner.Position};
    Player.ApplyActions(positions);
    Partner.ApplyActions(positions);

    UpdateMobs();

    if (Player.Health < 0 || Partner.Health < 0) {
        // you died, change to the end screen
        EndGame(false, Player.Gold + Partner.Gold);
    }

    UpdateSprites();

    SetVisiblePoint(Player.Position);
    SetVisiblePoint(Partner.Position);
}
#include "game.h"
#include "main.h"
#include "map.h"
#include "items.h"
#include "monsters.h"
#include "audio.h"
#include "resource_ids.h"

#include "raylib.h"
#include "raymath.h"

Game::Game() : State(), Player1("Player1", 1, State),Player2("Player2", 2, State), GameHud(Player1, Player2)
{

}

void Game::LoadLevel(const char *level)
{
	LoadMap(level);
	Player1.Sprite = AddSprite(PlayerSprite, Player1.Position);
	Player1.Sprite->Bobble = true;
	Player1.Sprite->Shadow = true;

	Player2.Sprite = AddSprite(PlayerSprite, Player2.Position);
	Player2.Sprite->Bobble = true;
	Player2.Sprite->Shadow = true;
}

void Game::StartLevel()
{
	State.GameClock = 0;

	Player1.LastConsumeable = -100;
	Player1.LastAttack = -100;

	Player2.LastConsumeable = -100;
	Player2.LastAttack = -100;

	auto *spawn = GetFirstMapObjectOfType(PlayerSpawnType);
	if (spawn != nullptr)
	{
		Player1.Position.x = spawn->Bounds.x;
		Player1.Position.y = spawn->Bounds.y;

		Player2.Position.x = spawn->Bounds.x;
		Player2.Position.y = spawn->Bounds.y;
	}

	Player1.TargetActive = false;
	Player2.TargetActive = false;

	State.Exits.clear();
	for (const TileObject *exit : GetMapObjectsOfType(ExitType))
	{
		const Property *level = exit->GetProperty("target_level");
		if (level != nullptr)
		{
			if (level->Value == "-1")
				State.Exits.emplace_back(Exit{exit->Bounds, "endgame"});
			else
				State.Exits.emplace_back(Exit{exit->Bounds, "level" + level->Value + ".tmx"});
		}
	}

	State.Chests.clear();
	// State.TargetChest = nullptr;
    Player1.TargetChest = nullptr;
    Player2.TargetChest = nullptr;
	for (const TileObject *chest : GetMapObjectsOfType(ChestType))
	{
		const Property *contents = chest->GetProperty("contents");
		if (contents != nullptr)
			State.Chests.emplace_back(Chest{chest->Bounds, contents->Value});
	}

	State.ItemDrops.clear();

	for (const TileObject *mobSpawn : GetMapObjectsOfType(MobSpawnType))
	{
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
	bool player1KeyPressed = false;
	Vector2 player1TargetPosition = Player1.Position;

	if (IsKeyDown(KEY_LEFT))
	{
		player1TargetPosition.x -= moveUnit;
		player1KeyPressed = true;
	}

	if (IsKeyDown(KEY_RIGHT))
	{
		player1TargetPosition.x += moveUnit;
		player1KeyPressed = true;
	}

	if (IsKeyDown(KEY_UP))
	{
		player1TargetPosition.y -= moveUnit;
		player1KeyPressed = true;
	}

	if (IsKeyDown(KEY_DOWN))
	{
		player1TargetPosition.y += moveUnit;
		player1KeyPressed = true;
	}

	// User2 input
	bool player2KeyPressed = false;
	Vector2 player2TargetPosition = Player2.Position;

	if (IsKeyDown(KEY_A))
	{
		player2TargetPosition.x -= moveUnit;
		player2KeyPressed = true;
	}

	if (IsKeyDown(KEY_D))
	{
		player2TargetPosition.x += moveUnit;
		player2KeyPressed = true;
	}

	if (IsKeyDown(KEY_W))
	{
		player2TargetPosition.y -= moveUnit;
		player2KeyPressed = true;
	}

	if (IsKeyDown(KEY_S))
	{
		player2TargetPosition.y += moveUnit;
		player2KeyPressed = true;
	}

	// check for key inputs
	if (player1KeyPressed)
	{
		if (PointInMap(player1TargetPosition))
		{
			Player1.TargetActive = true;
			Player1.Target = player1TargetPosition;
		}

		Player1.TargetChest = nullptr;
		for (auto &chest : State.Chests)
		{
			if (CheckCollisionPointRec(player1TargetPosition, chest.Bounds))
			{
				Player1.TargetChest = &chest;
			}
		}

		for (auto &mob : State.Mobs)
		{
			if (CheckCollisionPointCircle(player1TargetPosition, mob.Position, 20))
			{
				Player1.TargetMob = &mob;

				if (Vector2Distance(Player1.Position, mob.Position) <= Player1.GetAttack().Range + 40)
					Player1.TargetActive = false;
				break;
			}
		}
	}

	if (player2KeyPressed)
	{
		if (PointInMap(player2TargetPosition))
		{
			Player2.TargetActive = true;
			Player2.Target = player2TargetPosition;
		}

		Player2.TargetChest = nullptr;
		for (auto &chest : State.Chests)
		{
			if (CheckCollisionPointRec(player2TargetPosition, chest.Bounds))
			{
				Player2.TargetChest = &chest;
			}
		}

		for (auto &mob : State.Mobs)
		{
			if (CheckCollisionPointCircle(player2TargetPosition, mob.Position, 20))
			{
				Player2.TargetMob = &mob;

				if (Vector2Distance(Player2.Position, mob.Position) <= Player2.GetAttack().Range + 40)
					Player2.TargetActive = false;
				break;
			}
		}
	}
}

Player *Game::GetClosestPlayer(const Vector2 &position)
{
    auto vecToPlayer1 = Vector2Subtract(Player1.Position, position);
    auto distance1 = Vector2Length(vecToPlayer1);

    auto vecToPlayer2 = Vector2Subtract(Player2.Position, position);
    float distance2 = Vector2Length(vecToPlayer2);

    if (distance1 < distance2)
        return &Player1;

    return &Player2;
}

void Game::UpdateMobs()
{
    Positions positions {Player1.Position, Player2.Position};
	State.CullDeadMobs(positions);

	// check for mob actions
	for (auto &mob : State.Mobs)
	{
        auto * player = GetClosestPlayer(mob.Position);

		auto vecToPlayer = Vector2Subtract(player->Position, mob.Position);
		auto distance = Vector2Length(vecToPlayer);

		MOB *monsterInfo = GetMob(mob.MobId);
		if (monsterInfo == nullptr)
			continue;

		if (!mob.Triggered)
		{
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

		if (mob.Triggered)
		{
			if (distance < monsterInfo->Attack.Range)
			{
				// try to attack the player
				if (State.GetGameTime() - mob.LastAttack >= monsterInfo->Attack.Cooldown)
				{
					mob.LastAttack = State.GetGameTime();
					int damage = ResolveAttack(monsterInfo->Attack, player->GetDefense());

					if (monsterInfo->Attack.Melee)
						AddEffect(player->Position, EffectType::RotateFade, MobAttackSprite);
					else
						AddEffect(mob.Position, EffectType::ToTarget, ProjectileSprite, player->Position, 0.5f);

					if (damage == 0)
					{
						PlaySound(MissSoundId);
					}
					else
					{
						PlaySound(HitSoundId);
						PlaySound(PlayerDamageSoundId);
						AddEffect(Vector2{player->Position.x, player->Position.y - 16}, EffectType::RiseFade, DamageSprite);
						player->Health -= damage;
					}
				}
			}
			else
			{
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
	Player1.UpdateSprite();
	Player2.UpdateSprite();
	State.UpdateMobSprites();
}

void Game::UpdateGame()
{
	if (IsKeyPressed(KEY_ESCAPE))
	{

		if (GameHud.Player1InventoryOpen || GameHud.Player2InventoryOpen) {
            GameHud.Player1InventoryOpen = false;
            GameHud.Player2InventoryOpen = false;
        }
		else
		{
			PauseGame();
			return;
		}
	}

	if (!IsWindowFocused())
	{
		PauseGame();
		return;
	}

	// only update our game clock when we are unpaused
	State.GameClock += GetFrameTime();

	GetPlayerInput();

	auto destination1 = Player1.Move();
    if (destination1.has_value()) {
        if (destination1.value() == "endgame") {
            EndGame(true, Player1.Gold + 100);
        } else {
            std::string map = "maps/" + destination1.value();
            LoadLevel(map.c_str());
            StartLevel();
        }
    }

    auto destination2 = Player2.Move();
    if (destination2.has_value()) {
        if (destination2.value() == "endgame") {
            EndGame(true, Player1.Gold + 100);
        } else {
            std::string map = "maps/" + destination2.value();
            LoadLevel(map.c_str());
            StartLevel();
        }
    }

    Positions positions {Player1.Position, Player2.Position};
    Player1.ApplyActions(positions);
	Player2.ApplyActions(positions);

	UpdateMobs();

	if (Player1.Health < 0 || Player2.Health < 0)
	{
		// you died, change to the end screen
		EndGame(false, Player1.Gold + Player2.Gold);
	}

	UpdateSprites();

	SetVisiblePoint(Player1.Position);
	SetVisiblePoint(Player2.Position);
}
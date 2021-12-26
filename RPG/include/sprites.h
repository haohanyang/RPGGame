#pragma once

#include "raylib.h"

#include <stdint.h>

#define SpriteFlipNone 0
#define SpriteFlipX 0x02
#define SpriteFlipY 0x04
#define SpriteFlipDiagonal 0x08

void LoadSpriteFrames(int textureId, int colums, int rows, int spacing);
void SetSpriteOrigin(int spriteId, int x, int y);
void SetSpriteBorders(int spriteId, int left, int top, int right, int bottom);
void SetSpriteBorders(int spriteId, int inset);
void CenterSprite(int spriteId);

void DrawSprite(int spriteId, float x, float y, float rotation = 0, float scale = 1, Color tint = { 255, 255, 255, 255 }, uint8_t flip = SpriteFlipNone);
void FillRectWithSprite(int spriteId, const Rectangle& rect, Color tint = { 255, 255, 255, 255 }, uint8_t flip = SpriteFlipNone);

// sprite IDs
constexpr int BackgroundSprite = 60;
constexpr int PlayerSprite = 5;
constexpr int ClickTargetSprite = 140;
constexpr int CoinSprite = 141;
constexpr int InventoryBackgroundSprite = 147;
constexpr int ItemBackgroundSprite = 148;

constexpr int BagSprite = 108;
constexpr int SwordSprite = 62;
constexpr int LeatherArmorSprite = 142;
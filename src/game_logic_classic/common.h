/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 * This project is based on disassembly of NUKEM2.EXE from the game
 * Duke Nukem II, Copyright (C) 1993 Apogee Software, Ltd.
 *
 * Some parts of the code are based on or have been adapted from the Cosmore
 * project, Copyright (Context* ctx, ) 2020-2022 Scott Smitelli.
 * See LICENSE_Cosmore file at the root of the repository, or refer to
 * https://github.com/smitelli/cosmore/blob/master/LICENSE.
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (Context* ctx, t your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "gamedefs.h"
#include "vars.h"


#ifdef __cplusplus
extern "C"
{
#endif

  byte RandomNumber(Context* ctx);

  bool MM_Init(Context* ctx);
  void far* MM_PushChunk(Context* ctx, word size, ChunkType type);

  void pascal InitParticleSystem(Context* ctx);
  void pascal SpawnParticles(
    Context* ctx,
    word x,
    word y,
    signed char xVelocityScale,
    byte color);
  void pascal UpdateAndDrawParticles(Context* ctx);
  void pascal ClearParticles(Context* ctx);

  void SetScreenShift(Context* ctx, byte amount);

  void pascal GiveScore(Context* ctx, word score);
  void pascal AddInventoryItem(Context* ctx, word item);
  bool pascal RemoveFromInventory(Context* ctx, word item);

  void pascal PlaySound(Context* ctx, int16_t id);

  void pascal DrawActor(
    Context* ctx,
    word id,
    word frame,
    word x,
    word y,
    word drawStyle);

  word Map_GetTile(Context* ctx, word x, word y);
  void Map_SetTile(Context* ctx, word tileIndex, word x, word y);

  int16_t pascal ApplyWorldCollision(Context* ctx, word handle, word direction);
  bool pascal SpawnEffect(
    Context* ctx,
    word id,
    word x,
    word y,
    word type,
    word spawnDelay);

  bool pascal FindPlayerShotInRect(
    Context* ctx,
    word left,
    word top,
    word right,
    word bottom);
  void pascal Map_DestroySection(
    Context* ctx,
    word left,
    word top,
    word right,
    word bottom);
  bool pascal SpawnEffect(
    Context* ctx,
    word id,
    word x,
    word y,
    word type,
    word spawnDelay);
  void pascal SpawnDestructionEffects(
    Context* ctx,
    word handle,
    int16_t* spec,
    word actorId);
  void pascal
    SpawnBurnEffect(Context* ctx, word effectId, word sourceId, word x, word y);
  void DamagePlayer(Context* ctx);
  void UpdateAndDrawActors(Context* ctx);
  void UpdateAndDrawPlayerShots(Context* ctx);
  void UpdateAndDrawEffects(Context* ctx);
  void UpdateAndDrawWaterAreas(Context* ctx);
  void UpdateAndDrawTileDebris(Context* ctx);

  void pascal SpawnActor(Context* ctx, word id, word x, word y);
  int16_t pascal ApplyWorldCollision(Context* ctx, word handle, word direction);
  int16_t pascal CheckWorldCollision(
    Context* ctx,
    word direction,
    word actorId,
    word frame,
    word x,
    word y);
  void pascal ShowTutorial(Context* ctx, TutorialId index);
  void pascal PlaySoundIfOnScreen(Context* ctx, word handle, byte soundId);
  bool pascal IsActorOnScreen(Context* ctx, word handle);
  bool pascal
    IsSpriteOnScreen(Context* ctx, word id, word frame, word x, word y);
  bool pascal AreSpritesTouching(
    Context* ctx,
    word id1,
    word frame1,
    word x1,
    word y1,
    word id2,
    word frame2,
    word x2,
    word y2);
  bool pascal FindPlayerShotInRect(
    Context* ctx,
    word left,
    word top,
    word right,
    word bottom);
  bool pascal Boss3_IsTouchingPlayer(Context* ctx, word handle);
  void pascal
    HandleActorShotCollision(Context* ctx, int16_t damage, word handle);
  bool pascal PlayerInRange(Context* ctx, word handle, word distance);
  bool SpawnActorInSlot(Context* ctx, word slot, word id, word x, word y);
  void pascal InitActorState(
    Context* ctx,
    word listIndex,
    ActorUpdateFunc updateFunc,
    word id,
    word x,
    word y,
    word alwaysUpdate,
    word remainActive,
    word allowStairStepping,
    word gravityAffected,
    word health,
    word var1,
    word var2,
    word var3,
    word var4,
    word var5,
    word scoreGiven);
  byte pascal TestShotCollision(Context* ctx, word handle);
  void pascal
    SpawnPlayerShot(Context* ctx, word id, word x, word y, word direction);

  void ShowInGameMessage(Context* ctx, const char* msg);

  void ResetEffectsAndPlayerShots(Context* ctx);
  void UpdateAndDrawGame(Context* ctx);
  void UpdatePlayer(Context* ctx);
  void UpdateMovingMapParts(Context* ctx);
  void SetPixel(Context* ctx, word x, word y, byte color);
  void HUD_ShowOnRadar(Context* ctx, word x, word y);
  void DrawTileDebris(Context* ctx, word tileIndex, word x, word y);
  void ShowLevelSpecificHint(Context* ctx);

#ifdef __cplusplus
}
#endif

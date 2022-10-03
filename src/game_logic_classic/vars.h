/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 * This project is based on disassembly of NUKEM2.EXE from the game
 * Duke Nukem II, Copyright (C) 1993 Apogee Software, Ltd.
 *
 * Some parts of the code are based on or have been adapted from the Cosmore
 * project, Copyright (c) 2020-2022 Scott Smitelli.
 * See LICENSE_Cosmore file at the root of the repository, or refer to
 * https://github.com/smitelli/cosmore/blob/master/LICENSE.
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
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

#include "defs.h"
#include "gamedefs.h"

typedef struct Context_
{
  void* pRigelBridge;

  byte retConveyorBeltCheckResult;
  byte mapViewportHeight;
  bool gfxFlashScreen;
  byte gfxScreenFlashColor;
  bool gmIsTeleporting;
  int16_t plCollectedLetters;
  word gmTeleportTargetPosX;
  word gmTeleportTargetPosY;
  byte retPlayerShotDirection;
  bool gmPlayerTookDamage;
  word far* gfxTilesetAttributes;
  word mapBottom;
  word mapWidthShift;
  word mapWidth;
  word gmCameraPosX;
  word gmCameraPosY;
  word far* mapData;
  word far gmTileDebrisStates[700 * 5];
  word far* gfxActorInfoData;
  word gmNumActors;
  bool gmBossActivated;
  bool plRapidFireIsActiveFrame;
  bool gmRequestUnlockNextDoor;
  byte gmCurrentEpisode;
  byte gmCurrentLevel;
  ActorState gmActorStates[MAX_NUM_ACTORS];
  word levelActorListSize;
  word gfxCurrentDisplayPage;
  byte gmGameState;
  EffectState gmEffectStates[MAX_NUM_EFFECTS];
  PlayerShot gmPlayerShotStates[MAX_NUM_PLAYER_SHOTS];
  bool inputMoveUp;
  bool inputMoveDown;
  bool inputMoveLeft;
  bool inputMoveRight;
  bool inputJump;
  bool inputFire;
  word plRapidFireTimeLeft;
  dword plScore;
  bool mapParallaxBoth;
  bool mapParallaxHorizontal;
  bool mapBackdropAutoScrollX;
  bool mapBackdropAutoScrollY;
  byte mapSecondaryBackdrop;
  bool mapHasReactorDestructionEvent;
  bool mapSwitchBackdropOnTeleport;
  byte gmRngIndex;
  bool plOnElevator;
  byte plAirlockDeathStep;
  byte plBodyExplosionStep;
  byte plFallingSpeed;
  byte plDeathAnimationStep;
  byte plState;
  byte plJumpStep;
  byte plMercyFramesLeft;
  word plPosX;
  word plPosY;
  word gmBeaconPosX;
  word gmBeaconPosY;
  byte plActorId;
  byte plAnimationFrame;
  bool plKilledInShip;
  word gmPlayerEatingActor;
  bool gmRequestUnlockNextForceField;
  byte plInteractAnimTicks;
  bool plBlockLookingUp;
  bool mapHasEarthquake;
  byte gmEarthquakeCountdown;
  byte gmEarthquakeThreshold;
  byte gmReactorDestructionStep;
  byte gmNumMovingMapParts;
  word plCloakTimeLeft;
  MovingMapPartState gmMovingMapParts[MAX_NUM_MOVING_MAP_PARTS];
  word gmCamerasDestroyed;
  word gmCamerasInLevel;
  word gmWeaponsCollected;
  word gmWeaponsInLevel;
  word gmMerchCollected;
  word gmMerchInLevel;
  word gmTurretsDestroyed;
  word gmTurretsInLevel;
  word gmOrbsLeft;
  word gmBombBoxesLeft;
  bool gmWaterAreasPresent;
  word plAttachedSpider1;
  word plAttachedSpider2;
  word plAttachedSpider3;
  word gmBossHealth;
  byte gmRadarDishesLeft;
  word gmCloakPickupPosX;
  word gmCloakPickupPosY;
  word gmExplodingSectionLeft;
  word gmExplodingSectionTop;
  word gmExplodingSectionRight;
  word gmExplodingSectionBottom;
  byte gmExplodingSectionTicksElapsed;
  word gmActiveFanIndex;
  bool plBlockJumping;
  bool plWalkAnimTicksDue;
  bool plBlockShooting;
  byte levelHeaderData[3002];
  word mmChunkSizes[MM_MAX_NUM_CHUNKS];
  ChunkType mmChunkTypes[MM_MAX_NUM_CHUNKS];
  dword mmMemTotal;
  dword mmMemUsed;
  word mmChunksUsed;
  word far* psParticleData[NUM_PARTICLE_GROUPS];
  ParticleGroup psParticleGroups[NUM_PARTICLE_GROUPS];
  bool bdUseSecondary;
  byte plLadderAnimationStep;
  byte gmDifficulty;
  byte plWeapon;
  byte plAmmo;
  byte plHealth;
  bool gmBeaconActivated;
  byte mmRawMem[MM_TOTAL_SIZE];
} Context;

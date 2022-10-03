/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game_world_classic.hpp"

#include "assets/file_utils.hpp"
#include "assets/resource_loader.hpp"
#include "base/string_utils.hpp"
#include "base/warnings.hpp"
#include "data/unit_conversions.hpp"
#include "engine/random_number_generator.hpp"
#include "frontend/game_service_provider.hpp"
#include "frontend/user_profile.hpp"
#include "game_logic/dynamic_geometry_system.hpp"
#include "renderer/upscaling.hpp"
#include "renderer/viewport_utils.hpp"

#include "actors.h"
#include "common.h"
#include "defs.h"
#include "gamedefs.h"
#include "lvlhead.h"
#include "sounds.h"
#include "vars.h"


RIGEL_DISABLE_WARNINGS

using namespace rigel;


game_logic::Bridge& getBridge(Context* ctx)
{
  return *static_cast<game_logic::Bridge*>(ctx->pRigelBridge);
}


static data::InventoryItemType convertItemType(const word id)
{
  using IT = data::InventoryItemType;

  switch (id)
  {
    case ACT_CIRCUIT_CARD:
      return IT::CircuitBoard;

    case ACT_BLUE_KEY:
      return IT::BlueKey;

    case ACT_RAPID_FIRE_ICON:
      return IT::RapidFire;

    case ACT_SPECIAL_HINT_GLOBE_ICON:
      return IT::SpecialHintGlobe;

    case ACT_CLOAKING_DEVICE_ICON:
      return IT::CloakingDevice;
  }

  assert(false);
  return IT::RapidFire;
}


byte RandomNumber(Context* ctx)
{
  ctx->gmRngIndex++;
  return byte(rigel::engine::RANDOM_NUMBER_TABLE[ctx->gmRngIndex]);
}


void PlaySound(Context* ctx, int16_t id)
{
  getBridge(ctx).mpServiceProvider->playSound(
    static_cast<rigel::data::SoundId>(id));
}


void SetScreenShift(Context* ctx, byte amount)
{
  getBridge(ctx).mScreenShift = amount;
}


void pascal HUD_ShowOnRadar(Context* ctx, word x, word y)
{
  int16_t x1 = ctx->plPosX - 17;
  int16_t y1 = ctx->plPosY - 17;

  if (
    (int16_t)x > x1 && x < ctx->plPosX + 16 && (int16_t)y > y1 &&
    y < ctx->plPosY + 16)
  {
    x1 = x - ctx->plPosX;
    y1 = y - ctx->plPosY;

    getBridge(ctx).mRadarDots.push_back({x1, y1});
  }
}


void SetPixel(Context* ctx, word x, word y, byte color)
{
  getBridge(ctx).mPixelsToDraw.push_back(game_logic::PixelDrawCmd{x, y, color});
}


void DrawTileDebris(Context* ctx, word tileIndex, word x, word y)
{
  getBridge(ctx).mTileDebrisToDraw.push_back(
    game_logic::TileDrawCmd{tileIndex, x, y});
}


word Map_GetTile(Context* ctx, word x, word y)
{
  if ((int16_t)y < 0)
  {
    return 0;
  }

  return *(ctx->mapData + x + (y << ctx->mapWidthShift));
}


void Map_SetTile(Context* ctx, word tileIndex, word x, word y)
{
  *(ctx->mapData + x + (y << ctx->mapWidthShift)) = tileIndex;

  if (tileIndex < 8000)
  {
    getBridge(ctx).mpMap->setTileAt(0, x, y, tileIndex / 8);
  }
  else
  {
    assert(false);
  }
}


void ShowInGameMessage(Context* ctx, const char* msg)
{
  getBridge(ctx).mpMessageDisplay->setMessage(msg);
}


void pascal ShowTutorial(Context* ctx, TutorialId index)
{
  const auto id = static_cast<rigel::data::TutorialMessageId>(index);

  if (!getBridge(ctx).mpPlayerModel->tutorialMessages().hasBeenShown(id))
  {
    getBridge(ctx).mpMessageDisplay->setMessage(data::messageText(id));
    getBridge(ctx).mpPlayerModel->tutorialMessages().markAsShown(id);
  }
}


void pascal GiveScore(Context* ctx, word score)
{
  ctx->plScore += score;
}


void pascal AddInventoryItem(Context* ctx, word item)
{
  getBridge(ctx).mpPlayerModel->giveItem(convertItemType(item));
}


void ShowLevelSpecificHint(Context* ctx) { }


bool pascal RemoveFromInventory(Context* ctx, word item)
{
  const auto type = convertItemType(item);

  if (!getBridge(ctx).mpPlayerModel->hasItem(type))
  {
    return false;
  }

  getBridge(ctx).mpPlayerModel->removeItem(type);
  return true;
}


void pascal
  DrawActor(Context* ctx, word id, word frame, word x, word y, word drawStyle)
{
  getBridge(ctx).mSpritesToDraw.push_back(
    game_logic::SpriteDrawCmd{id, frame, x, y, drawStyle});
}


void DamagePlayer(Context* ctx)
{
  if (
    !ctx->plCloakTimeLeft && !ctx->plMercyFramesLeft &&
    ctx->plState != PS_DYING)
  {
    ctx->plHealth--;
    ctx->gmPlayerTookDamage = true;

    if (ctx->plHealth > 0 && ctx->plHealth < 12)
    {
      ctx->plMercyFramesLeft = 50 - ctx->gmDifficulty * 10;
      PlaySound(ctx, SND_DUKE_PAIN);
    }
    else
    {
      if (ctx->plState == PS_USING_SHIP)
      {
        ctx->plKilledInShip = true;
      }

      ctx->plState = PS_DYING;
      ctx->plDeathAnimationStep = 0;
      PlaySound(ctx, SND_DUKE_DEATH);
    }
  }
}


namespace rigel::game_logic
{

namespace
{

const base::Color SCREEN_FLASH_COLORS[] = {
  data::GameTraits::INGAME_PALETTE[0],
  data::GameTraits::INGAME_PALETTE[15],
  data::GameTraits::INGAME_PALETTE[7],
  data::GameTraits::INGAME_PALETTE[0]};


const char EPISODE_PREFIXES[] = {'L', 'M', 'N', 'O'};


std::string levelFileName(const int16_t episode, const int16_t level)
{
  assert(episode >= 0 && episode < 4);
  assert(level >= 0 && level < 8);

  std::string fileName;
  fileName += EPISODE_PREFIXES[episode];
  fileName += std::to_string(level + 1);
  fileName += ".MNI";
  return fileName;
}


[[nodiscard]] auto setupIngameViewport(
  renderer::Renderer* pRenderer,
  const int16_t screenShakeOffsetX)
{
  auto saved = renderer::saveState(pRenderer);

  const auto offset =
    data::GameTraits::inGameViewportOffset + base::Vec2{screenShakeOffsetX, 0};
  renderer::setLocalTranslation(pRenderer, offset);
  renderer::setLocalClipRect(
    pRenderer, base::Rect<int>{{}, data::GameTraits::inGameViewportSize});

  return saved;
}


void ResetGameState(Context* ctx)
{
  int16_t i;

  ctx->gmGameState = GS_RUNNING;
  ctx->gmBossActivated = false;
  ctx->plBodyExplosionStep = 0;
  ctx->plAttachedSpider1 = 0;
  ctx->plAttachedSpider2 = 0;
  ctx->plAttachedSpider3 = 0;
  ctx->plOnElevator = false;
  ctx->gfxFlashScreen = false;
  ctx->plKilledInShip = false;
  ctx->gfxCurrentDisplayPage = 1;
  ctx->gmRngIndex = 0;
  ctx->plAnimationFrame = 0;
  ctx->plState = PS_NORMAL;
  ctx->plMercyFramesLeft = INITIAL_MERCY_FRAMES;
  ctx->gmIsTeleporting = false;
  ctx->gmExplodingSectionTicksElapsed = 0;
  ctx->plInteractAnimTicks = 0;
  ctx->plBlockLookingUp = false;
  ctx->gmEarthquakeCountdown = 0;
  ctx->gmEarthquakeThreshold = 0;

  ResetEffectsAndPlayerShots(ctx);
  ClearParticles(ctx);

  if (!ctx->gmBeaconActivated)
  {
    ctx->gmPlayerTookDamage = false;

    ctx->gmNumMovingMapParts = 0;
    for (i = 0; i < MAX_NUM_MOVING_MAP_PARTS; i++)
    {
      ctx->gmMovingMapParts[i].type = 0;
    }

    ctx->gmRequestUnlockNextDoor = false;
    ctx->plAirlockDeathStep = 0;
    ctx->gmRequestUnlockNextForceField = false;
    ctx->gmWaterAreasPresent = false;
    ctx->gmRadarDishesLeft = 0;
    ctx->plCollectedLetters = 0;
    ctx->plRapidFireTimeLeft = 0;
    ctx->gmReactorDestructionStep = 0;
    ctx->bdUseSecondary = false;
    ctx->plCloakTimeLeft = 0;
    ctx->gmCamerasDestroyed = ctx->gmCamerasInLevel = 0;
    ctx->gmWeaponsCollected = ctx->gmWeaponsInLevel = 0;
    ctx->gmMerchCollected = ctx->gmMerchInLevel = 0;
    ctx->gmTurretsDestroyed = ctx->gmTurretsInLevel = 0;
    ctx->gmNumActors = 0;
    ctx->plHealth = PLAYER_MAX_HEALTH;
    ctx->gmOrbsLeft = 0;
    ctx->gmBombBoxesLeft = 0;
  }
}


void CenterViewOnPlayer(Context* ctx)
{
  ctx->gmCameraPosX = ctx->plPosX - (VIEWPORT_WIDTH / 2 - 1);

  if ((int16_t)ctx->gmCameraPosX < 0)
  {
    ctx->gmCameraPosX = 0;
  }
  else if (ctx->gmCameraPosX > ctx->mapWidth - VIEWPORT_WIDTH)
  {
    ctx->gmCameraPosX = ctx->mapWidth - VIEWPORT_WIDTH;
  }

  ctx->gmCameraPosY = ctx->plPosY - (VIEWPORT_HEIGHT - 1);

  if ((int16_t)ctx->gmCameraPosY < 0)
  {
    ctx->gmCameraPosY = 0;
  }
  else if (ctx->gmCameraPosY > ctx->mapBottom - (VIEWPORT_HEIGHT + 1))
  {
    ctx->gmCameraPosY = ctx->mapBottom - (VIEWPORT_HEIGHT + 1);
  }
}


bool CheckDifficultyMarker(Context* ctx, word id)
{
  if (
    (id == ACT_META_MEDIUMHARD_ONLY && ctx->gmDifficulty == DIFFICULTY_EASY) ||
    (id == ACT_META_HARD_ONLY && ctx->gmDifficulty != DIFFICULTY_HARD))
  {
    return true;
  }

  return false;
}


/** Spawn actors that appear in the current level */
void SpawnLevelActors(Context* ctx)
{
  int16_t i;
  int16_t currentDrawIndex;
  int16_t drawIndex;
  word offset;
  word x;
  word y;
  word actorId;

  // The draw index is a means to make certain actors always appear in front of
  // or behind other types of actors, regardless of their position in the actor
  // list (which normally defines the order in which actors are drawn).
  //
  // The way it works is that we do multiple passes over the actor list in the
  // level file, and only spawn the actors during each pass which match the
  // draw index for that pass.
  //
  // Notably, this only works for actors that appear at the start of the level.
  // Any actors that are spawned during gameplay will be placed at wherever a
  // free slot in the actor list can be found, so their draw order is basically
  // random (it's still deterministic but depends on what has happened so far
  // during gameplay, so in practice, it very much appears to be random).
  for (currentDrawIndex = -1; currentDrawIndex < 4; currentDrawIndex++)
  {
    // ctx->levelActorListSize is the number of words, hence we multiply by 2.
    // Each actor specification is 3 words long, hence we add 6 to i on each
    // iteration.
    for (i = 0; i < ctx->levelActorListSize * 2; i += 6)
    {
      actorId = READ_LVL_ACTOR_DESC_ID(i);

      // Skip actors that don't appear in the currently chosen difficulty
      if (CheckDifficultyMarker(ctx, actorId))
      {
        i += 6;
        continue;
      }

      offset = ctx->gfxActorInfoData[actorId];

      drawIndex = AINFO_DRAW_INDEX(offset);

      if (drawIndex == currentDrawIndex)
      {
        x = READ_LVL_ACTOR_DESC_X(i);
        y = READ_LVL_ACTOR_DESC_Y(i);

        if (SpawnActorInSlot(ctx, ctx->gmNumActors, actorId, x, y))
        {
          ctx->gmNumActors++;
        }
      }
    }
  }
}


void relayInput(const PlayerInput& input, Context* ctx)
{
  ctx->inputMoveUp = input.mUp;
  ctx->inputMoveDown = input.mDown;
  ctx->inputMoveLeft = input.mLeft;
  ctx->inputMoveRight = input.mRight;
  ctx->inputFire = input.mFire.mIsPressed || input.mFire.mWasTriggered;
  ctx->inputJump = input.mJump.mIsPressed || input.mJump.mWasTriggered;
}


void relayPlayerModel(const data::PlayerModel& playerModel, Context* ctx)
{
  ctx->plWeapon = static_cast<byte>(playerModel.weapon());
  ctx->plScore = dword(playerModel.score());
  ctx->plAmmo = byte(playerModel.ammo());
  ctx->plHealth = byte(playerModel.health());
}

} // namespace


struct GameWorld_Classic::QuickSaveData
{
  QuickSaveData(
    const data::PlayerModel& playerModel,
    const data::map::Map& map,
    const Context& state)
    : mPlayerModel(playerModel)
    , mMap(map)
    , mState(state)
  {
  }

  data::PlayerModel mPlayerModel;
  data::map::Map mMap;
  Context mState;
};


GameWorld_Classic::GameWorld_Classic(
  data::PlayerModel* pPlayerModel,
  const data::GameSessionId& sessionId,
  GameMode::Context context,
  std::optional<base::Vec2> playerPositionOverride,
  bool showWelcomeMessage,
  const PlayerInput& initialInput)
  : mpRenderer(context.mpRenderer)
  , mpServiceProvider(context.mpServiceProvider)
  , mUiSpriteSheet(
      renderer::Texture{mpRenderer, context.mpResources->loadUiSpriteSheet()},
      data::GameTraits::viewportSize,
      mpRenderer)
  , mTextRenderer(&mUiSpriteSheet, mpRenderer, *context.mpResources)
  , mpPlayerModel(pPlayerModel)
  , mpOptions(&context.mpUserProfile->mOptions)
  , mpResources(context.mpResources)
  , mpSpriteFactory(context.mpSpriteFactory)
  , mSessionId(sessionId)
  , mPlayerModelAtLevelStart(*mpPlayerModel)
  , mHudRenderer(
      sessionId.mLevel + 1,
      mpOptions,
      mpRenderer,
      &mUiSpriteSheet,
      renderer::Texture{
        mpRenderer,
        context.mpResources->loadWideHudFrameImage()},
      renderer::Texture{
        mpRenderer,
        context.mpResources->loadUltrawideHudFrameImage()},
      mpSpriteFactory)
  , mMessageDisplay(mpServiceProvider, &mTextRenderer)
  , mSpecialEffects(mpRenderer, *mpOptions)
  , mLowResLayer(
      mpRenderer,
      renderer::determineWidescreenViewport(mpRenderer).mWidthPx,
      data::GameTraits::viewportHeightPx)
  , mPreviousWindowSize(mpRenderer->windowSize())
  , mpContext(std::make_unique<Context>())
{
  mpContext->pRigelBridge = &mBridge;

  MM_Init(mpContext.get());

  InitParticleSystem(mpContext.get());

  {
    const auto actorInfo = mpResources->file("ACTRINFO.MNI");
    mpContext->gfxActorInfoData =
      (word*)MM_PushChunk(mpContext.get(), word(actorInfo.size()), CT_COMMON);
    std::memcpy(
      mpContext->gfxActorInfoData, actorInfo.data(), actorInfo.size());
  }

  mpContext->gmBeaconActivated = false;
  ResetGameState(mpContext.get());

  mpContext->gmCurrentLevel = byte(sessionId.mLevel);
  mpContext->gmCurrentEpisode = byte(sessionId.mEpisode);
  mpContext->gmDifficulty = byte(sessionId.mDifficulty) + 1;

  relayPlayerModel(*mpPlayerModel, mpContext.get());

  loadLevel(sessionId);

  mBridge.mpMap = &mMap;
  mBridge.mpServiceProvider = mpServiceProvider;
  mBridge.mpMessageDisplay = &mMessageDisplay;
  mBridge.mpPlayerModel = mpPlayerModel;

  if (playerPositionOverride)
  {
    mpContext->plPosX = playerPositionOverride->x;
    mpContext->plPosY = playerPositionOverride->y;
  }

  CenterViewOnPlayer(mpContext.get());

  relayInput(initialInput, mpContext.get());
  UpdateAndDrawGame(mpContext.get());

  // TODO: welcome msg/radar message

  mpServiceProvider->playMusic(mMusicFile);
}


GameWorld_Classic::~GameWorld_Classic() = default;


bool GameWorld_Classic::levelFinished() const
{
  return mpContext->gmGameState == GS_LEVEL_FINISHED ||
    mpContext->gmGameState == GS_EPISODE_FINISHED;
}


std::set<data::Bonus> GameWorld_Classic::achievedBonuses() const
{
  return {};
}


bool GameWorld_Classic::needsPerElementUpscaling() const
{
  return false;
}


void GameWorld_Classic::updateGameLogic(const PlayerInput& input)
{
  mMapRenderer->updateAnimatedMapTiles();

  // Run original logic

  mBridge.mSpritesToDraw.clear();
  mBridge.mPixelsToDraw.clear();
  mBridge.mTileDebrisToDraw.clear();
  mBridge.mRadarDots.clear();

  const auto beaconWasActive = mpContext->gmBeaconActivated;

  relayInput(input, mpContext.get());
  UpdateAndDrawGame(mpContext.get());

  mHudRenderer.updateAnimation();
  mMessageDisplay.update();

  if (!mpContext->gmIsTeleporting)
  {
    syncBackdrop();
  }

  syncPlayerModel(mpContext.get(), *mpPlayerModel);

  if (mpContext->gmBeaconActivated && !beaconWasActive)
  {
    mCheckpointState = mpPlayerModel->makeCheckpoint();
  }
}


void GameWorld_Classic::render(float)
{
  // For C macros used below
  auto ctx = mpContext.get();

  {
    auto saved = setupIngameViewport(mpRenderer, mBridge.mScreenShift);

    if (mpContext->gfxFlashScreen)
    {
      mpRenderer->clear(SCREEN_FLASH_COLORS[mpContext->gfxScreenFlashColor]);
    }
    else
    {
      const auto region = base::Rect<int>{
        {mpContext->gmCameraPosX, mpContext->gmCameraPosY},
        data::GameTraits::mapViewportSize};

      mMapRenderer->renderBackdrop(
        base::cast<float>(region.topLeft), region.size);
      mMapRenderer->renderDynamicSection(
        mMap, region, {}, engine::MapRenderer::DrawMode::Background);


      auto drawSprite = [&](const SpriteDrawCmd& request) {
        const auto offset =
          mpContext->gfxActorInfoData[request.id] + request.frame * 8;
        const auto topLeft = base::Vec2{request.x, request.y} -
          base::Vec2{mpContext->gmCameraPosX, mpContext->gmCameraPosY} -
          base::Vec2{0, AINFO_HEIGHT(offset) - 1} +
          base::Vec2{AINFO_X_OFFSET(offset), AINFO_Y_OFFSET(offset)};

        mpSpriteFactory->textureAtlas().draw(
          mpSpriteFactory->actorFrameImageId(
            static_cast<data::ActorID>(request.id), request.frame),
          {data::tilesToPixels(topLeft),
           data::tilesToPixels(
             base::Size{AINFO_WIDTH(offset), AINFO_HEIGHT(offset)})});
      };


      for (const auto& request : mBridge.mSpritesToDraw)
      {
        if (
          request.drawStyle == DS_INVISIBLE || request.drawStyle == DS_IN_FRONT)
        {
          continue;
        }

        if (request.drawStyle == DS_WHITEFLASH)
        {
          const auto innerGuard = renderer::saveState(mpRenderer);
          mpRenderer->setOverlayColor(data::GameTraits::INGAME_PALETTE[15]);
          drawSprite(request);
        }
        else if (request.drawStyle == DS_TRANSLUCENT)
        {
        }
        else
        {
          drawSprite(request);
        }
      }

      mMapRenderer->renderDynamicSection(
        mMap, region, {}, engine::MapRenderer::DrawMode::Foreground);

      for (const auto& request : mBridge.mSpritesToDraw)
      {
        if (
          request.drawStyle == DS_INVISIBLE || request.drawStyle != DS_IN_FRONT)
        {
          continue;
        }

        if (request.drawStyle == DS_WHITEFLASH)
        {
          const auto innerGuard = renderer::saveState(mpRenderer);
          mpRenderer->setOverlayColor(data::GameTraits::INGAME_PALETTE[15]);
          drawSprite(request);
        }
        else if (request.drawStyle == DS_TRANSLUCENT)
        {
        }
        else
        {
          drawSprite(request);
        }
      }

      for (const auto& request : mBridge.mTileDebrisToDraw)
      {
        mMapRenderer->renderSingleTile(
          request.tileIndex / 8,
          data::tilesToPixels(base::Vec2{request.x, request.y}));
      }


      for (const auto& request : mBridge.mPixelsToDraw)
      {
        mpRenderer->drawPoint(
          {request.x, request.y},
          data::GameTraits::INGAME_PALETTE[request.color]);
      }
    }

    mHudRenderer.renderClassicHud(*mpPlayerModel, mBridge.mRadarDots);
  }

  auto saved = renderer::saveState(mpRenderer);
  renderer::setLocalTranslation(
    mpRenderer,
    {mBridge.mScreenShift + data::GameTraits::inGameViewportOffset.x, 0});
  mMessageDisplay.render();
}


void GameWorld_Classic::processEndOfFrameActions()
{
  if (mpContext->gmIsTeleporting)
  {
    mpServiceProvider->fadeOutScreen();

    mpContext->plPosY = mpContext->gmTeleportTargetPosY;
    mpContext->plPosX = mpContext->gmTeleportTargetPosX + 1;
    CenterViewOnPlayer(mpContext.get());

    syncBackdrop();

    relayInput({}, mpContext.get());
    UpdateAndDrawGame(mpContext.get());
    render(1.0f);

    mpServiceProvider->fadeInScreen();

    mpContext->gmIsTeleporting = false;
  }

  if (mpContext->gmGameState == GS_PLAYER_DIED)
  {
    mpServiceProvider->fadeOutScreen();

    ResetGameState(mpContext.get());

    if (mpContext->gmBeaconActivated)
    {
      mpPlayerModel->restoreFromCheckpoint(*mCheckpointState);

      mpContext->plPosX = mpContext->gmBeaconPosX;
      mpContext->plPosY = mpContext->gmBeaconPosY;
      mpContext->plActorId = ACT_DUKE_R;
    }
    else
    {
      *mpPlayerModel = mPlayerModelAtLevelStart;
      loadLevel(mSessionId);
    }

    syncBackdrop();

    relayPlayerModel(*mpPlayerModel, mpContext.get());

    CenterViewOnPlayer(mpContext.get());

    relayInput({}, mpContext.get());
    UpdateAndDrawGame(mpContext.get());
    render(1.0f);

    mpServiceProvider->fadeInScreen();
  }

  mBridge.mScreenShift = 0;
}


void GameWorld_Classic::updateBackdropAutoScrolling(const engine::TimeDelta dt)
{
  mMapRenderer->updateBackdropAutoScrolling(dt);
}


void GameWorld_Classic::activateFullHealthCheat() { }


void GameWorld_Classic::activateGiveItemsCheat() { }


void GameWorld_Classic::quickSave()
{
  if (
    !mpOptions->mQuickSavingEnabled || mpContext->gmGameState == GS_PLAYER_DIED)
  {
    return;
  }

  mpQuickSave =
    std::make_unique<QuickSaveData>(*mpPlayerModel, mMap, *mpContext);

  mMessageDisplay.setMessage("Quick saved.", ui::MessagePriority::Menu);
}


void GameWorld_Classic::quickLoad()
{
  if (!canQuickLoad())
  {
    return;
  }

  *mpPlayerModel = mpQuickSave->mPlayerModel;
  mMap = mpQuickSave->mMap;
  *mpContext = mpQuickSave->mState;

  mMessageDisplay.setMessage("Quick save restored.", ui::MessagePriority::Menu);
}


bool GameWorld_Classic::canQuickLoad() const
{
  return mpOptions->mQuickSavingEnabled && mpQuickSave;
}


void GameWorld_Classic::loadLevel(const data::GameSessionId& sessionId)
{
  auto ctx = mpContext.get();

  {
    const auto levelDataRaw =
      mpResources->file(levelFileName(sessionId.mEpisode, sessionId.mLevel));

    assets::LeStreamReader reader(levelDataRaw);

    const dword headerSize = reader.readU16();

    if (headerSize >= sizeof(mpContext->levelHeaderData))
    {
      throw std::runtime_error("Invalid or corrupt level file");
    }

    std::memcpy(
      mpContext->levelHeaderData,
      levelDataRaw.data() + sizeof(std::uint16_t),
      headerSize);
    mpContext->levelActorListSize = READ_LVL_HEADER_WORD(43);

    mpContext->mapData =
      (word*)MM_PushChunk(mpContext.get(), 65500, CT_MAP_DATA);
    std::memcpy(
      mpContext->mapData,
      levelDataRaw.data() + headerSize + sizeof(std::uint16_t),
      65500);

    mpContext->gfxTilesetAttributes =
      (word*)MM_PushChunk(mpContext.get(), 3600, CT_CZONE);

    const auto czoneFile =
      mpResources->file(strings::trimRight(readFixedSizeString(reader, 13)));

    std::memcpy(mpContext->gfxTilesetAttributes, czoneFile.data(), 3600);
  }

  const auto levelData = assets::loadLevel(
    levelFileName(sessionId.mEpisode, sessionId.mLevel),
    *mpResources,
    sessionId.mDifficulty);

  mpContext->mapWidth = word(levelData.mMap.width());
  mpContext->mapWidthShift = word(std::log(mpContext->mapWidth) / std::log(2));
  mpContext->mapBottom = word(levelData.mMap.height() - 1);
  mpContext->mapViewportHeight = VIEWPORT_HEIGHT;

  using SM = data::map::BackdropScrollMode;
  using BSC = data::map::BackdropSwitchCondition;

  mpContext->mapParallaxBoth =
    levelData.mBackdropScrollMode == SM::ParallaxBoth;
  mpContext->mapParallaxHorizontal =
    levelData.mBackdropScrollMode == SM::ParallaxHorizontal;
  mpContext->mapBackdropAutoScrollX =
    levelData.mBackdropScrollMode == SM::AutoHorizontal;
  mpContext->mapBackdropAutoScrollY =
    levelData.mBackdropScrollMode == SM::AutoVertical;
  mpContext->mapHasEarthquake = levelData.mEarthquake;
  mpContext->mapHasReactorDestructionEvent =
    levelData.mBackdropSwitchCondition == BSC::OnReactorDestruction;
  mpContext->mapSwitchBackdropOnTeleport =
    levelData.mBackdropSwitchCondition == BSC::OnTeleportation;

  mMap = std::move(levelData.mMap);

  mMapRenderer.emplace(
    mpRenderer,
    data::map::Map{},
    &mMap.attributeDict(),
    engine::MapRenderer::MapRenderData{
      std::move(levelData.mTileSetImage),
      std::move(levelData.mBackdropImage),
      std::move(levelData.mSecondaryBackdropImage),
      levelData.mBackdropScrollMode});

  mMusicFile = levelData.mMusicFile;

  SpawnLevelActors(mpContext.get());
}


void GameWorld_Classic::syncBackdrop()
{
  if (mpContext->bdUseSecondary != mIsUsingSecondaryBackdrop)
  {
    mMapRenderer->switchBackdrops();
    mIsUsingSecondaryBackdrop = mpContext->bdUseSecondary;
  }
}


void GameWorld_Classic::syncPlayerModel(
  Context* ctx,
  data::PlayerModel& playerModel)
{
  using LT = data::CollectableLetterType;

  playerModel.mWeapon = static_cast<data::WeaponType>(ctx->plWeapon);
  playerModel.mScore = ctx->plScore;
  playerModel.mAmmo = ctx->plAmmo;
  playerModel.mHealth = ctx->plHealth;

  playerModel.mCollectedLetters.clear();

  if (ctx->plCollectedLetters & 0x100)
  {
    playerModel.mCollectedLetters.push_back(LT::N);
  }

  if (ctx->plCollectedLetters & 0x200)
  {
    playerModel.mCollectedLetters.push_back(LT::U);
  }

  if (ctx->plCollectedLetters & 0x400)
  {
    playerModel.mCollectedLetters.push_back(LT::K);
  }

  if (ctx->plCollectedLetters & 0x800)
  {
    playerModel.mCollectedLetters.push_back(LT::E);
  }

  if (ctx->plCollectedLetters & 0x1000)
  {
    playerModel.mCollectedLetters.push_back(LT::M);
  }
}


} // namespace rigel::game_logic

RIGEL_RESTORE_WARNINGS

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

#pragma once

#include "base/color.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/bonus.hpp"
#include "data/game_session_data.hpp"
#include "data/map.hpp"
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/graphical_effects.hpp"
#include "engine/map_renderer.hpp"
#include "engine/sprite_factory.hpp"
#include "frontend/game_mode.hpp"
#include "frontend/igame_world.hpp"
#include "game_logic/input.hpp"
#include "ui/hud_renderer.hpp"
#include "ui/ingame_message_display.hpp"
#include "ui/menu_element_renderer.hpp"


struct Context_;


namespace rigel::game_logic
{


struct SpriteDrawCmd
{
  std::uint16_t id;
  std::uint16_t frame;
  std::uint16_t x;
  std::uint16_t y;
  std::uint16_t drawStyle;
};


struct PixelDrawCmd
{
  std::uint16_t x;
  std::uint16_t y;
  std::uint8_t color;
};


struct TileDrawCmd
{
  std::uint16_t tileIndex;
  std::uint16_t x;
  std::uint16_t y;
};


struct Bridge
{
  std::vector<SpriteDrawCmd> mSpritesToDraw;
  std::vector<PixelDrawCmd> mPixelsToDraw;
  std::vector<TileDrawCmd> mTileDebrisToDraw;
  std::vector<base::Vec2> mRadarDots;
  std::uint8_t mScreenShift;
  data::map::Map* mpMap;
  IGameServiceProvider* mpServiceProvider;
  ui::IngameMessageDisplay* mpMessageDisplay;
  data::PlayerModel* mpPlayerModel;
};


class GameWorld_Classic : public IGameWorld
{
public:
  GameWorld_Classic(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vec2> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false,
    const PlayerInput& initialInput = PlayerInput{});
  ~GameWorld_Classic() override;

  GameWorld_Classic(const GameWorld_Classic&) = delete;
  GameWorld_Classic& operator=(const GameWorld_Classic&) = delete;

  bool levelFinished() const override;
  std::set<data::Bonus> achievedBonuses() const override;

  bool needsPerElementUpscaling() const override;

  void updateGameLogic(const PlayerInput& input) override;
  void render(float interpolationFactor = 0.0f) override;
  void processEndOfFrameActions() override;
  void updateBackdropAutoScrolling(engine::TimeDelta dt) override;

  bool isPlayerInShip() const override { return false; }
  void toggleGodMode() override { }
  bool isGodModeOn() const override { return false; }

  void activateFullHealthCheat() override;
  void activateGiveItemsCheat() override;

  void quickSave() override;
  void quickLoad() override;
  bool canQuickLoad() const override;

  void debugToggleBoundingBoxDisplay() override { }
  void debugToggleWorldCollisionDataDisplay() override { }
  void debugToggleGridDisplay() override { }
  void printDebugText(std::ostream& stream) const override { }

private:
  void loadLevel(const data::GameSessionId& sessionId);
  void syncBackdrop();
  void syncPlayerModel(Context_* ctx, data::PlayerModel& playerModel);

  struct QuickSaveData;

  renderer::Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  engine::TiledTexture mUiSpriteSheet;
  ui::MenuElementRenderer mTextRenderer;
  data::PlayerModel* mpPlayerModel;
  const data::GameOptions* mpOptions;
  const assets::ResourceLoader* mpResources;
  engine::SpriteFactory* mpSpriteFactory;
  data::GameSessionId mSessionId;
  bool mIsUsingSecondaryBackdrop = false;
  std::string mMusicFile;

  data::map::Map mMap;
  std::optional<engine::MapRenderer> mMapRenderer;
  data::PlayerModel mPlayerModelAtLevelStart;
  ui::HudRenderer mHudRenderer;
  ui::IngameMessageDisplay mMessageDisplay;
  engine::SpecialEffectsRenderer mSpecialEffects;
  renderer::RenderTargetTexture mLowResLayer;
  base::Size mPreviousWindowSize;

  Bridge mBridge;
  std::unique_ptr<Context_> mpContext;
  std::unique_ptr<QuickSaveData> mpQuickSave;

  std::optional<data::PlayerModel::CheckpointState> mCheckpointState;
};

} // namespace rigel::game_logic

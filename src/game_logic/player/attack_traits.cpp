/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
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

#include "attack_traits.hpp"


namespace rigel { namespace game_logic { namespace player {

ProjectileDirection shotDirection(
  const PlayerState state,
  const Orientation orientation
) {
  if (state == PlayerState::LookingUp) {
    return ProjectileDirection::Up;
  } else {
    return orientation == Orientation::Right
      ? ProjectileDirection::Right
      : ProjectileDirection::Left;
  }
}

}}}

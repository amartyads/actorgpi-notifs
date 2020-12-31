/**
 * @file
 * This file is part of Pond.
 *
 * @author Alexander Pöppl (poeppl AT in.tum.de, https://www5.in.tum.de/wiki/index.php/Alexander_P%C3%B6ppl,_M.Sc.)
 *
 * @section LICENSE
 *
 * Pond is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pond is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Pond.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * @section DESCRIPTION
 *
 * TODO
 */

//#include "actorlib/utils/mpi_consts.hpp"

#include <GASPI.h>

#include <cstddef>
#include <memory>
#include <vector>

#pragma once

struct Coordinates{
    Coordinates(int x, int y) : x(x), y(y) {}
    int x;
    int y;
};

class ActorDistributor {
public:
  ActorDistributor(size_t xSize, size_t ySize);

  // Due to the allocated memory, we don't allow assignment to avoid
  // zombies/leaks
  ActorDistributor(ActorDistributor &) = delete;
  ActorDistributor &operator=(ActorDistributor &) = delete;

  std::vector<Coordinates> getLocalActorCoordinates() const;
  std::vector<Coordinates> getActorCoordinates() const;

  auto getXSize() const { return xSize; }
  auto getYSize() const { return ySize; }

  gaspi_rank_t getRankFor(Coordinates c) const {
    return actorDistribution[c.x * ySize + c.y];
  }

  virtual ~ActorDistributor();

protected:
  std::string toString() const;

  size_t xSize;
  size_t ySize;

  std::vector<gaspi_rank_t> actorDistribution;
};

std::unique_ptr<ActorDistributor> createActorDistributor(size_t xSize,
                                                         size_t ySize);

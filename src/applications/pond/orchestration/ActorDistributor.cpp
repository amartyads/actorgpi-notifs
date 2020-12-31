/**
 * @file
 * This file is part of Pond.
 *
 * @author Michael Bader, Kaveh Rahnema, Tobias Schnabel
 * @author Sebastian Rettenberger (rettenbs AT in.tum.de, http://www5.in.tum.de/wiki/index.php/Sebastian_Rettenberger,_M.Sc.)
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

#include "ActorDistributor.hpp"

#include <cmath>
#include <iomanip>
#include "actorlib/utils/gpi_utils.hpp"

#if defined(METIS_PARTITIONING)
#include "orchestration/MetisActorDistributor.hpp"
#elif defined(EIGEN3_PARTITIONING)
#include "orchestration/FiedlerVectorActorDistributor.hpp"
#else
#include "orchestration/SimpleActorDistributor.hpp"
#endif

ActorDistributor::ActorDistributor(size_t xSize, size_t ySize)
    : xSize(xSize), ySize(ySize) {
  actorDistribution.resize(xSize*ySize);
}

ActorDistributor::~ActorDistributor() = default;

std::string ActorDistributor::toString() const {
  std::stringstream ss;
  gaspi_rank_t places = gpi_util::get_total_ranks();
  int digits = ceil(std::log10(places - 1));
  ss << "\n";
  for (size_t y = 0; y < xSize; y++) {
    for (size_t x = 0; x < ySize; x++) {
      ss << std::setw(digits) << actorDistribution[y * xSize + x] << " ";
    }
    ss << std::endl;
  }
  return ss.str();
}

std::vector<Coordinates> ActorDistributor::getLocalActorCoordinates() const {
  std::vector<Coordinates> res;
  for (size_t x = 0; x < xSize; x++) {
    for (size_t y = 0; y < ySize; y++) {
      if (actorDistribution[x * ySize + y] == gpi_util::get_local_rank()) {
        res.emplace_back(Coordinates(x, y));
      }
    }
  }
  return res;
}

std::vector<Coordinates> ActorDistributor::getActorCoordinates() const {
  std::vector<Coordinates> res;
  for (size_t x = 0; x < xSize; x++) {
    for (size_t y = 0; y < ySize; y++) {
      res.emplace_back(Coordinates(x, y));
    }
  }
  return res;
}

std::unique_ptr<ActorDistributor> createActorDistributor(size_t xSize,
                                                         size_t ySize) {
#if defined(METIS_PARTITIONING)
  return std::make_unique<MetisActorDistributor>(xSize, ySize);
#elif defined(EIGEN3_PARTITIONING)
  return std::make_unique<FiedlerVectorActorDistributor>(xSize, ySize);
#else
  return std::make_unique<SimpleActorDistributor>(xSize, ySize);
#endif
}
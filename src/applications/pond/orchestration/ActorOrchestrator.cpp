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

#include "orchestration/ActorOrchestrator.hpp"

#include "actor/SimulationActor.hpp"
#include "orchestration/ActorDistributor.hpp"
#include "util/Configuration.hpp"
#include "util/Logger.hh"
#include "actorlib/utils/gen_utils.hpp"
//#include "actorlib/utils/mpi_synchronization.hpp"

#include "actorlib/utils/gpi_utils.hpp"

#include <chrono>
#include <vector>
#include <algorithm>
#include <limits>
#include <sstream>
#include <iostream>

#ifndef ASSERT
#define ASSERT(ec) gpi_util::success_or_exit(__FILE__,__LINE__,ec)
#endif

static tools::Logger &l = tools::Logger::logger;

ActorOrchestrator::ActorOrchestrator(Configuration config)
    : config(config) {
}

void ActorOrchestrator::initActorGraph() {
    size_t xActors = config.xSize / config.patchSize;
    size_t yActors = config.ySize / config.patchSize;
    auto sd = createActorDistributor(xActors, yActors);

    createActors(sd.get());
    ag.synchronizeActors();

#ifndef NDEBUG
    l.printString(utils::to_string(ag));
#endif
    //ag.printActors();
    connectActors(sd.get());

    ASSERT( gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK) );

    initializeActors();
}

void ActorOrchestrator::createActors(ActorDistributor *ad) {
  auto localActorCoords = ad->getLocalActorCoordinates();
  for (auto &coordPair : localActorCoords) {
    auto actor = new SimulationActor(config, coordPair.x, coordPair.y);
    ag.addActor(actor);
    localActors.push_back(actor);
  }
}

void ActorOrchestrator::connectActors(ActorDistributor *ad) {
    auto coords = ad->getActorCoordinates();
  for (auto &coord : coords) {
    this->connectToNeighbors(coord, ad->getXSize(), ad->getYSize());
  }
}

void ActorOrchestrator::initializeActors() {
    float safeTimestep{std::numeric_limits<float>::infinity()};
    for (SimulationActor *a : localActors) {
        a->initializeBlock();
        auto blockSafeTs = a->getMaxBlockTimestepSize();
        safeTimestep = std::min(safeTimestep, blockSafeTs);
#ifndef NDEBUG
        std::cout << "Safe timestep on actor " << a->getName() << " is " << blockSafeTs << " current min: " << safeTimestep << std::endl;
#endif
    }
    float globalSafeTs = 0.0;
    gaspi_pointer_t local = &safeTimestep;
    gaspi_pointer_t global = &globalSafeTs;
    ASSERT( gaspi_allreduce(local, global, 1, GASPI_OP_MIN, GASPI_TYPE_FLOAT, GASPI_GROUP_ALL, GASPI_BLOCK) );

    //globalSafeTs = mpi::allReduce(safeTimestep, MPI_MIN);
#ifndef NDEBUG
    l.cout() << "Received safe timestep: " << globalSafeTs << " local was " << safeTimestep << std::endl;
#endif
    for (SimulationActor *a : localActors) {
        a->setTimestepBaseline(globalSafeTs);
    }
}

void ActorOrchestrator::simulate() {
    l.printString("********************* Start Simulation **********************", false);
    auto runTime = ag.run();
    l.printString("********************** End Simulation ***********************", false);
    uint64_t localPatchUpdates = 0;
    for (SimulationActor *a : localActors) {
        localPatchUpdates += a->getNumberOfPatchUpdates();
    }
    uint64_t totalPatchUpdates = 0;
    gaspi_pointer_t local = &localPatchUpdates;
    gaspi_pointer_t global = &totalPatchUpdates;
    ASSERT( gaspi_allreduce(local, global, 1, GASPI_OP_SUM, GASPI_TYPE_ULONG, GASPI_GROUP_ALL, GASPI_BLOCK) );//maybe keep mpi? since ulong?
    //totalPatchUpdates = mpi::allReduce(localPatchUpdates, MPI_SUM);
    if (!gpi_util::get_local_rank()) {
        l.cout() << "Performed " << totalPatchUpdates << " patch updates in " << runTime << " seconds." << std::endl;
        l.cout() << "Performed " << (totalPatchUpdates * config.patchSize * config.patchSize) << " cell updates in " << runTime << " seconds." << std::endl;
        l.cout() << "=> " <<  (static_cast<double>(totalPatchUpdates * config.patchSize * config.patchSize) / runTime)<< " CellUpdates/s" << std::endl;
        l.cout() << "=> " << (static_cast<double>(135 * 2 * totalPatchUpdates * config.patchSize * config.patchSize) / runTime) << " FlOps/s" << std::endl;
    }
}

void ActorOrchestrator::connectToNeighbors(Coordinates coords, size_t xActors,
                                           size_t yActors) {
  // All ranks will call this function with the same parameters

  auto simArea = makePatchArea(config, coords.x, coords.y);
  auto &curActor = ag.getActor(simArea.toString());

  auto connect = [&](Actor &oa, const std::string &op, Actor &ia,
                     const std::string &ip) {
#ifndef NDEBUG
    l.cout(false) << "Connecting " << oa << " (" << op << ") -> (" << ip << ") "
                  << ia << std::endl;
#endif
    ag.connectPorts<std::vector<float>, 32>(oa, op, ia, ip);
                                          //  3 * config.patchSize); communicationCount?
  };
  
  if (coords.x > 0) {
    auto leftSimArea = makePatchArea(config, coords.x - 1, coords.y);
    auto &leftActor = ag.getActor(leftSimArea.toString());
    connect(curActor, "BND_LEFT", leftActor, "BND_RIGHT");
  }
  if (coords.x < xActors - 1) {
    auto rightSimArea = makePatchArea(config, coords.x + 1, coords.y);
    auto &rightActor = ag.getActor(rightSimArea.toString());
    connect(curActor, "BND_RIGHT", rightActor, "BND_LEFT");
  }
  if (coords.y > 0) {
    auto bottomSimArea = makePatchArea(config, coords.x, coords.y - 1);
    auto &bottomActor = ag.getActor(bottomSimArea.toString());
    connect(curActor, "BND_BOTTOM", bottomActor, "BND_TOP");
  }
  if (coords.y < yActors - 1) {
    auto topSimArea = makePatchArea(config, coords.x, coords.y + 1);
    auto &topActor = ag.getActor(topSimArea.toString());
    connect(curActor, "BND_TOP", topActor, "BND_BOTTOM");
  }
}

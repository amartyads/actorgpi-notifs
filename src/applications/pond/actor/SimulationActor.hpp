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

#include "actorlib/Actor.hpp"

#include "block/SWE_Block.hh"
#include "block/SWE_WaveAccumulationBlock.hh"
#include "block/BlockCommunicator.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

#pragma once

class Configuration;

namespace io {
    class Writer;
}

enum class SimulationActorState {
    INITIAL, 
    RUNNING,
    FINISHED,
    TERMINATED
};

class SimulationActor : public Actor {

    private:
        const Configuration &config;
        const size_t position[2];
        SWE_WaveAccumulationBlock block;
        InPort<std::vector<float>, 32> *dataIn[4];
        OutPort<std::vector<float>, 32> *dataOut[4];
        BlockCommunicator communicators[4];
        SimulationActorState currentState;
        float currentTime;
        float timestepBaseline;
        float outputDelta;
        float nextWriteTime;
        float endTime;
        bool portsLoaded;
        uint64_t patchUpdates;
        io::Writer *writer;

    public:
        const SimulationArea patchArea;

    public:
        SimulationActor(Configuration &config, size_t xPos, size_t yPos);
        ~SimulationActor();

        void initializeBlock();
        float getMaxBlockTimestepSize();
        void setTimestepBaseline(float timestepBaseline);
        void initializeBoundary(BoundaryEdge edge, std::function<bool()>);
        void act() override;
        uint64_t getNumberOfPatchUpdates();

    private:
        void computeWriteDelta();
        void performComputationStep();
        void sendData();
        void receiveData();
        void sendTerminationSignal();
        bool hasReceivedTerminationSignal();
        bool mayWrite();
        bool mayRead();
        void writeTimeStep(float currentTime);
        void loadPorts();
};

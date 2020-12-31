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

#include "block/BlockCommunicator.hpp"
#include "block/SWE_Block.hh"

#include <vector>
#include <cassert>
#include <cstddef>

using namespace std;

BlockCommunicator::BlockCommunicator() 
    : copyLayer(nullptr),
      ghostLayer(nullptr),
      patchSize(0) {
}

BlockCommunicator::BlockCommunicator(size_t patchSize, SWE_Block1D *copyLayer, SWE_Block1D *ghostLayer)
    : copyLayer(copyLayer),
      ghostLayer(ghostLayer),
      patchSize(patchSize) {
}

vector<float> BlockCommunicator::packCopyLayer() {
    assert(patchSize > 0);
    vector<float> res;
    res.reserve(3 * patchSize);

    for (size_t i = 0; i < patchSize; i++) {
        res.push_back(copyLayer->h[i + 1]);
    }

    for (size_t i = 0; i < patchSize; i++) {
        res.push_back(copyLayer->hu[i + 1]);
    }

    for (size_t i = 0; i < patchSize; i++) {
        res.push_back(copyLayer->hv[i + 1]);
    }
    return res;
}

void BlockCommunicator::receiveGhostLayer(vector<float> ghostLayerBuffer) {
    assert(patchSize > 0);
    assert(ghostLayerBuffer.size() == 3 * this->patchSize);

    for (size_t i = 0; i < this->patchSize; i++) {
        ghostLayer->h[i + 1] = ghostLayerBuffer[i];
    }

    for (size_t i = 0; i < this->patchSize; i++) {
        ghostLayer->hu[i + 1] = ghostLayerBuffer[patchSize + i];
    }

    for (size_t i = 0; i < this->patchSize; i++) {
        ghostLayer->hv[i + 1] = ghostLayerBuffer[2 * patchSize + i];
    }
}

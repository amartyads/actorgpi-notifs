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

#include <vector>
#include <cstddef>

#pragma once

class SWE_Block1D;

struct BlockCommunicator {
    std::vector<float> sendBuffer;
    SWE_Block1D *copyLayer;
    SWE_Block1D *ghostLayer;
    size_t patchSize;

    BlockCommunicator();
    BlockCommunicator(size_t patchSize, SWE_Block1D *copyLayer, SWE_Block1D *ghostLayer);

    std::vector<float> packCopyLayer();
    void receiveGhostLayer(std::vector<float> ghostLayerBuffer);
};

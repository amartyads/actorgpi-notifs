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

#include "scenario/ScalablePoolDropScenario.hpp"

#include <vector>
#include <cmath>

ScalablePoolDropScenario::ScalablePoolDropScenario(float xScale, float yScale, double end)
    : endTime(end), 
      xScale(xScale),
      yScale(yScale) {
}


float ScalablePoolDropScenario::getBathymetry(float x, float y) const {
    return 0.0f;
}

float ScalablePoolDropScenario::getWaterHeight(float x, float y) const {
    float xMid = 50.0f * xScale;
    float yMid = 50.0f * yScale;
    float pos = std::sqrt((x - xMid) * (x - xMid) + (y - yMid) * (y - yMid));
    return (pos < 40.0f) ? 12.0f : 10.0f;
}

float ScalablePoolDropScenario::endSimulation() const {
    return this->endTime;
}

float ScalablePoolDropScenario::getBoundaryPos(BoundaryEdge e) const {
    switch (e) {
    case BND_TOP:
        return 100.0f * yScale;
    case BND_RIGHT:
        return 100.f * xScale;
    default:
        return 0.0f;
    }
}

BoundaryType ScalablePoolDropScenario::getBoundaryType(BoundaryEdge e) const {
    return WALL;
}

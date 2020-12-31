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
#include <GASPI.h>
#include "actorlib/utils/gpi_utils.hpp"
#include "orchestration/SimpleActorDistributor.hpp"

#include "util/Logger.hh"

#include <cmath>
#ifndef ASSERT
#define ASSERT(ec) gpi_util::success_or_exit(__FILE__,__LINE__,ec)
#endif
static tools::Logger &l = tools::Logger::logger;

SimpleActorDistributor::SimpleActorDistributor(size_t xSize, size_t ySize)
    : ActorDistributor(xSize, ySize) {
    gaspi_rank_t totProc = gpi_util::get_total_ranks();
    auto tmp1 = static_cast<double>(xSize) / std::sqrt(static_cast<double>(totProc));
    l.cout() << xSize << "/ sqrt(" <<totProc << ") = " << tmp1 << std::endl;
    double xBlockSize = std::floor(tmp1);
    l.cout() << xBlockSize << std::endl;
    size_t xSplits = std::max(xSize / static_cast<size_t>(xBlockSize), 1ul);
    size_t ySplits = totProc / xSplits;
#ifndef NDEBUG
    l.cout() <<"xSplits: " << xSplits << " ySplits: " << ySplits << std::endl;
#endif

    putInitial(xSplits, ySplits);
    l.printString(toString());
}

void SimpleActorDistributor::putInitial(size_t xSplits, size_t ySplits) {
    gaspi_rank_t totProc = gpi_util::get_total_ranks();
    for (size_t x = 0; x < xSize; x++) {
        for (size_t y = 0; y < ySize; y++) {
            auto tmpX = x / (xSize / xSplits);
            auto tmpY = y / (ySize / ySplits);
            auto res = static_cast<gaspi_rank_t>(tmpX * ySplits + tmpY);
            actorDistribution[x * ySize + y] = std::min((int)res, totProc - 1);
        }
    }
}
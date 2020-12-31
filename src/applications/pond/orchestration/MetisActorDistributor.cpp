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

#include "orchestration/MetisActorDistributor.hpp"
#include "util/Logger.hh"
#include "actorlib/utils/mpi_synchronization.hpp"
#include "actorlib/utils/gpi_utils.hpp"
#include <GASPI.h>

extern "C" {
#include <metis.h>
}

#include <cassert>
#include <cmath>
#include <sstream>
#include <vector>

static tools::Logger &l = tools::Logger::logger;

struct MetisState {
    idx_t metisOptions[METIS_NOPTIONS];

    MetisState() {
        METIS_SetDefaultOptions(metisOptions);
        metisOptions[METIS_OPTION_PTYPE] = METIS_PTYPE_KWAY;
        metisOptions[METIS_OPTION_NUMBERING] = 0; /* C-Style numbering starting with 0 */
        metisOptions[METIS_OPTION_IPTYPE] = METIS_IPTYPE_GROW; /* C-Style numbering starting with 0 */
        metisOptions[METIS_OPTION_CONTIG] = 1; /* Try to produce contiguous partitions. */
        metisOptions[METIS_OPTION_NCUTS] = 2; /* Try 2 times, take the best */
        metisOptions[METIS_OPTION_NITER] = 2; /* Try 2 times in each refinement step take the best */
        metisOptions[METIS_OPTION_UFACTOR] = 100; /* Max imbalance of 10% */
#ifndef NDEBUG
        metisOptions[METIS_OPTION_DBGLVL] = ((gpi_util::get_local_rank() == 0) ? (METIS_DBG_INFO) : 0);
#endif
    }
};

template <typename MetisIdx, typename Idx>
void moveFrom(std::vector<MetisIdx> &from, std::vector<Idx> &to) {
    l.cout(false) << "Size difference in the size of idx_t and mpi::rank. Copying and casting." << std::endl;
    for (idx_t i : from) {
        to.push_back(static_cast<Idx>(i));
    }
}

template<typename Idx>
void moveFrom(std::vector<Idx> &from, std::vector<Idx> &to) {
    l.cout(false) << "Directly moving the data into the destination, as the size is the same" << std::endl;
    to = std::move(from);
}

struct MetisAdjacencyGraph {

    size_t xSize;
    size_t ySize;

    std::vector<idx_t> vertexAdjacencyListStarts;
    std::vector<idx_t> vertexAdjacencies;

    MetisAdjacencyGraph(size_t xSize, size_t ySize, std::vector<gaspi_rank_t> &partitions)
        : xSize(xSize),
          ySize(ySize) {
        idx_t numAdjacencies = 0;
        for (size_t y = 0; y < ySize; y++) {
            for (size_t x = 0; x < xSize; x++) {
                vertexAdjacencyListStarts.push_back(numAdjacencies);
                int numNeighbors = 0;
                if (x > 0) {
                    numNeighbors++;
                    vertexAdjacencies.push_back(y * xSize + (x - 1));
                }
                
                if (x < xSize - 1) {
                    numNeighbors++;
                    vertexAdjacencies.push_back(y * xSize + (x + 1));
                }

                if (y > 0) {
                    numNeighbors++;
                    vertexAdjacencies.push_back((y-1) * xSize + x);
                }

                if (y < ySize - 1) {
                    numNeighbors++;
                    vertexAdjacencies.push_back((y+1) * xSize + x);
                }
                numAdjacencies += numNeighbors;
            }
        }
        vertexAdjacencyListStarts.push_back(numAdjacencies);
    }

    std::vector<gaspi_rank_t> partition(MetisState *metisOptions) {
        idx_t numberOfVertices = xSize * ySize;
        idx_t numberOfConstraints = 1;
        idx_t numberOfPartitions = gpi_util::get_total_ranks();
        idx_t finalEdgeCut = 0;
        std::vector<idx_t> partitions(xSize * ySize, 0);
        int metisResult;
        if (gpi_util::get_total_ranks() > 1) {
            metisResult = METIS_PartGraphKway(
                &numberOfVertices,
                &numberOfConstraints,
                vertexAdjacencyListStarts.data(),
                vertexAdjacencies.data(),
                NULL,
                NULL,
                NULL,
                &numberOfPartitions,
                NULL,
                NULL,
                metisOptions->metisOptions,
                &finalEdgeCut,
                partitions.data()
            );
        } else {
            std::fill(partitions.begin(), partitions.end(), 0.0f);
            metisResult = METIS_OK;
        }
        if (metisResult != METIS_OK) {
            if (metisResult == METIS_ERROR_INPUT) {
                throw std::runtime_error("METIS_ERROR_INPUT occurred.");
            } else if (metisResult == METIS_ERROR_MEMORY) {
                throw std::runtime_error("METIS_ERROR_MEMORY occurred.");
            } else if (metisResult == METIS_ERROR) {
                throw std::runtime_error("METIS_ERROR occurred.");
            } else {
                throw std::runtime_error("An undocumented METIS error occurred.");
            }
        }
        std::vector<gaspi_rank_t> res;
        moveFrom(partitions, res);
        return res;
    }

    std::string toString() {
        std::stringstream ss;
        ss << "[ ";
        for (auto i : vertexAdjacencyListStarts) {
            ss << i << " ";
        }
        ss << "]\n[";
        for (auto i : vertexAdjacencies) {
            ss << i << " ";
        }
        ss << "]";
        return ss.str();
    }
};

MetisActorDistributor::MetisActorDistributor(size_t xSize, size_t ySize)
    : ActorDistributor(xSize, ySize){
    MetisState state;
    MetisAdjacencyGraph graph(xSize, ySize, actorDistribution);
    gaspi_rank_t locRank = gpi_util::get_local_rank();
    try {
        if (!locRank) {
            actorDistribution = graph.partition(&state);   
        }
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        abort();
    }
    assert(actorDistribution.size() == xSize * ySize);
    mpi::broadcast(actorDistribution); //need this for now
    if (!locRank) l.cout() << "\n" << this->toString() << std::endl;
}
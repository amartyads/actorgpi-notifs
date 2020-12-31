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

#include <iostream>
#include <string>

#include "actorlib/utils/mpi_synchronization.hpp"
#include "actorlib/utils/gpi_utils.hpp"

#include <GASPI.h>

#include "orchestration/ActorOrchestrator.hpp"

#include "util/Configuration.hpp"
#include "util/Logger.hh"

#include "block/SWE_Block.hh"

using namespace std;

#ifndef ASSERT
#define ASSERT(ec) gpi_util::success_or_exit(__FILE__,__LINE__,ec)
#endif

void initLogger(gaspi_rank_t rank);

static tools::Logger &l = tools::Logger::logger;

int main(int argc, char **argv) {
    mpi::init();
    {
        mpi::barrier();
        ASSERT( gaspi_proc_init(GASPI_BLOCK));
        gaspi_rank_t rank = gpi_util::get_local_rank();
        initLogger(rank);
        auto config = Configuration::build(argc, argv, rank);
        if (!rank) cout << config.toString();
        ActorOrchestrator orch(config);
        orch.initActorGraph();
        ASSERT(gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK));
        orch.simulate();
        ASSERT( gaspi_proc_term(GASPI_BLOCK));
    }
    mpi::finalize();
}

void initLogger(gaspi_rank_t rank) {
    l.setProcessRank(rank);
    l.printWelcomeMessage();
}

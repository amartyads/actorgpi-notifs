/**
 * @file
 * This file is part of actorlib.
 *
 * @author Alexander Pöppl (poeppl AT in.tum.de,
 * https://www5.in.tum.de/wiki/index.php/Alexander_P%C3%B6ppl,_M.Sc.)
 *
 * @section LICENSE
 *
 * actorlib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * actorlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with actorlib.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * @section DESCRIPTION
 *
 * TODO
 */

#include "config.hpp"

#include <sstream>

#define STR(...) #__VA_ARGS__
#define S(...) STR(__VA_ARGS__)

namespace config {

const std::string gitRevision = S(ACTORLIB_GIT_REVISION);
const std::string gitCommitDate = S(ACTORLIB_GIT_DATE);
const std::string gitCommitMessage = S(ACTORLIB_GIT_COMMIT_MSG);

const uint64_t dataBlockSize = sizeof(int8_t) * 1024 * 10; // 10 kB
//const uint64_t dataBlockSize = 47;
const uint64_t numBlocksInBank = 1024 * 20; //with datablocksize, 20 MB
const uint64_t numBlocksInCache = 1;

std::string configToString() {
  std::stringstream ss;
  std::string parallelizationStr = "MPI-GPI Hybrid";
  ss << "GPI Actor Library Configuration" << std::endl;
  ss << "=================================" << std::endl;
  ss << "Actorlib mode:           " << parallelizationStr << std::endl;
  ss << "Data block size (bytes): " << dataBlockSize << std::endl;
  ss << "Blocks in bank:          " << numBlocksInBank << std::endl;
  ss << "Cache size in blocks:    " << numBlocksInCache << std::endl;
  ss << "Git revision:            " << gitRevision << std::endl;
  ss << "Git revision date:       " << gitCommitDate << std::endl;
  ss << "Git revision message:    " << gitCommitMessage << std::endl;
  return ss.str();
}

} // namespace config

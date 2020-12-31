#ifndef ACTORMPI_MPI_CONSTS_HPP
#define ACTORMPI_MPI_CONSTS_HPP

#include "mpi.h"
#include <string>

namespace mpi {

using rank = int;
using tag = int;

constexpr rank INVALID_RANK_ID = -1;
constexpr tag INVALID_TAG_ID = -1;
constexpr rank ROOT = 0;

constexpr int INFERRED_FROM_T = 0;

static rank me() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
}

static bool isRoot() { return me() == ROOT; }

static int world() {
  int worldSize;
  MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
  return worldSize;
}

static std::string name() {
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);
  return std::string(processor_name);
}

} // namespace mpi

#endif
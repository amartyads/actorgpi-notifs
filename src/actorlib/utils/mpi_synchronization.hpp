//
// Created by Bruno Macedo Miguel on 2019-05-26.
//

#ifndef ACTORMPI_MPI_SYNCHRONIZATION_HPP
#define ACTORMPI_MPI_SYNCHRONIZATION_HPP

#include "mpi_consts.hpp"
#include "mpi_type_traits.h"
#include <numeric>
#include <vector>

using namespace ::mpi;

namespace mpi {

static void barrier() { MPI_Barrier(MPI_COMM_WORLD); }
static void init() { MPI_Init(nullptr, nullptr); }
static void finalize() { MPI_Finalize(); }

template <class T> T allReduce(const T &localValue, MPI_Op op) {
  T globalValue;
  MPI_Allreduce(&localValue, &globalValue, 1,
                mpi_type_traits<T>::get_type(std::move(globalValue)), op,
                MPI_COMM_WORLD);
  return globalValue;
}

template <class T> void broadcast(T &data) {
  MPI_Bcast(
      mpi_type_traits<T>::get_addr(data), mpi_type_traits<T>::get_size(data),
      mpi_type_traits<T>::get_type(std::move(data)), mpi::ROOT, MPI_COMM_WORLD);
}

template <class T> class SynchronizeData {
public:
  explicit SynchronizeData(std::vector<T> &&data, MPI_Datatype &type)
      : localData(std::move(data)), type(type), totalObjects(0) {}
  explicit SynchronizeData(const std::vector<T> &data, MPI_Datatype &type)
      : localData(data), type(type), totalObjects(0) {}

  auto synchronize() {
    allGatherNumberOfObjects();

    gatherObjects();

    broadcastObjects();

    return std::move(globalObjects);
  }

private:
  void allGatherNumberOfObjects() {
    numObjectsPerRank.resize(world());
    int totalLocalData = localData.size();
    MPI_Allgather(&totalLocalData, 1, MPI_INT, numObjectsPerRank.data(), 1,
                  MPI_INT, MPI_COMM_WORLD);

    totalObjects =
        std::accumulate(numObjectsPerRank.begin(), numObjectsPerRank.end(), 0);
  }

  void gatherObjects() {
    std::vector<int> displacement{0};
    for (int i = 1; i < world(); i++) {
      auto disp = numObjectsPerRank[i - 1] + displacement.back();
      displacement.push_back(disp);
    }

    globalObjects.resize(totalObjects);
    MPI_Gatherv(localData.data(), localData.size(), type, globalObjects.data(),
                numObjectsPerRank.data(), displacement.data(), type, ROOT,
                MPI_COMM_WORLD);
  }

  void broadcastObjects() {
    MPI_Bcast(globalObjects.data(), totalObjects, type, ROOT, MPI_COMM_WORLD);
  }

  std::vector<T> localData;
  MPI_Datatype &type;

  std::vector<rank> numObjectsPerRank;
  int totalObjects;

  std::vector<T> globalObjects;
};
} // namespace mpi

#endif // ACTORMPI_MPI_SYNCHRONIZATION_HPP

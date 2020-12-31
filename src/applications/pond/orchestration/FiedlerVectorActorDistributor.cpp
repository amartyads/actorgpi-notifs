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

#include "orchestration/FiedlerVectorActorDistributor.hpp"
#include "util/Logger.hh"

#ifdef EIGEN3_PARTITIONING

// Ignore this warning in GCC
#pragma GCC diagnostic push
// Just ignore the warnings coming from the library!
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include <Eigen/Eigen>
#pragma GCC diagnostic pop

#include <cassert>
#include <cmath>
#include <sstream>
#include "actorlib/utils/gpi_utils.hpp"

using namespace Eigen;

static tools::Logger &l = tools::Logger::logger;

SparseMatrix<float> createAdjacencyMatrix(size_t xSize, size_t ySize) {
  std::vector<Triplet<float>> nonZeroes;
  for (int y = 0; y < ySize * xSize; y++) {
    for (int x = 0; x < ySize * xSize; x++) {
      bool isUpperNeighbor = (x == y + xSize);
      bool isLowerNeighbor = (x == y - xSize);
      bool isLeftNeighbor = (x == y + 1) && (x % xSize != 0);
      bool isRightNeighbor = (x == y - 1) && (x % xSize != xSize - 1);
      float num = 0.0f;
      if (isUpperNeighbor || isLowerNeighbor || isLeftNeighbor ||
          isRightNeighbor) {
        num = -1.0f;
      } else if (x == y) {
        float degree = 0.0f;
        if (x % xSize != 0)
          degree++;
        if (x % xSize != xSize - 1)
          degree++;
        if (y / xSize != 0)
          degree++;
        if (y / xSize != ySize - 1)
          degree++;
        num = degree;
      }
      if (num != 0.0f) {
        nonZeroes.push_back(Triplet<float>(y, x, num));
      }
    }
  }
  SparseMatrix<float> m(xSize * ySize, xSize * ySize);
  m.setFromTriplets(nonZeroes.begin(), nonZeroes.end());
  return m;
}

MatrixXf createEigenvectorMatrix(EigenSolver<MatrixXf> &es, size_t numPatches,
                                 size_t numRanks) {
  auto eigenvalues = es.eigenvalues();
  auto eigenvectors = es.pseudoEigenvectors();
  std::vector<std::pair<size_t, float>> eigenvalueIndices;

  for (size_t i = 0; i < eigenvalues.size(); i++) {
    eigenvalueIndices.push_back(std::make_pair(i, eigenvalues(i).real()));
  }
  std::sort(eigenvalueIndices.begin(), eigenvalueIndices.end(),
            [](std::pair<size_t, float> &a, std::pair<size_t, float> &b) {
              return a.second < b.second;
            });

  for (auto &p : eigenvalueIndices) {
    l.cout(false) << "[" << p.first << ", " << p.second << "] " << std::endl;
  }

  MatrixXf eigenvectorMatrix(numPatches, numRanks);
  assert(numRanks <= numPatches);
  for (size_t i = 0; i < numRanks; i++) {
    auto eigenvalueIdx = eigenvalueIndices[i].first;
    eigenvectorMatrix.col(i) << eigenvectors.col(eigenvalueIdx);
  }
  return eigenvectorMatrix;
}

void kMeans(MatrixXf &input, size_t numPatches, size_t numRanks) {
  l.cout(false) << "kMeans" << std::endl;
  std::vector<VectorXf> centroids;
  std::vector<size_t> partititions;
  for (size_t i = 0; i < numRanks; i++) {
    centroids.push_back(input.row(i));
  }

  MatrixXf distances(centroids.size(), input.rows());
  l.cout(false)
      << "added initial centroids and created distances matrix distances("
      << centroids.size() << ", " << input.rows() << ")" << std::endl;

  VectorXf partition(input.rows());

  for (int i = 0; i < input.rows(); i++) {
    for (size_t j = 0; j < centroids.size(); j++) {
      auto differenceVec = input.row(i).transpose() - centroids[j];
      auto distance = differenceVec.squaredNorm();
      distances(j, i) = distance;
    }
  }

  int changedPartitions = 0;
  for (int i = 0; i < input.rows(); i++) {
    auto currentMinimum = std::numeric_limits<float>::infinity();
    auto minPartitionIdx = 0;
    for (size_t j = 0; j < centroids.size(); j++) {
      if (distances(j, i) < currentMinimum) {
        currentMinimum = distances(j, i);
        minPartitionIdx = j;
        changedPartitions++;
      }
    }
    partition(i) = minPartitionIdx;
  }

  for (size_t i = 0; i < centroids.size(); i++) {
    float pointsInPartition = 0.0f;
    VectorXf mean = VectorXf::Zero(centroids[0].size());
    for (int pt = 0; pt < input.rows(); pt++) {
      if (partition(pt) == i) {
        mean += input.row(pt);
        pointsInPartition += 1.0f;
      }
    }
    centroids[i] = mean /= pointsInPartition;
    l.cout(false) << "Centroid " << i << ":\n" << centroids[i];
  }

  int iteration = 0;
  while (changedPartitions > 0) {
    changedPartitions = 0;

    for (int i = 0; i < input.rows(); i++) {
      for (size_t j = 0; j < centroids.size(); j++) {
        auto differenceVec = input.row(i).transpose() - centroids[j];
        auto distance = differenceVec.squaredNorm();
        distances(j, i) = distance;
      }
    }

    for (int i = 0; i < input.rows(); i++) {
      auto currentMinimum = std::numeric_limits<float>::infinity();
      auto minPartitionIdx = 0;
      for (size_t j = 0; j < centroids.size(); j++) {
        if (distances(j, i) < currentMinimum) {
          currentMinimum = distances(j, i);
          minPartitionIdx = j;
        }
      }

      if (partition(i) != minPartitionIdx) {
        changedPartitions++;
        partition(i) = minPartitionIdx;
      }
    }

    for (size_t i = 0; i < centroids.size(); i++) {
      float pointsInPartition = 0.0f;
      VectorXf mean = VectorXf::Zero(centroids[0].size());
      for (int pt = 0; pt < input.rows(); pt++) {
        if (partition(pt) == i) {
          mean += input.row(pt);
          pointsInPartition += 1.0f;
        }
      }
      centroids[i] = mean /= pointsInPartition;
    }

    l.cout(false) << "Iteration " << iteration++ << ": " << changedPartitions
                  << " changes." << std::endl;
  }

  l.cout(false) << distances << std::endl;
  l.cout(false) << partition << std::endl;

  std::vector<int> patchesPerPartition(gpi_util::get_local_rank());
  for (int i = 0; i < partition.size(); i++) {
    auto idx = static_cast<size_t>(partition(i));
    patchesPerPartition[idx]++;
  }

  std::stringstream ss;
  for (int i : patchesPerPartition) {
    ss << i << ", ";
  }
  l.cout(false) << ss.str() << std::endl;
}

FiedlerVectorActorDistributor::FiedlerVectorActorDistributor(size_t xSize,
                                                             size_t ySize)
    : ActorDistributor(xSize, ySize) {
  SparseMatrix<float> m = createAdjacencyMatrix(xSize, ySize);
  l.cout(false) << "Received xSize=" << xSize << ", ySize=" << ySize
                << std::endl;
  l.cout(false) << "Constructed Matrix with size" << m.size() << " ~> ("
                << m.rows() << ", " << m.cols() << ")" << std::endl;
  l.cout(false) << "\n" << m << std::endl;
  EigenSolver<MatrixXf> es(m);
  l.cout(false) << "The eigenvalues of m are\n"
                << es.eigenvalues() << std::endl;
  l.cout(false) << "The matrix of eigenvectors of m is\n"
                << es.eigenvectors() << std::endl;
  l.cout(false) << "Create matrix of " << gpi_util::get_local_rank() << " smallest eigenvectors"
                << std::endl;
  MatrixXf U = createEigenvectorMatrix(es, xSize * ySize, gpi_util::get_local_rank());
  l.cout(false) << "\n" << U << std::endl;
  kMeans(U, xSize * ySize, gpi_util::get_local_rank());
}

#else

/**
 * This is a set of stubs to avoid linker errors in case Eigen is unavailable.
 * This should not be used in production code.
 */

FiedlerVectorActorDistributor::FiedlerVectorActorDistributor(size_t xSize,
                                                             size_t ySize)
    : ActorDistributor(xSize, ySize) {}

#endif

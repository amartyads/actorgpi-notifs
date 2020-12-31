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
#include <cstring>
#include <sstream>
#include <cmath>
#include <iomanip>

class PatchBlockDistributor {
private:
    const size_t xSize;
    const size_t ySize;
    const size_t places;

    const std::unique_ptr<size_t[]> patchPlaces;
    const std::unique_ptr<size_t[]> placeCounts;

public:
    PatchBlockDistributor(size_t xSize, size_t ySize, size_t places)
        : xSize(xSize),
          ySize(ySize),
          places(places),
          patchPlaces(std::make_unique<size_t[]>(ySize * xSize)),
          placeCounts(std::make_unique<size_t[]>(places)) {
              std::memset(patchPlaces.get(), 0, xSize * ySize * sizeof(size_t));
              std::memset(placeCounts.get(), 0, places * sizeof(size_t));
    }

    void createDistribution() {
        createInitialDistribution();

    }

    size_t operator[](size_t patch) {
        return patchPlaces[patch];
    }

    std::string printArray() {
        std::stringstream ss;
        int digits = ceil(std::log10(places-1));

        for (size_t y = 0; y < ySize; y++) {
            for (size_t x = 0; x < xSize; x++) {
                ss << std::setw(digits) << patchPlaces[y * xSize + x] << " ";
            }
            ss << std::endl;
        }
        return ss.str();
    }

private:
    void createInitialDistribution() {
        size_t xSplits = static_cast<size_t>(std::floor(std::sqrt(std::sqrt(static_cast<double>(places)))));
        size_t ySplits = static_cast<size_t>(std::floor(std::sqrt(std::sqrt(static_cast<double>(places)))));
        size_t xPlaces = static_cast<size_t>(std::ceil(static_cast<double>(xSize) / static_cast<double>(xSplits)));

        std::cout << "X splits: " << xSplits << "Y splits: " << ySplits << std::endl;

        for (size_t y = 0; y < ySize; y++) {
            for (size_t x = 0; x < xSize; x++) {
                auto tmpX = x / xSplits;
                auto tmpY = y / ySplits;
                auto res = tmpY * xPlaces + tmpX;
                patchPlaces[y * xSize + x] = (res < places) ? res : (places - 1);
            }
        }
    }


};



int Blamain(int argc, const char * argv[]) {
    // insert code here...
    PatchBlockDistributor b(32, 32, 14);
    b.createDistribution();
    std:: cout << b.printArray() << std::endl;
    return 0;
}

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

#include "util/Configuration.hpp"

#include "util/args.hh"
#include "scenario/SWE_Scenario.hh"
#include "scenario/SWE_simple_scenarios.hh"
#ifdef WRITENETCDF
#include "scenario/NetCdfScenario.hpp"
#endif
#include "scenario/ScalablePoolDropScenario.hpp"

#include <sstream>
#include <string>

using namespace std;
using namespace std::string_literals;

Configuration::Configuration(size_t xSize, size_t ySize, size_t patchSize, size_t numberOfCheckpoints, std::string fileNameBase, Scenario *scenario)
    : xSize(xSize),
      ySize(ySize),
      patchSize(patchSize),
      numberOfCheckpoints(numberOfCheckpoints),
      fileNameBase(fileNameBase),
      scenario(scenario),
      dx(scenario->getSimulationArea().getDx(xSize)),
      dy(scenario->getSimulationArea().getDy(ySize)) {
    if (xSize % patchSize != 0) {
        throw std::runtime_error("Patch Size "s + to_string(patchSize) + " is no even divisor of x size "s + to_string(xSize));
    } else if (ySize % patchSize != 0) {
        throw std::runtime_error("Patch Size "s + to_string(patchSize) + " is no even divisor of y size "s + to_string(ySize));
    }
}


string Configuration::toString() {
    std::stringstream ss;
    ss << "#### POND Configuration ####" << std::endl;
    ss << "Total Grid Size:          " << xSize << "x" << ySize << " = " << (xSize * ySize) << " cells. (x*y)" << std::endl;
    ss << "Patch Size:               " << patchSize << "x" << patchSize << std::endl;
    ss << "Number of checkpoints:    " << numberOfCheckpoints << std::endl;
    ss << "File name Prefix:         " << fileNameBase << std::endl;
    ss << "Scenario simulation area: " << scenario->getSimulationArea() << std::endl;
    ss << "Cell Size:                " << dx << "m * " << dy << "m (dx * dy)" << std::endl;
    return ss.str();
}

Configuration Configuration::build(int argc, char **argv, size_t rank) {
    tools::Args args;
    args.addOption("grid-size-x", 'x', "Number of cell in x direction");
    args.addOption("grid-size-y", 'y', "Number of cell in y direction");
    args.addOption("patch-size", 'p', "Size of the patches (quadratic)");
    args.addOption("output-basepath", 'o', "Output base file name");
    args.addOption("output-steps-count", 'c', "Number of output time steps");
    args.addOption("scenario", 's', "Number of the scenario: 0 = default Radial dam break, 1 = NetCdfScenario, 2 = ScalablePoolDropScenario");
#ifdef WRITENETCDF
    args.addOption("bathymetry-file", 'b', "Path to a bathymetry file in GEBCO/NetCdf format", tools::Args::Required, false);
    args.addOption("displacement-x", 'u', "X Position of the initial displacement", tools::Args::Required, false);
    args.addOption("displacement-y", 'v', "Y Position of the initial displacement", tools::Args::Required, false);
    args.addOption("displacement-radius", 'r', "Radius of the initial displacement", tools::Args::Required, false);
#endif
    args.addOption("end-simulation", 'e', "Time after which simulation ends", tools::Args::Required, false);
    tools::Args::Result ret = args.parse(argc, argv, rank == 0);

    switch (ret) {
        case tools::Args::Error:
            throw std::runtime_error("Unable to parse configuration");
        case tools::Args::Help:
            return Configuration(0,0,0,0,"", nullptr);
        default:
            break;
    }

    auto xSize = args.getArgument<size_t>("grid-size-x");
    auto ySize = args.getArgument<size_t>("grid-size-y");
    auto patchSize = args.getArgument<size_t>("patch-size");
    auto fileNameBase = args.getArgument<std::string>("output-basepath");
    auto numberOfCheckpoints = args.getArgument<size_t>("output-steps-count");
    auto scenarioNumber = args.getArgument<int>("scenario");
    auto endTime = args.getArgument<float>("end-simulation");
    Scenario *scenario;
    if (scenarioNumber == 1) {
#ifdef WRITENETCDF
        auto bathymetryFile = args.getArgument<std::string>("bathymetry-file");
        auto displacementX = args.getArgument<double>("displacement-x");
        auto displacementY = args.getArgument<double>("displacement-y");
        auto displacementR = args.getArgument<double>("displacement-radius");
        scenario = new NetCdfScenario(bathymetryFile, displacementX, displacementY, displacementR, endTime);
#else
        throw std::runtime_error("NetCdfScenario is disabled, as NetCDF was not found on the system.");
#endif
    } else if (scenarioNumber == 0) {
        scenario = new RadialDamBreakScenario();
    } else if (scenarioNumber == 2) {
        auto gcd = [](size_t a, size_t b) {
            size_t h = 0;
            while (b != 0) {
                h = a % b;
                a = b;
                b = h;
            }
            return a;
        }(xSize, ySize);

        auto endTime = args.getArgument<float>("end-simulation");
        scenario = new ScalablePoolDropScenario(xSize / gcd, ySize / gcd, endTime);
    } else {
        scenario = nullptr;
        throw std::runtime_error("Invalid scenario number."); 
    }
    return Configuration(xSize, ySize, patchSize, numberOfCheckpoints, fileNameBase, scenario);
}   

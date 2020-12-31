import argparse
from math import *

homeDir = '/dss/dsshome1/lxc06/ga53qud2'

gpiraw = """#!/bin/bash
#SBATCH -J {jobName}
#SBATCH -o ./%x.%j.%N.out
#SBATCH -D ./
#SBATCH --get-user-env
#SBATCH --clusters={cluster}
#SBATCH --partition={partition}
#SBATCH --nodes={numNodes}
#SBATCH --ntasks-per-node={numTasksPerNode}
#SBATCH --mail-type=all
#SBATCH --mail-user=ga53qud@mytum.de
#SBATCH --export=NONE
#SBATCH --time=03:00:00
  
module load slurm_setup

module use -p /lrz/sys/spack/.tmp/test/gpi2/modules/x86_avx2/linux-sles12-x86_64
module load gpi-2/1.4.0-gcc8-impi-rdma-core-24
module load metis/5.1.0-intel19-i32-r32
module load netcdf-hdf5-all/4.6_hdf5-1.10-intel19-impi
module load gcc/8

cd {buildDir}

export OMP_NUM_THREADS=1

gaspi_logger &
mpiexec -n {totRanks} --perhost {numTasksPerNode} ./pond -x {xlen} -y {ylen} -p {patchSize} -e 1 -c 1 --scenario 2 -o output/out
"""

upcppraw="""#!/bin/bash
#SBATCH -J {jobName}
#SBATCH -o ./%x.%j.%N.out
#SBATCH -D ./
#SBATCH --get-user-env
#SBATCH --clusters={cluster}
#SBATCH --partition={partition}
#SBATCH --nodes={numNodes}
#SBATCH --tasks-per-node={numTasksPerNode}
#SBATCH --mail-type=all
#SBATCH --mail-user=ga53qud@mytum.de
#SBATCH --export=NONE
#SBATCH --time=03:00:00
  
module load slurm_setup

module load metis/5.1.0-intel19-i32-r32
module load netcdf-hdf5-all/4.6_hdf5-1.10-intel19-impi
module load gcc/8

export PATH="{upcxxPath}:$PATH"

cd {buildDir}

export OMP_NUM_THREADS={ompThreads}

upcxx-run -n {totRanks} ./pond -x {xlen} -y {ylen} -p {patchSize} -e 1 -c 1 --scenario 2 -o output/out
"""


def generateGPI(jobName, numNodes, xlen, ylen, patchSize):
    cluster = 'cm2' if numNodes > 2 else 'cm2_tiny'
    partition = 'cm2_tiny' if cluster == 'cm2_tiny' else 'cm2_large' if numNodes > 24 else 'cm2'
    numTasksPerNode = 28
    buildDir = homeDir + '/actor-gpi-v2/build'
    totRanks = numNodes * numTasksPerNode
    return gpiraw.format(
        jobName = jobName,
        cluster = cluster,
        partition = partition,
        numNodes = numNodes,
        numTasksPerNode = numTasksPerNode,
        buildDir = buildDir,
        totRanks = totRanks,
        xlen = xlen,
        ylen = ylen,
        patchSize = patchSize
    )

def generateUPCtask(jobName, numNodes, xlen, ylen, patchSize):
    cluster = 'cm2' if numNodes > 2 else 'cm2_tiny'
    partition = 'cm2_tiny' if cluster == 'cm2_tiny' else 'cm2_large' if numNodes > 24 else 'cm2'
    numTasksPerNode = 1
    ompThreads = 28
    buildDir = homeDir + '/actor-upcxx/build'
    totRanks = numNodes * numTasksPerNode
    upcxxPath = homeDir + '/upc_new/bin'
    return upcppraw.format(
        jobName = jobName,
        cluster = cluster,
        partition = partition,
        numNodes = numNodes,
        numTasksPerNode = numTasksPerNode,
        buildDir = buildDir,
        totRanks = totRanks,
        xlen = xlen,
        ylen = ylen,
        patchSize = patchSize,
        ompThreads = ompThreads,
        upcxxPath = upcxxPath
    )

def generateUPCrank(jobName, numNodes, xlen, ylen, patchSize):
    cluster = 'cm2' if numNodes > 2 else 'cm2_tiny'
    partition = 'cm2_tiny' if cluster == 'cm2_tiny' else 'cm2_large' if numNodes > 24 else 'cm2'
    numTasksPerNode = 28
    ompThreads = 1
    buildDir = homeDir + '/actor-upcxx/build'
    totRanks = numNodes * numTasksPerNode
    upcxxPath = homeDir + '/upc_new/bin'
    return upcppraw.format(
        jobName = jobName,
        cluster = cluster,
        partition = partition,
        numNodes = numNodes,
        numTasksPerNode = numTasksPerNode,
        buildDir = buildDir,
        totRanks = totRanks,
        xlen = xlen,
        ylen = ylen,
        patchSize = patchSize,
        ompThreads = ompThreads,
        upcxxPath = upcxxPath
    )

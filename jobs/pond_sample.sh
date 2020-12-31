#!/bin/bash
#SBATCH -J pond_proper
#SBATCH -o ./%x.%j.%N.out
#SBATCH -D ./
#SBATCH --get-user-env
#SBATCH --clusters=cm2
#SBATCH --partition=cm2_std
#SBATCH --nodes=8
#SBATCH --mail-type=end
#SBATCH --mail-user=ga53qud@mytum.de
#SBATCH --export=NONE
#SBATCH --time=05:00:00
  
module load slurm_setup

module use -p /lrz/sys/spack/.tmp/test/gpi2/modules/x86_avx2/linux-sles12-x86_64
module load gpi-2/1.4.0-gcc8-impi-rdma-core-24
module load cmake/3.16.5
module load metis/5.1.0-intel19-i32-r32
module load netcdf-hdf5-all/4.6_hdf5-1.10-intel19-impi
module load gcc/8

cd ../build/

mpiexec -n 8 --perhost 1 ./pond -x 16384 -y 16384 -p 2048 -e 1 -c 1 --scenario 2 -o output/out
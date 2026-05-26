#!/bin/bash

#SBATCH --job-name=lab5

#SBATCH --time=0:05:00

#SBATCH --ntasks-per-node=1

#SBATCH --partition batch

module load intel/mpi4

mpirun -r ssh ./lab5

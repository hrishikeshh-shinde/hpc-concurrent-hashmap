#!/usr/bin/env zsh
#SBATCH --partition=instruction
#SBATCH --time 00:30:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --output=hashmap_tests.out

# Log in the submission directory
cd $SLURM_SUBMIT_DIR

# Load the gcc module
module load gcc

# Create build directory
mkdir -p build

# Compile all tests
make all

# Run correctness test
echo "==== Running Correctness Test ===="
./build/compilation
echo

# Run read-heavy test with 8 threads
echo "==== Running Read-Heavy Test ===="
./build/read_heavy 8 1000 100
echo


echo "All tests completed!"
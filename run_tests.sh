#!/usr/bin/env zsh
#SBATCH --partition=instruction
#SBATCH --time 00:30:00 
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8 
#SBATCH --mem-per-cpu=2G 
#SBATCH --job-name=hashmap_tests
#SBATCH --output=hashmap_tests_%j.out

# Log in the submission directory
cd $SLURM_SUBMIT_DIR 

module load gcc

# Create build directory
echo "Creating build directory..."
mkdir -p build
echo

echo "Compiling all targets..."
make clean 
make all

if [ $? -ne 0 ]; then
    echo "MAKE FAILED! Exiting."
    exit 1
fi
echo "Compilation successful."
echo

echo "==== Running Correctness Test (build/compilation) ===="
./build/compilation
echo "======================================================"
echo


echo "==== Running Read-Heavy Test (build/read_heavy) ===="
./build/read_heavy 8 1000 100
echo "======================================================"
echo

echo "==== Running Resize Stress Test (build/resize_stress) ===="
./build/resize_stress
echo "======================================================"
echo

echo "==== Running Benchmark Tests (build/benchmarks) ===="
./build/benchmarks
echo "======================================================"
echo


echo "All tests completed at $(date)!"
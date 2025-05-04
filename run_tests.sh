#!/usr/bin/env bash
# Using bash for broader compatibility

#SBATCH --partition=instruction
#SBATCH --time=00:15:00 # Adjusted time
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8 # Keep cores for timing test
#SBATCH --mem-per-cpu=2G
#SBATCH --job-name=hashmap_part_tests
#SBATCH --output=hashmap_part_tests_%j.out

# Log in the submission directory
echo "Changing to submission directory: $SLURM_SUBMIT_DIR"

# Load necessary modules (adjust if needed)
echo "Loading GCC module..."
module load gcc

# Create build directory
echo "Creating build directory..."
mkdir -p build
echo

# Compile the specific target (or all if preferred)
echo "Compiling targets..."
cd build || exit 1 # Enter build directory
cmake .. # Configure
make all # Build everything defined in CMakeLists.txt
cd .. # Go back to project root

if [ $? -ne 0 ]; then
    echo "MAKE FAILED! Exiting."
    exit 1
fi
echo "Compilation successful."
echo

# --- Run Simple Correctness Test ---
SIMPLE_TEST_EXEC="./build/partition_simple_test"
if [ -f "$SIMPLE_TEST_EXEC" ]; then
    echo "==== Running Partition HashMap Simple Test ($SIMPLE_TEST_EXEC) ===="
    "$SIMPLE_TEST_EXEC"
    echo "==================================================================="
    echo
else
    echo "ERROR: Test executable $SIMPLE_TEST_EXEC not found!"
    # Decide whether to exit or continue
    # exit 1
fi

# --- Run Timing Test ---
TIMING_TEST_EXEC="./build/partition_timing_test"
# Use the number of CPUs allocated by Slurm for the timing test
NUM_THREADS=${SLURM_CPUS_PER_TASK:-4} # Default to 4 if var not set

if [ -f "$TIMING_TEST_EXEC" ]; then
    echo "==== Running Partition HashMap Timing Test ($TIMING_TEST_EXEC) with $NUM_THREADS threads ===="
    # Pass the number of threads as a command-line argument
    "$TIMING_TEST_EXEC" "$NUM_THREADS"
    echo "======================================================================================"
    echo
else
    echo "ERROR: Test executable $TIMING_TEST_EXEC not found!"
    # Decide whether to exit or continue
    # exit 1
fi


# --- Commented out other test runs ---
# ...

echo "All specified tests completed at $(date)!"
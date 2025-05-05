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
cd "$SLURM_SUBMIT_DIR" || exit 1 # Exit if cd fails

# Load necessary modules (adjust if needed for GLIBC compatibility)
echo "Loading GCC module..."
# --- ADJUST THIS LINE AS NEEDED ---
# Example: Try GCC 11.x (check 'module avail gcc' on your cluster)
module load gcc/11.3.0
# Or comment out the line above to use the system default compiler
# --- END ADJUSTMENT ---

# Check if g++ is available
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ command not found after loading module. Check module name/availability."
    exit 1
fi
echo "Using g++: $(command -v g++)"
echo "g++ version: $(g++ --version | head -n1)"


# Compile using Makefile
echo "Compiling targets using Makefile..."
# Clean previous build (optional but recommended)
make clean
# Build the targets defined in the Makefile ('all' target)
make all

# Check if make was successful
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

echo "All specified tests completed at $(date)!"
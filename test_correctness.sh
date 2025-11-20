#!/bin/bash

# Test script for Game of Life implementations
# Tests correctness across different thread counts

echo "================================="
echo "Game of Life Correctness Testing"
echo "================================="
echo ""

SEED=12345
SIZE=100
ITERS=50
PROB=0.3

# Function to run test and capture output
run_test() {
    local impl=$1
    local threads=$2
    local game=$3
    
    echo "Testing $impl with $threads thread(s), game=$game..."
    
    # Run with display disabled and capture final population
    output=$(./$impl -n $SIZE -i $ITERS -t $threads -s $SEED -p $PROB -g $game -d 2>&1 | grep "Final population")
    
    if [ $? -ne 0 ]; then
        echo "  ERROR: Test failed to run"
        return 1
    fi
    
    echo "  $output"
    echo "$output" | awk '{print $3}'
}

# Test OpenMP implementation
test_openmp() {
    echo ""
    echo "--- Testing OpenMP Implementation ---"
    cd OpenMP || exit 1
    
    if [ ! -f life ]; then
        echo "ERROR: life executable not found. Run 'make' first."
        cd ..
        return 1
    fi
    
    echo "Test 1: Random game with different thread counts"
    pop1=$(run_test "life" 1 0)
    pop2=$(run_test "life" 2 0)
    pop4=$(run_test "life" 4 0)
    pop8=$(run_test "life" 8 0)
    
    if [ "$pop1" == "$pop2" ] && [ "$pop2" == "$pop4" ] && [ "$pop4" == "$pop8" ]; then
        echo "  ✓ PASS: All thread counts produce same result (population=$pop1)"
    else
        echo "  ✗ FAIL: Results differ - 1T:$pop1, 2T:$pop2, 4T:$pop4, 8T:$pop8"
    fi
    
    echo ""
    echo "Test 2: Block still life (should remain 4)"
    pop_block=$(run_test "life" 4 1)
    if [ "$pop_block" == "4" ]; then
        echo "  ✓ PASS: Block pattern correct (population=4)"
    else
        echo "  ✗ FAIL: Block pattern incorrect (population=$pop_block, expected 4)"
    fi
    
    echo ""
    echo "Test 3: Glider (should remain 5)"
    pop_glider=$(run_test "life" 4 2)
    if [ "$pop_glider" == "5" ]; then
        echo "  ✓ PASS: Glider pattern correct (population=5)"
    else
        echo "  ✗ FAIL: Glider pattern incorrect (population=$pop_glider, expected 5)"
    fi
    
    cd ..
}

# Test Pthreads implementation
test_pthreads() {
    echo ""
    echo "--- Testing Pthreads Implementation ---"
    cd Pthreads || exit 1
    
    if [ ! -f life ]; then
        echo "ERROR: life executable not found. Run 'make' first."
        cd ..
        return 1
    fi
    
    echo "Test 1: Random game with different thread counts"
    pop1=$(run_test "life" 1 0)
    pop2=$(run_test "life" 2 0)
    pop4=$(run_test "life" 4 0)
    pop8=$(run_test "life" 8 0)
    
    if [ "$pop1" == "$pop2" ] && [ "$pop2" == "$pop4" ] && [ "$pop4" == "$pop8" ]; then
        echo "  ✓ PASS: All thread counts produce same result (population=$pop1)"
    else
        echo "  ✗ FAIL: Results differ - 1T:$pop1, 2T:$pop2, 4T:$pop4, 8T:$pop8"
    fi
    
    echo ""
    echo "Test 2: Block still life (should remain 4)"
    pop_block=$(run_test "life" 4 1)
    if [ "$pop_block" == "4" ]; then
        echo "  ✓ PASS: Block pattern correct (population=4)"
    else
        echo "  ✗ FAIL: Block pattern incorrect (population=$pop_block, expected 4)"
    fi
    
    echo ""
    echo "Test 3: Glider (should remain 5)"
    pop_glider=$(run_test "life" 4 2)
    if [ "$pop_glider" == "5" ]; then
        echo "  ✓ PASS: Glider pattern correct (population=5)"
    else
        echo "  ✗ FAIL: Glider pattern incorrect (population=$pop_glider, expected 5)"
    fi
    
    cd ..
}

# Compare OpenMP vs Pthreads
compare_implementations() {
    echo ""
    echo "--- Comparing OpenMP vs Pthreads ---"
    
    cd OpenMP
    omp_result=$(./life -n $SIZE -i $ITERS -t 4 -s $SEED -p $PROB -d 2>&1 | grep "Final population" | awk '{print $3}')
    cd ..
    
    cd Pthreads
    pthread_result=$(./life -n $SIZE -i $ITERS -t 4 -s $SEED -p $PROB -d 2>&1 | grep "Final population" | awk '{print $3}')
    cd ..
    
    if [ "$omp_result" == "$pthread_result" ]; then
        echo "  ✓ PASS: OpenMP and Pthreads produce identical results (population=$omp_result)"
    else
        echo "  ✗ FAIL: Results differ - OpenMP:$omp_result, Pthreads:$pthread_result"
    fi
}

# Run all tests
test_openmp
test_pthreads
compare_implementations

echo ""
echo "================================="
echo "Testing Complete"
echo "================================="
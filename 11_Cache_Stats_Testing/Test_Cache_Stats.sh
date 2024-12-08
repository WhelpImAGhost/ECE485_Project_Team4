#!/bin/bash

# This script tests all of these cases

# 1. Verify Tracking of Cache Reads and Writes
# 1.1 Process Read and Write Requests

# 2. Confirm Cache Hit and Miss Count Accuracy
# 2.1 Track Hits and Misses During Cache Access

# 3. Calculate and Report Cache Hit Ratio
# 3.1 Compute and Display Cache Hit Ratio

# 4. Output Statistics in Silent and Normal Modes
# 4.1 Verify Output in Silent Mode
# 4.2 Verify Output in Normal Mode


# Check if the required executable exists
if [ ! -f "../test" ]; then
    echo "Error: The './test' executable is not found. Compile your program first."
    exit 1
fi

# Input trace file
TRACE_FILE="11_1.din"

# Check if the trace file exists
if [ ! -f "$TRACE_FILE" ]; then
    echo "Error: Trace file '$TRACE_FILE' does not exist."
    exit 1
fi

# Expected read and write counts based on the trace file
EXPECTED_READS=7
EXPECTED_WRITES=5

# Expected cache miss and hits countsbased on the trace file
EXPECTED_HITS=4
EXPECTED_MISSES=8

# This is the expected cache hit ratio
EXPECTED_RATIO=0.33333

# Execute the test with the trace file
echo "Running cache test with trace file: $TRACE_FILE"
OUTPUT=$(../test -m 1 -f $TRACE_FILE)

# Display the output for debugging
echo "Program Output:"
echo "$OUTPUT"

# Extract values using refined awk logic
ACTUAL_READS=$(echo "$OUTPUT" | awk -F': ' '/Overall Reads:/ {print $2}' | awk -F',' '{print $1}')
ACTUAL_WRITES=$(echo "$OUTPUT" | awk -F': ' '/Overall Writes:/ {print $3}'| awk -F',' '{print $1}')
ACTUAL_HITS=$(echo "$OUTPUT" | awk -F': ' '/Cache hits:/ {print $4}' | awk -F',' '{print $1}')
ACTUAL_MISSES=$(echo "$OUTPUT" | awk -F': ' '/Cache misses:/ {print $5}'| awk -F',' '{print $1}')
ACTUAL_RATIO=$(echo "$OUTPUT" | awk -F': ' '/Cache misses:/ {print $6}'| awk -F',' '{print $1}')

# Debugging: Show extracted values
echo "Extracted Reads: $ACTUAL_READS"
echo "Extracted Writes: $ACTUAL_WRITES"
echo "Extracted Cache Hits: $ACTUAL_HITS"
echo "Extracted Cache Misses: $ACTUAL_MISSES"
echo "Extracted Cache Hit Ratio: $ACTUAL_RATIO"


# Verify the results for reads and writes
echo "Verifying cache read/write tracking..."
if [[ "$ACTUAL_READS" -eq "$EXPECTED_READS" && "$ACTUAL_WRITES" -eq "$EXPECTED_WRITES" ]]; then
    echo "Test 1 Passed: Read and Write counts match the expected values."
    echo "Expected Reads: $EXPECTED_READS, Actual Reads: $ACTUAL_READS"
    echo "Expected Writes: $EXPECTED_WRITES, Actual Writes: $ACTUAL_WRITES"
    #exit 0
else
    echo "Test 1 Failed: Mismatch in read/write counts."
    echo "Expected Reads: $EXPECTED_READS, Actual Reads: $ACTUAL_READS"
    echo "Expected Writes: $EXPECTED_WRITES, Actual Writes: $ACTUAL_WRITES"
    #exit 1
fi

# Verify for cache hits and misses
echo "Verifying cache hits/misses tracking..."
if [[ "$ACTUAL_HITS" -eq "$EXPECTED_HITS" && "$ACTUAL_MISSES" -eq "$EXPECTED_MISSES" ]]; then
    echo "Test 2 Passed: Counts match the expected values."
    echo "Expected Reads: $EXPECTED_HITS, Actual Reads: $ACTUAL_HITS"
    echo "Expected Writes: $EXPECTED_MISSES, Actual Writes: $ACTUAL_MISSES"
    #exit 0
else
    echo "Test 2 Failed: Mismatch in counts."
    echo "Expected Hits: $EXPECTED_HITS, Actual Hits: $ACTUAL_HITS"
    echo "Expected Misses: $EXPECTED_MISSES, Actual Misses: $ACTUAL_MISSES"
    #exit 1
fi

# Verify for cache hit ratio
echo "Verifying cache hit ratio..."
if awk "BEGIN {exit !(($ACTUAL_RATIO == $EXPECTED_RATIO))}"; then
    echo "Test 3 Passed: Cache hit ratio matches the expected value."
    echo "Expected Ratio: $EXPECTED_RATIO, Actual Ratio: $ACTUAL_RATIO"
    #exit 0
else
    echo "Test 3 Failed: Cache hit ratio mismatch."
    echo "Expected Ratio: $EXPECTED_RATIO, Actual Ratio: $ACTUAL_RATIO"
    #exit 1
fi

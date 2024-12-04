#!/bin/bash

#1. Verify Tracking of Cache Reads and Writes
#1.1 Process Read and Write Requests

# Check if the required executable exists
if [ ! -f "../test" ]; then
    echo "Error: The './test' executable is not found. Compile your program first."
    exit 1
fi

# Input trace file
TRACE_FILE="test_cache_stats.din"

# Check if the trace file exists
if [ ! -f "$TRACE_FILE" ]; then
    echo "Error: Trace file '$TRACE_FILE' does not exist."
    exit 1
fi

# Expected read and write counts based on the trace file
EXPECTED_READS=7
EXPECTED_WRITES=5

# Execute the test with the trace file
echo "Running cache test with trace file: $TRACE_FILE"
OUTPUT=$(../test -m 1 -f $TRACE_FILE)

# Display the output for debugging
echo "Program Output:"
echo "$OUTPUT"

# Extract values using refined awk logic
ACTUAL_READS=$(echo "$OUTPUT" | awk -F': ' '/Overall Reads:/ {print $2}' | awk -F',' '{print $1}')
ACTUAL_WRITES=$(echo "$OUTPUT" | awk -F': ' '/Overall Writes:/ {print $3}'| awk -F',' '{print $1}')

# Debugging: Show extracted values
echo "Extracted Reads: $ACTUAL_READS"
echo "Extracted Writes: $ACTUAL_WRITES"

# Verify the results
echo "Verifying cache read/write tracking..."
if [[ "$ACTUAL_READS" -eq "$EXPECTED_READS" && "$ACTUAL_WRITES" -eq "$EXPECTED_WRITES" ]]; then
    echo "Test Passed: Read and Write counts match the expected values."
    echo "Expected Reads: $EXPECTED_READS, Actual Reads: $ACTUAL_READS"
    echo "Expected Writes: $EXPECTED_WRITES, Actual Writes: $ACTUAL_WRITES"
    exit 0
else
    echo "Test Failed: Mismatch in read/write counts."
    echo "Expected Reads: $EXPECTED_READS, Actual Reads: $ACTUAL_READS"
    echo "Expected Writes: $EXPECTED_WRITES, Actual Writes: $ACTUAL_WRITES"
    exit 1
fi

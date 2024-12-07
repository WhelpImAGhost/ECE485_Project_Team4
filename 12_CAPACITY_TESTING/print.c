#include <stdio.h>

void fill_cache_and_print(int num_sets, int num_ways) {
    for (int set = 0; set < num_sets; set++) {
        for (int way = 0; way < num_ways; way++) {
            // Construct the 32-bit address
            unsigned int address = 0;  // Start with all bits set to 0
            address |= (set << 6);     // Bits 6-19 represent the set index
            address |= (way << 20);    // Bits 20-23 represent the way index
            
            // Print the formatted output
            printf("0 %08X\n", address);
        }
    }
}

int main() {
    int num_sets = 16384;  // Example: 64 sets (2^6 sets -> 6 bits for the set index)
    int num_ways = 16;   // Example: 4 ways (2^2 ways -> 2 bits for the way index)

    fill_cache_and_print(num_sets, num_ways);
    return 0;
}
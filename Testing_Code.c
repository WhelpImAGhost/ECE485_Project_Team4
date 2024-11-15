/*
#include <stdio.h>
#include <stdint.h>

#define INDEX_ARRAY1 16384 // 16K size
#define SET_ARRAY2 17      // Changed to 17
#define PLRU_ARRAY3 15

// Array to store cache data
int array1[INDEX_ARRAY1][SET_ARRAY2][PLRU_ARRAY3]; // 3D array

// Tag array to store tags associated with each SET_ARRAY2 entry
uint32_t tags[INDEX_ARRAY1][SET_ARRAY2]; // Array to hold tags for each entry in SET_ARRAY2

void accessArray(uint32_t hexValue) {
    // Step 1: Print the input hex value
    printf("Hex Value: 0x%X\n", hexValue);

    // Step 2: Extract the index and tag from the hex value
    uint32_t index = (hexValue >> 6) & 0x3FFF; // Extract A19:A6 for the index (14 bits)
    uint32_t tag = (hexValue >> 20) & 0xFFF;   // Extract A31:A20 for the tag (12 bits)
    printf("Extracted Index (A19:A6): %u\n", index);
    printf("Extracted Tag (A31:A20): 0x%X\n", tag);

    // Step 3: Check if this tag already exists in SET_ARRAY2 for the given index
    int found = 0;
    for (int i = 0; i < SET_ARRAY2; i++) {
        if (tags[index][i] == tag) { // Check if the tag matches
            found = 1;
            printf("Tag 0x%X found in SET_ARRAY2[%d] for array1[%u]. Values:\n", tag, i, index);
            for (int j = 0; j < PLRU_ARRAY3; j++) {
                printf("%d ", array1[index][i][j]);
            }
            printf("\n");
            break;
        }
    }

    if (!found) {
        printf("Tag 0x%X not found in SET_ARRAY2 for array1[%u]. Adding new entry.\n", tag, index);
        // If tag is not found, add it to SET_ARRAY2 (e.g., replace an entry - here we replace entry 0 for simplicity)
        tags[index][0] = tag;
        for (int j = 0; j < PLRU_ARRAY3; j++) {
            array1[index][0][j] = tag + j; // Example values; set new data
        }
    }
}

int main() {
    uint32_t hexValue = 0x12345678; // Example hex value
    accessArray(hexValue);
    accessArray(hexValue);
    return 0;
}
*/
//3 arrays and some of them nested.

#include <stdio.h>
#include <stdint.h>

#define INDEX_ARRAY_SIZE 16384  // 16Ki sets
#define SET_ARRAY_SIZE 2
#define PLRU_ARRAY_SIZE 15
#define TAG_ARRAY 16
#define TAG_ARRAY_MESI 2

// Enumeration for MESI states for clarity
enum MESIState { INVALID = 0, SHARED, EXCLUSIVE, MODIFIED };

// Define the structure of each "Set" in the cache
struct Set {
    int plru[PLRU_ARRAY_SIZE];            // PLRU array
    int tags[TAG_ARRAY][TAG_ARRAY_MESI]; // Tag array with [MESI, Tag]
};

// The main array, representing the cache's sets
struct Set cache[INDEX_ARRAY_SIZE][SET_ARRAY_SIZE];

void accessCache(uint32_t hexValue) {
    // Step 1: Extract index and tag from the hex value
    uint32_t index = (hexValue >> 6) & 0x3FFF; // A19:A6 for the index (14 bits)
    uint32_t tag = (hexValue >> 20) & 0xFFF;   // A31:A20 for the tag (12 bits)
    printf("Hex Value: 0x%X\n", hexValue);
    printf("Extracted Index (A19:A6): %u\n", index);
    printf("Extracted Tag (A31:A20): 0x%X\n", tag);

    // Step 2: Access the Set array for the specified index
    struct Set *set = &cache[index][0]; // Access the first Set array for simplicity

    // Step 3: Check if the tag exists in the Tag array within this Set
    int found = 0;
    for (int i = 0; i < TAG_ARRAY; i++) {
        if (set->tags[i][1] == tag && set->tags[i][0] != INVALID) { // Check for a matching, valid tag
            found = 1;
            printf("Tag 0x%X found in Set[%u]. Tag Array row %d:\n", tag, index, i);
            printf("  MESI State: %d, Tag: 0x%X\n", set->tags[i][0], set->tags[i][1]);

            // Display PLRU array values
            printf("  PLRU array: ");
            for (int j = 0; j < PLRU_ARRAY_SIZE; j++) {
                printf("%d ", set->plru[j]);
            }
            printf("\n");
            break;
        }
    }

    if (!found) {
        printf("Tag 0x%X not found in Set[%u]. Adding new entry.\n", tag, index);
        // If tag is not found, replace the first entry (simplified replacement)
        set->tags[0][0] = MODIFIED; // Set MESI to MODIFIED for demonstration
        set->tags[0][1] = tag;      // Store the tag

        // Initialize PLRU values (for demonstration purposes)
        for (int j = 0; j < PLRU_ARRAY_SIZE; j++) {
            set->plru[j] = j; // Set each PLRU element to its index
        }
    }
}

int main() {
    uint32_t hexValue = 0x12345678; // Example hex value
    accessCache(hexValue);
    accessCache(hexValue);
    return 0;
}



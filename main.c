/*
 * ECE 485/585 Final Prroject
 * Team 4
 * 
 * Ameer Melli
 * Caleb Monti
 * Chris Kane-Pardy
 * 
* This is the test branch
* gcc main.c -o test -DDEBUG (to run debug mode with prints)
* gcc main.c -o test (run without prints)
* ./test (run code)
* ./test rwims.din (to run with a file name)
*/


#include <stdio.h>
#include <stdlib.h>
#include "defines.c"
#include <stdint.h>

// Global variables
uint64_t counter = 0;
int mode = 1;       // 1 for normal, 0 for silent

// Global Type Definitions
typedef struct {
    int mesi; // MESI state
    int tag;  // Tag
} Way;

typedef struct {
    int *plru;       // Pointer for PLRU bits (dynamic array)
    Way **ways;      // Pointer to array of Way pointers (dynamic array)
} Set;

// Function Prototypes

// Function to extract tag, index, and byte select from an address
void extract_address_components(unsigned int address, int *tag, int *set_index, int *byte_select, int tag_bits, int index_bits, int byte_select_bits);
void UpdatePLRU(int PLRU[], int w );
uint8_t VictimPLRU(int PLRU[]);
int GetSnoopResult(unsigned int address);
int hit_or_miss(Set *index[], int set_index, int tag, int *InvalidWays);

int main(int argc, char *argv[]) {

    
    // Add flags for setting non-default variables


    /* Memory Size Calculations */

    const int TRUE_CAPACITY = pow(2,CACHE_SIZE);
    const int LINES = (TRUE_CAPACITY / CACHE_LINE_SIZE);
    const int SETS = (LINES / ASSOCIATIVITY);


    /* Tag Array Calculations */


    const int BYTE_SELECT_BITS = log2(CACHE_LINE_SIZE);
    const int INDEX_BITS = log2(SETS);
    const int TAG_BITS = ADDRESS_SIZE - (BYTE_SELECT_BITS + INDEX_BITS);
    const int PLRU_ARRAY_SIZE = (ASSOCIATIVITY - 1);
    const int TOTAL_TAG_ARRAY = SETS * ((ASSOCIATIVITY * (TAG_BITS + TAG_ARRAY_MESI)) + PLRU_ARRAY_SIZE); 	
    //Can be used for testing to verify that values can be changed correctly

    // Include tests for if the tag is too big or index is too big
    // (go to default if the input values are out of bounds). 

    int plru_array[PLRU_ARRAY_SIZE];

    Set *index[SETS]; // Array of Set pointers

    for (int i = 0; i < SETS; i++) {
    index[i] = (Set *)malloc(sizeof(Set));
    if (index[i] == NULL) {
        perror("Failed to allocate memory for Set");
        exit(EXIT_FAILURE);
    }

    index[i]->plru = (int *)malloc(PLRU_ARRAY_SIZE * sizeof(int));
    if (index[i]->plru == NULL) {
        perror("Failed to allocate memory for PLRU array");
        exit(EXIT_FAILURE);
    }

    for (int j = 0; j < PLRU_ARRAY_SIZE; j++) {
        index[i]->plru[j] = 0;
    }

    index[i]->ways = (Way **)malloc(ASSOCIATIVITY * sizeof(Way *));
    if (index[i]->ways == NULL) {
        perror("Failed to allocate memory for Way array");
        exit(EXIT_FAILURE);
    }

    for (int k = 0; k < ASSOCIATIVITY; k++) {
        index[i]->ways[k] = (Way *)malloc(sizeof(Way));
        if (index[i]->ways[k] == NULL) {
            perror("Failed to allocate memory for Way");
            exit(EXIT_FAILURE);
        }
        index[i]->ways[k]->mesi = INVALID;
        index[i]->ways[k]->tag = 0;
    }
}

    #ifdef DEBUG
    fprintf(stderr, "Number of sets: %d\n", SETS);
    fprintf(stderr, "Total number of initialized Ways: %llu\n", counter);
    #endif



    // Set default filename
    const char *default_filename = "Default.din";
    const char *filename;

    // Check if user provided a filename as an argument
    if (argc > 1) {
        filename = argv[1];
    } else {
        filename = default_filename;
    }

    FILE *file = fopen(filename, "r");

    // If the provided file can't be opened, try the default file
    if (file == NULL && argc > 1) {
        fprintf(stderr, "The '%s' file provided could not be opened. Attempting to open default file '%s'.\n", filename, default_filename);
        filename = default_filename;
        file = fopen(filename, "r");
    }

    // Check if the file (either provided or default) was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fprintf(stderr, "Using file: %s\n", filename);

    int operation;
    unsigned int address;
    int tag, set_index, byte_select;

    // Read each line until end of file
    while (fscanf(file, "%d %x", &operation, &address) == 2) {
        extract_address_components(address, &tag, &set_index, &byte_select, TAG_BITS, INDEX_BITS, BYTE_SELECT_BITS);
        #ifdef DEBUG
        fprintf(stderr, "Operation: %d, Address: 0x%X\n", operation, address);
        fprintf(stderr, "Extracted Tag: 0x%X\n", tag);
        fprintf(stderr, "Extracted Index: 0x%X\n", set_index);
        fprintf(stderr, "Extracted Byte Select: 0x%X\n", byte_select);
        #endif
        // Process the values here if needed
    }

    fclose(file);  // Close the file

return 0;
}

// Function declarations

// Function to extract tag, index, and byte select from an address
void extract_address_components(unsigned int address, int *tag, int *set_index, int *byte_select, int tag_bits, int index_bits, int byte_select_bits) {
    // Mask for the least significant 'Byte Select' bits
    unsigned int byte_select_mask = (1 << byte_select_bits) - 1;

    // Mask for the next 'Index' Bits
    unsigned int index_mask = ((1 << index_bits) - 1) << byte_select_bits;

    // Extract byte select (least significant bits)
    *byte_select = address & byte_select_mask;

    // Extract index ("middle" bits)
    *set_index = (address & index_mask) >> byte_select_bits;

    // Extract tag (remaining bits above index)
    *tag = address >> (byte_select_bits + index_bits);
}

// Function to check new address for hit or miss
int hit_or_miss(Set *index[], int set_index, int tag, int *InvalidWays){
    InvalidWays = NULL;
    for (int i = 0; i < ASSOCIATIVITY; i++){
        Way *way = index[set_index]->ways[i];
        if (way->mesi != INVALID && way->tag == tag) {
            return 1; //Hit
        }
        else if (way->mesi == INVALID) {
            *InvalidWays = i;
        }
    }
    return 0; //Miss
}


// Function call to update PLRU bits after the most recent access
// Takes in specific PLRU for that set as an argument
void UpdatePLRU(int PLRU[], int w ){
    int bit = 0;
    if (w > (ASSOCIATIVITY-1) || w < 0){
        fprintf(stderr,"Value of w not allowed for PLRU updating\n");
        exit(-1);
    }


    for (int i = 0; i < log2(ASSOCIATIVITY); i++){
        if ( (0x1 & (w >> ( (int)(log2((int)ASSOCIATIVITY) -1)-i ) )) == 0 )  {
            PLRU[bit] = 0;
            bit = (2*bit) + 1;
        }

        else {
            PLRU[bit] = 1;
            bit = (2*bit) + 2;
        }
    }
    return;
}

// Function call to determine which way to evict for a PLRU
// Takes in specific PLRU for that set as an argument
uint8_t VictimPLRU(int PLRU[]){

    int b = 0;

    for (int i = 0; i < log2(ASSOCIATIVITY); i++){
        #ifdef DEBUG
            fprintf(stderr,"Triggering PLRU check #%d with val %d, ", i, PLRU[b]);
            fprintf(stderr, "B = %d, \n", b);
        #endif
        if(PLRU[b]) b = 2 * b + 1;
        else b = 2 * b + 2;
    }

    b = b - (ASSOCIATIVITY - 1);
    return b;

}

int GetSnoopResult(unsigned int address) {
    return address & 0x3;

}

void MESI_set(int mesi, unsigned int address, int operation){

    int snoop = GetSnoopResult(address);


    switch (operation){

        
        case READ_HD: // n = 0
        case READ_HI: // n = 2
            if (snoop == HIT || snoop == HITM){
                mesi = SHARED;
            }
            else mesi = EXCLUSIVE;

            break;
        case WRITE_HD: // n = 1
            if (mesi == INVALID) if (mode) fprintf(stderr, "ANNOUNCING READ FOR OWNERSHIP AT ADDRESS 0x%8X", address);
            if (mesi == SHARED) if (mode) fprintf(stderr, "ANNOUNCING BUS UPGRADE AT ADDRESS 0x%8X", address);
            
            mesi = MODIFIED;
            break;
        case READ_S: // n = 3

            if (mesi == EXCLUSIVE) mesi = SHARED;
            if (mesi == MODIFIED) {
                if (mode) fprintf(stderr, "FLUSHING CONTENTS TO DRAM");
                mesi = SHARED;
            }
        
        case RWIM_S: // n = 5
            if (mesi == MODIFIED) if (mode) fprintf(stderr, "FLUSHING CONTENTS TO DRAM");
        case WRITE_S: // n = 4
        case INVALIDATE_S: // n = 6
        case CLEAR: // n = 8


            mesi = INVALID;
            break;
        default:
            fprintf(stderr,"Invalid operation input");
            exit(-1);
    }

    return;

}


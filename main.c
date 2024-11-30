/*####################################################################
* ECE 485/585 Final Prroject
* Team 4
* 
* Ameer Melli
* Caleb Monti
* Chris Kane-Pardy
* 
* This is the test branch
* gcc main.c -o test -DDEBUG (to run debug mode with prints)
* gcc main.c -o test (run without Debug Mode Active)
* ./test (run code)
* ./test rwims.din (to run with a file name)
######################################################################*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "defines.c"

/*####################### Global variables ###########################*/
int mode = 0;       // Default mode for output is Silent
uint32_t address;
int operation;

/*#################### Global Type Definitions #######################*/
typedef struct {
    int mesi; // MESI state
    int tag;  // Tag
} Way;

typedef struct {
    int *plru;       // Pointer for PLRU bits (dynamic array)
    Way **ways;      // Pointer to array of Way pointers (dynamic array)
} Set;

/*##################### Function Prototypes ##########################*/

// Function to extract tag, index, and byte select from an address
void extract_address_components(unsigned int address, int *tag, int *set_index, int *byte_select, int tag_bits, int index_bits, int byte_select_bits);
// Update PLRU in active set to reflect MRU way
void UpdatePLRU(int PLRU[], int w );
// Find LRU way in active set for eviction
uint8_t VictimPLRU(int PLRU[], Way *way);
// Extract 2 LSB's of address to determine HIT, HITM, or NOTHIT
int GetSnoopResult(unsigned int address);
// Determine if newest input address is a Hit or Miss, and act accordingly
int hit_or_miss(Set *index[], int set_index, int tag);
// Determine MESI state updates based upon Snoop Results
void MESI_set(int* mesi, unsigned int address, int operation, int hm);

int main(int argc, char *argv[]) {


    // Set default filename
    char *default_filename = "Default.din";
    char *filename = default_filename;

    int cache_size = CACHE_SIZE;

    

    // Flags for setting non-default variable vvalues

    for( argc--, argv++; argc > 0; argc-=2, argv+=2 ) {
		if (strcmp(argv[0], "-m" ) == 0 ) 
            mode = atoi(argv[1]); // Set normal operation
		else if (strcmp(argv[0], "-f" ) == 0 ) 
            filename = argv[1]; // Set input file
        else if (strcmp(argv[0], "-c") == 0)
            cache_size = atoi(argv[1]);
		else { 
			printf("\nInvalid Arguments\n"); exit(-1); 
		}
	}

    // Check if user provided a filename as an argument

    FILE *file = fopen(filename, "r");

    // If the provided file can't be opened, try the default file
    if (file == NULL) {
        fprintf(stderr, "The '%s' file provided could not be opened. Attempting to open default file '%s'.\n", filename, default_filename);
        filename = default_filename;
        file = fopen(filename, "r");
    }

    // Check if the file (either provided or default) was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Display active file name
    fprintf(stderr, "Using file: %s\n", filename);

    /* MEMORY SIZE CALCULATIONS */
    const int TRUE_CAPACITY = pow(2,cache_size);
    const int LINES = (TRUE_CAPACITY / CACHE_LINE_SIZE);
    const int SETS = (LINES / ASSOCIATIVITY);


    /* TAG ARRAY CALCULATIONS 
    Can be used for testing to verify that values can be changed correctly for modularity*/
    const int BYTE_SELECT_BITS = log2(CACHE_LINE_SIZE);
    const int INDEX_BITS = log2(SETS);
    const int TAG_BITS = ADDRESS_SIZE - (BYTE_SELECT_BITS + INDEX_BITS);
    const int PLRU_ARRAY_SIZE = (ASSOCIATIVITY - 1);
    const int TOTAL_TAG_ARRAY = SETS * ((ASSOCIATIVITY * (TAG_BITS + TAG_ARRAY_MESI)) + PLRU_ARRAY_SIZE); 	

    /* Include tests for if the tag is too big or index is too big
    (go to default if the input values are out of bounds). */

    int plru_array[PLRU_ARRAY_SIZE]; //Initialize PLRU Array

    Set *index[SETS]; // Array of Set Pointers

    /*################## Initializing Data Structures #################*/
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
    // Print # of Sets for Debugging
    #ifdef DEBUG
    fprintf(stderr, "Number of sets: %d\n", SETS);
    #endif

    

    int tag, set_index, byte_select;

    // Read each line until end of file
    while (fscanf(file, "%d %x", &operation, &address) == 2) {
        // Breakdown address into proper components for storage in the cache
        extract_address_components(address, &tag, &set_index, &byte_select, TAG_BITS, INDEX_BITS, BYTE_SELECT_BITS);
        // Determine if Hit or Miss on CPU command
        if (operation == READ_HD | operation == READ_HI | operation == WRITE_HD) {
        // Call "hit_or_miss" to determine result
        int CacheResult = hit_or_miss(index, set_index, tag);
        // Print CacheResult
        if (CacheResult) {
         if(mode) printf("Hit!\n");
        } else {
        if (mode) printf("Miss!\n");
        }} else {
        // If Debug is Active, Display Operation and Address Components
        #ifdef DEBUG
        fprintf(stderr, "Operation: %d, Address: 0x%X\n", operation, address);
        fprintf(stderr, "Extracted Tag: 0x%X\n", tag);
        fprintf(stderr, "Extracted Index: 0x%X\n", set_index);
        fprintf(stderr, "Extracted Byte Select: 0x%X\n", byte_select);
        #endif
        }

    }
    fclose(file);  // Close the file

return 0;
}

/*#################### Function declarations ####################*/

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

// Determine if newest input address is a Hit or Miss, and act accordingly
int hit_or_miss(Set *index[], int set_index, int tag){
    // Refresh InvalidWays for each function run
    int InvalidWays = -1;
    // Read through each way in the active set to determine if hit
    for (int i = 0; i < ASSOCIATIVITY; i++){
        Way *way = index[set_index]->ways[i];

        #ifdef DEBUG
        fprintf(stderr, "Current tag: 0x%2X, New tag: 0x%2X\n" , way->tag, tag );
        fprintf(stderr, "Current MESI bits: %d\n", way->mesi);
        #endif
        // Hit if MESI bits are not "INVALID" and address tag == cached tag

        if (way->mesi != INVALID && way->tag == tag) {
            // Update PLRU to reflect MRU (Hit)
            UpdatePLRU(index[set_index]->plru, i);
            MESI_set( &(index[set_index]->ways[i]->mesi), address, operation, 1);
            return 1; //Hit
        }
        //If not a hit, and there is an invalid way, mark the invalid way to be "filled" upon miss
        else if (way->mesi == INVALID){
            InvalidWays = i;
        }
    }

    // Miss with invalid ways in the current set
    if(InvalidWays >= 0) {
        Way *invalid_way =index[set_index]->ways[InvalidWays]; // Navigate to way in active set with an INVALID MESI state
        invalid_way->tag = tag; // Place new tag at current way
        MESI_set(&(index[set_index]->ways[InvalidWays]->mesi), address, operation, 0); // Update MESI State based off snoop results
        UpdatePLRU(index[set_index]->plru, InvalidWays); // Update PLRU for the newly entered way
    // Miss with a full set
    } else {
        int victim_line = VictimPLRU(index[set_index]->plru, *index[set_index]->ways); // Decide which way is a the LRU
        Way *victim_eviction = index[set_index]->ways[victim_line];
        victim_eviction ->tag = tag; // Replace the victim ways tag with current tag
        MESI_set(&(victim_eviction->mesi), address, operation, 0); // Update MESI State based off snoop results
        UpdatePLRU(index[set_index]->plru, victim_line); // Update PLRU for the newly entered way
    }
    return 0; //Miss
}


/* Update PLRU in active set to reflect MRU way 
Takes in specific PLRU for that set as an argument */
void UpdatePLRU(int PLRU[], int w ){
    int bit = 0;
    // Check for any possible errors
    if (w > (ASSOCIATIVITY-1) || w < 0){
        fprintf(stderr,"Value of w not allowed for PLRU updating\n");
        exit(-1);
    }
    // Determine the most updated way by moving through the array
    for (int i = 0; i < log2(ASSOCIATIVITY); i++){
        // If first line is 0 >> Left, if it is 1 >> Right
        if ( (0x1 & (w >> ( (int)(log2((int)ASSOCIATIVITY) -1)-i ) )) == 0 )  {
            PLRU[bit] = 0;
            bit = (2*bit) + 1; // Left Child
        }

        else {
            PLRU[bit] = 1;
            bit = (2*bit) + 2; //Right Child
        }
    }
    return;
}

/* Find LRU way in active set for eviction
Takes in specific PLRU for that set as an argument */
uint8_t VictimPLRU(int PLRU[], Way *way){

    int b = 0;
    // Traverse the PLRU array for replacement the opposite way of accesses
    for (int i = 0; i < log2(ASSOCIATIVITY); i++){
        #ifdef DEBUG
            fprintf(stderr,"Triggering PLRU check #%d with val %d, ", i, PLRU[b]);
            fprintf(stderr, "B = %d, \n", b);
        #endif
        if(PLRU[b]) b = 2 * b + 1;
        else b = 2 * b + 2;
    }
    way[b].mesi = INVALID;
    b = b - (ASSOCIATIVITY - 1);
    return b;

}

// Extract 2 LSB's of address to determine HIT, HITM, or NOTHIT
int GetSnoopResult(unsigned int address) {
// Mask 2LSB'f of address & return for use in MESI funtion
    return address & 0x3;

}

// Determine MESI state updates based upon Snoop Results
void MESI_set(int* mesi, unsigned int address, int operation, int hm){

    int snoop = GetSnoopResult(address);


    switch (operation){

        
        case READ_HD: // n = 0
        case READ_HI: // n = 2
            #ifdef DEBUG
            fprintf(stderr, "Case 0 or 2\n");
            #endif

            if(hm) break;

            if (mode) printf("Performing a BusRead Operation\n");
            if (snoop == HIT){

                if (mode) printf("Snoop Result was HIT, reading from DRAM, moving to Shared\n");
                *mesi = SHARED;
            }
            else if (snoop == HITM && operation != 1) {

                if (mode) printf("Snoop Result was HITM, waiting for writeback, reading from DRAM, then moving to Shared\n");
                *mesi = SHARED;
            }
            else{

                if (mode) printf("Snoop Result was MISS, reading from DRAM, moving to Exclusive\n");
                *mesi = EXCLUSIVE;
            }
            #ifdef DEBUG
            fprintf(stderr, "New MESI: %d\n", *mesi);
            #endif

            break;
        case WRITE_HD: // n = 1
            if (*mesi == INVALID) { 
                if (mode) printf("Performing a BusRdX Operation for address 0x%8X, moving to Modified\n", address);
                *mesi = MODIFIED;
            } 
            else if (*mesi == SHARED) { 
                if (mode) printf("Performing a BusUpgr Operation for address 0x%8X, moving to Modified\n", address);
                *mesi = MODIFIED;
            }
            else if (*mesi == EXCLUSIVE) { 
                if (mode) printf("No Bus Operation, moving to Modified\n");
                *mesi = MODIFIED;
            }
            else {
                if (mode) printf("No Bus Operation, already in Modified\n");

            }
            
            break;
        case READ_S: // n = 3

            if (*mesi == EXCLUSIVE) {
                if (mode) printf("Snooped a BusRd Operation for address 0x%8X while I am Exclusive, moving to Shared\n", address);
                if (mode) printf("Echoing a HIT for address 0x%8X\n", address);
                *mesi = SHARED;
            }
            else if (*mesi == MODIFIED) {
                if (mode) printf("Snooped a BusRd Operation for address 0x%8X while I have it Modified, flushing contents to DRAM and moving to Shared\n", address);
                if (mode) printf("Echoing a HITM for address 0x%8X\n", address);
                *mesi = SHARED;
            }
            else if (*mesi == SHARED) { 
                if (mode) printf("Snooped a BusRd Operation for address 0x%8X while I have it Shared, no changes here\n", address); 
                if (mode) printf("Echoing a HIT for address 0x%8X\n", address);
                }
            else{ 
                if(mode) printf("Snooped a BusRd Operation for address 0x%8X while I have it Invalid, no changes here\n", address);
                if(mode) printf("Echoing a MISS for address 0x%8X\n", address);
            }
        
        case RWIM_S: // n = 5
            if (*mesi == MODIFIED) { if (mode) printf("Snooped a BusRdX Operation for address 0x%8X while I have it Modified, flushing contents to DRAM and Invalidating \n", address); }
            else if (*mesi == EXCLUSIVE) { if (mode) printf("Snooped a BusRdX Operation for address 0x%8X while I have it Exclusive, Invalidating \n", address); }
            else if (*mesi == SHARED) { fprintf(stderr, "0x%8X Address shared and gathered RWIM. Not a valid state.\n", address); }
            else {  if(mode) printf("Snooped a BusRdX for address 0x%8X while I have it Invalid, no changes here\n", address); }
            *mesi = INVALID;
            break;

        case WRITE_S: // n = 4
            if (*mesi == MODIFIED) { if (mode) printf("Snooped a Write Request for address 0x%8X while I have it Modified, flushing contents to DRAM and Invalidating \n", address); }
            else if (*mesi == EXCLUSIVE) { if (mode) printf("Snooped a Write Request for address 0x%8X while I have it Exclusive, Invalidating \n", address); }
            else if (*mesi == SHARED) { if (mode) printf("Snooped a Write Request for address 0x%8X while I have it shared, Invalidating\n", address); }
            else {  if(mode) printf("Snooped a Write Request for address 0x%8X while I have it Invalid, no changes here\n", address); }
            *mesi = INVALID;
            break;

        case INVALIDATE_S: // n = 6
            if (*mesi == MODIFIED) { if (mode) fprintf(stderr, "0x%8X Address Modified and gathered Invalidate. Not a valid state.\n", address); }
            else if (*mesi == EXCLUSIVE) { if (mode) fprintf(stderr, "0x%8X Address Exclusive and gathered Invalidate. Not a valid state.\n", address); }
            else if (*mesi == SHARED) { if (mode) printf("Snooped BusUpgr Operation for address 0x%8X while I have it shared, Invalidating\n", address); }
            else {  if(mode) printf("Snooped a BusUpgr Operation for address 0x%8X while I have it Invalid, no changes here\n", address); }
            *mesi = INVALID;
            break;

        default:
            fprintf(stderr,"Invalid Operation");
            exit(-1);
    }

        
    return;

}


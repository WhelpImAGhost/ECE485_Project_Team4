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
char mesi_state[12] = "INVALID";
char snoop_state[5] = "MISS";

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
int GetSnoopResult(char* snoop_state);
// Determine if newest input address is a Hit or Miss, and act accordingly
int hit_or_miss(Set *index[], int set_index, int tag, char* mesi_state);
// Determine MESI state updates based upon Snoop Results
void MESI_set(int* mesi, int operation, int hm);
// Clear cache contents for Operation Code 8
void clear_cache (Set *index[], int sets, int plru_size, int assoc);
// Print cache contents for Operation Code 9
void print_cache (Set *index[], int sets, int plru_size, int assoc);
// Print correct statements to simulate communication from this cache to higher cache level
void inclusive_print(int state);

int main(int argc, char *argv[]) {


    // Set default filename
    char *default_filename = "Default.din";
    char *filename = default_filename;
    int cache_size = CACHE_SIZE;
    int tag, set_index, byte_select;
    int CacheResult;


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

    #ifdef DEBUG
        mode = 1;
    #endif

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
    const int TOTAL_TAG_ARRAY = (SETS * ((ASSOCIATIVITY * (TAG_BITS + TAG_ARRAY_MESI)) + PLRU_ARRAY_SIZE)) / 8; 	

    #ifdef DEBUG
    fprintf(stderr,"Cache Capacity: %d bytes, # of Cache Lines: %d, # of Sets: %d\n",TRUE_CAPACITY,LINES,SETS);
    fprintf(stderr," # of Byte Select Bits: %d bits, # of Tndex Bits: %d bits, # of Tag Bits: %d bits\n",BYTE_SELECT_BITS,INDEX_BITS,TAG_BITS);
    fprintf(stderr," PLRU Array Size: %d bits per set, Total Tag Array Size: %d bytes\n",PLRU_ARRAY_SIZE,TOTAL_TAG_ARRAY);
    #endif

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

    // Read each line until end of file
    while (fscanf(file, "%d %x", &operation, &address) == 2) {

        // Breakdown address into proper components for storage in the cache
        extract_address_components(address, &tag, &set_index, &byte_select, TAG_BITS, INDEX_BITS, BYTE_SELECT_BITS);

        // If Debug is Active, Display Operation and Address Components
        #ifdef DEBUG
            fprintf(stderr, "Operation: %d, Address: 0x%X\n", operation, address);
            fprintf(stderr, "Extracted Tag: 0x%X\n", tag);
            fprintf(stderr, "Extracted Index: 0x%X\n", set_index);
            fprintf(stderr, "Extracted Byte Select: 0x%X\n", byte_select);
        #endif

        switch (operation) {

            case READ_HD:           /* Read request from higher data cache */
                #ifdef DEBUG
                    fprintf(stderr, "Case 0\n");
                #endif
                CacheResult = hit_or_miss(index, set_index, tag, mesi_state);
                if (CacheResult) {
                    if(mode){printf("PrRd HIT @ 0x%08X, MESI State: %s\n", address, mesi_state);
                    }
                }else {
                    if(mode){
                        printf("PrRd MISS @ 0x%08X\n", address);
                        printf("BusRd @ 0x%08X, Snoop Result:, MESI State: %s\n", (address & ~(0x3F))), mesi_state; //TODO Snoop Result
                        inclusive_print(SENDLINE);
                    }
                }
                break;

            case WRITE_HD:		    /* Write request from higher data cache */
                #ifdef DEBUG
                    fprintf(stderr, "Case 1\n");
                #endif
                CacheResult = hit_or_miss(index, set_index, tag, mesi_state);
                break;

                
            case READ_HI: 		    /* Read request from higher instruction cache */
                #ifdef DEBUG
                    fprintf(stderr, "Case 2\n");
                #endif
                CacheResult = hit_or_miss(index, set_index, tag, mesi_state);
                if (CacheResult) {
                    if(mode){ printf("PrRd HIT @ 0x%08X, %s\n", address,mesi_state ); //TODO add MESI bits
                    }
                }else {
                    if(mode){ 
                        printf("PrRd MISS @ 0x%08X\n", address);
                        printf("BusRd @ 0x%08X, Snoop Result:\n", (address & ~(0x3F))); //TODO Snoop Result
                        inclusive_print(SENDLINE); //Add Mesi Bit
                    }
                }
                break;


            case READ_S: 		    /* Snoop read request */
                #ifdef DEBUG
                    fprintf(stderr, "Case 3\n");
                #endif
                break;

                
            case WRITE_S: 		    /* Snoop write request */
                #ifdef DEBUG
                    fprintf(stderr, "Case 4\n");
                #endif
                break;


            case RWIM_S: 		    /* Snoop read with intent to modify request */
                #ifdef DEBUG
                    fprintf(stderr, "Case 5\n");
                #endif
                break;


            case INVALIDATE_S: 	    /* Snoop invalidate command */
                #ifdef DEBUG
                    fprintf(stderr, "Case 6\n");
                #endif
                break;


            case CLEAR: 		    /* Clear the cache and reset all states */
                #ifdef DEBUG
                    fprintf(stderr, "Case 8\n");
                #endif
                clear_cache(index, SETS, PLRU_ARRAY_SIZE, ASSOCIATIVITY);
                break;


            case PRINT: 		    /* Print content and state of each valid cache line */
                #ifdef DEBUG
                    fprintf(stderr, "Case 9\n");
                #endif
                print_cache(index, SETS, PLRU_ARRAY_SIZE, ASSOCIATIVITY);
                break;


            default:
                fprintf(stderr,"Invalid Operation");
                exit(-1);
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
int hit_or_miss(Set *index[], int set_index, int tag, char* mesi_state){
    // Refresh InvalidWays for each function run
    int InvalidWays = -1;
    // Read through each way in the active set to determine if hit
    for (int i = 0; i < ASSOCIATIVITY; i++){
        Way *way = index[set_index]->ways[i];

        #ifdef DEBUG
        //fprintf(stderr, "Current tag: 0x%2X, New tag: 0x%2X\n" , way->tag, tag );
        //fprintf(stderr, "Current MESI bits: %d\n", way->mesi);
        #endif
        // Hit if MESI bits are not "INVALID" and address tag == cached tag

        if (way->mesi != INVALID && way->tag == tag) {
            // Update PLRU to reflect MRU (Hit)
            UpdatePLRU(index[set_index]->plru, i);
            MESI_set( &(index[set_index]->ways[i]->mesi), operation, 1);
            strcpy(mesi_state, (way->mesi == INVALID) ? "INVALID" : ((way->mesi == EXCLUSIVE) ? "EXCLUSIVE" : ((way->mesi == SHARED) ? "SHARED" : ((way->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));
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
        MESI_set(&(index[set_index]->ways[InvalidWays]->mesi), operation, 0); // Update MESI State based off snoop results
        strcpy(mesi_state, (invalid_way->mesi == INVALID) ? "INVALID" : ((invalid_way->mesi == EXCLUSIVE) ? "EXCLUSIVE" : ((invalid_way->mesi == SHARED) ? "SHARED" : ((invalid_way->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));
        UpdatePLRU(index[set_index]->plru, InvalidWays); // Update PLRU for the newly entered way
    // Miss with a full set
    } else {
        int victim_line = VictimPLRU(index[set_index]->plru, *index[set_index]->ways); // Decide which way is a the LRU
        inclusive_print(INVALIDATELINE);
        Way *victim_eviction = index[set_index]->ways[victim_line];
        victim_eviction ->tag = tag; // Replace the victim ways tag with current tag
        MESI_set(&(victim_eviction->mesi), operation, 0); // Update MESI State based off snoop results
        strcpy(mesi_state, (victim_eviction->mesi == INVALID) ? "INVALID" : ((victim_eviction->mesi == EXCLUSIVE) ? "EXCLUSIVE" : ((victim_eviction->mesi == SHARED) ? "SHARED" : ((victim_eviction->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));
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
int GetSnoopResult(char* snoop_state) {
// Mask 2LSB'f of address & return for use in MESI funtion
    int val = address & 0x3;
    
    strcpy(snoop_state, (val == HIT ? "HIT" : (val == HITM ? "HITM" : "MISS")));
    
    return val;

}

void SendSnoopResult(int* mesi){

    switch(*mesi) {

        case INVALID:
            if(mode) printf("L2 result: MISS\n");
            break;
        case EXCLUSIVE:
            if(mode) printf("L2 result: HIT\n");
            break;
        case SHARED:
            if(mode) printf("L2 result: HIT\n");
            break;
        case MODIFIED:
            if(mode) printf("L2 result: HITM\n");
            break;
        default:
            fprintf(stderr, "Invalid MESI state\n");
            exit(-1);

    }

}

// Determine MESI state updates based upon Snoop Results
void MESI_set(int* mesi, int operation, int hm){

    int snoop = GetSnoopResult(snoop_state);


    switch (operation){

        
        case READ_HD: // n = 0
        case READ_HI: // n = 2
            #ifdef DEBUG
            fprintf(stderr, "Case 0 or 2\n");
            #endif

            if(hm) break;

            if (snoop == HIT){
                *mesi = SHARED;
            }
            else if (snoop == HITM && operation != 1) {

                *mesi = SHARED;
            }
            else{

                *mesi = EXCLUSIVE;
            }
            #ifdef DEBUG
            fprintf(stderr, "New MESI: %d\n", *mesi);
            #endif

            break;
        case WRITE_HD: // n = 1
            if (*mesi == INVALID) { 
                *mesi = MODIFIED;
            } 
            else if (*mesi == SHARED) { 
                *mesi = MODIFIED;
            }
            else if (*mesi == EXCLUSIVE) { 
                *mesi = MODIFIED;
            }
            else {
                *mesi = MODIFIED;

            }
            
            break;
        case READ_S: // n = 3

            if (*mesi == EXCLUSIVE) {
                *mesi = SHARED;
            }
            else if (*mesi == MODIFIED) {
                *mesi = SHARED;
            }
            else if (*mesi == SHARED) { 
                }

        case RWIM_S: // n = 5
            *mesi = INVALID;
            break;

        case WRITE_S: // n = 4
            *mesi = INVALID;
            break;

        case INVALIDATE_S: // n = 6
            *mesi = INVALID;
            break;

        default:
            fprintf(stderr,"Invalid Operation");
            exit(-1);
    }

    return;

}

void clear_cache (Set *index[], int sets, int plru_size, int assoc) {

    for (int i = 0; i < sets; i++){

        for (int j = 0; j < plru_size; j++) {
            index[i]->plru[j] = 0;
        }

        for (int k = 0; k < assoc; k++) {
            index[i]->ways[k]->mesi = INVALID;
            index[i]->ways[k]->tag = 0;
        }
    }
    return;
}


void print_cache (Set *index[], int sets, int plru_size, int assoc) {

    for (int i = 0; i < sets; i++){

        printf("Set %d PLRU: ", i);
        for (int j = 0; j < plru_size; j++){
            printf("%d", index[i]->plru[j]);
        }
        printf("\n");
        for (int k = 0; k < assoc; k++) {
            if (index[i]->ways[k]->mesi != INVALID) {
                printf("    Cache contents for Set %d way %d: MESI STATE %d, TAG: %4X\n", i, k, index[i]->ways[k]->mesi, index[i]->ways[k]->tag );
            }
        }
    }
    return;
}


void inclusive_print(int state){

    switch(state) {

        case GETLINE:
            if(mode) printf("L2: GETLINE 0x%08X\n", address);
            break;
        case SENDLINE:
            if(mode) printf("L2: SENDLINE 0x%08X\n", address);
            break;
        case INVALIDATELINE:
            if(mode) printf("L2: INVALIDATELINE 0x%08X\n", address);
            break;
        case EVICTLINE:
            if(mode) printf("L2: EVICTLINE 0x%08X\n", address);
            break;
        default:
            fprintf(stderr, "Invalid state in inclusive_print function\n");
            exit(-1);
    }

    return;
}
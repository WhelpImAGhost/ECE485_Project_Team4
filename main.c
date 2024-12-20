/*####################################################################
* ECE 485/585 Final Prroject
* Team 4
* 
* Ameer Melli
* Caleb Monti
* Chris Kane-Pardy
* 
* gcc main.c -o test -DDEBUG (to run debug mode with prints)
* gcc main.c -o test (run without Debug Mode Active)
* ./test (run code)
######################################################################*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "defines.c"

/*####################### Global variables ###########################*/
int mode = 0;                  // Default mode for output is Silent
uint32_t address;              // Input Address
int operation;                 // Cache Operation
int old_mesi_state = INVALID;
int old_address = 0x00000000;
int byte_select_bits = 0;
int index_bits = 0;
int tag_bits = 0;
char mesi_state[12] = "INVALID";
char snoop_state[5] = "NOHIT";
char snoop_reply[5] = "NOHIT";

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
void extract_address_components(unsigned int address, int *tag, int *set_index, int *byte_select);
// Update PLRU in active set to reflect MRU way
void UpdatePLRU(int PLRU[], int w );
// Find LRU way in active set for eviction
uint8_t VictimPLRU(int PLRU[], Way *way);
// Extract 2 LSB's of address to determine HIT, HITM, or NOTHIT
int GetSnoopResult(char* snoop_state);
// Determine if newest input address is a Hit or Miss, and act accordingly
int SnoopChecker(Set *index[], int set_index, int tag);
// Produce L2 Cache snoop result based on MESI bits of active way
void SendSnoopResult(int SnoopCheck, char* snoop_reply);
// Determine if newest input address is a Hit or Miss, and act accordingly
int hit_or_miss(Set *index[], int set_index, int tag);
// Determine MESI state updates based upon Snoop Results
void MESI_set(int* mesi, int operation, int hm);
// Clear cache contents for Operation Code 8
void clear_cache (Set *index[], int sets, int plru_size, int assoc);
// Print cache contents for Operation Code 9
void print_cache (Set *index[], int sets, int plru_size, int assoc);
// Print the cache statistics for hits, misses, reads, writes, and ratios 
void cache_statistics(int operation, int CacheResult, bool finished_program);
// Print correct statements to simulate communication from this cache to higher cache level
void inclusive_print(int state);

int main(int argc, char *argv[]) {


    // Set default filename
    char *default_filename = "Other_dins/Default.din";
    char *filename = default_filename;

    // Cache usage and tracking variables
    int cache_size = CACHE_SIZE;
    int tag, set_index, byte_select;
    int CacheResult;
    int SnoopReply;


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

    // If in debug mode, always set verbosity to "normal mode"
    #ifdef DEBUG
        mode = 1;
    #endif

    /*################## File and Variable Initialization #################*/

    // Load given file (if there was one)
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
        // Exit the program if file opening fails
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
    byte_select_bits = BYTE_SELECT_BITS;
    const int INDEX_BITS = log2(SETS);
    index_bits = INDEX_BITS;
    const int TAG_BITS = ADDRESS_SIZE - (BYTE_SELECT_BITS + INDEX_BITS);
    tag_bits = TAG_BITS;
    const int PLRU_ARRAY_SIZE = (ASSOCIATIVITY - 1);
    const int TOTAL_TAG_ARRAY = (SETS * ((ASSOCIATIVITY * (TAG_BITS + TAG_ARRAY_MESI)) + PLRU_ARRAY_SIZE)) / 8; 	

    // Display in-depth cache values if in debug mode
    #ifdef DEBUG
    fprintf(stderr,"\nCache Capacity: %d bytes, # of Cache Lines: %d, # of Sets: %d\n",TRUE_CAPACITY,LINES,SETS);
    fprintf(stderr,"# of Byte Select Bits: %d bits, # of Tndex Bits: %d bits, # of Tag Bits: %d bits\n",BYTE_SELECT_BITS,INDEX_BITS,TAG_BITS);
    fprintf(stderr,"PLRU Array Size: %d bits per set, Total Tag Array Size: %d bytes\n",PLRU_ARRAY_SIZE,TOTAL_TAG_ARRAY);
    #endif

    // Prepare cache initialization variables
    int plru_array[PLRU_ARRAY_SIZE]; //Initialize PLRU Array
    Set *index[SETS]; // Array of Set Pointers

    // This checks if the input file finished for the cache stats
    bool finished_program = false;

    /*################## Initializing Data Structure #################*/
    for (int i = 0; i < SETS; i++) {

        // Allocate space for the Set
        index[i] = (Set *)malloc(sizeof(Set));
        if (index[i] == NULL) {
            perror("Failed to allocate memory for Set");
            exit(EXIT_FAILURE);
        }

        // Initialize and allocate the PLRU for the Set
        index[i]->plru = (int *)malloc(PLRU_ARRAY_SIZE * sizeof(int));
        if (index[i]->plru == NULL) {
            perror("Failed to allocate memory for PLRU array");
            exit(EXIT_FAILURE);
        }

        // Set each PLRU bit to 0 for the Set
        for (int j = 0; j < PLRU_ARRAY_SIZE; j++) {
            index[i]->plru[j] = 0;
        }

        // Initialize and allocate the storage for all Ways in the Set
        index[i]->ways = (Way **)malloc(ASSOCIATIVITY * sizeof(Way *));
        if (index[i]->ways == NULL) {
            perror("Failed to allocate memory for Way array");
            exit(EXIT_FAILURE);
        }

        // For each Way, allocate space
        for (int k = 0; k < ASSOCIATIVITY; k++) {
            index[i]->ways[k] = (Way *)malloc(sizeof(Way));
            if (index[i]->ways[k] == NULL) {
                perror("Failed to allocate memory for Way");
                exit(EXIT_FAILURE);
            }

            // Set each tag to 0 and each MESI state to INVALID
            index[i]->ways[k]->mesi = INVALID;
            index[i]->ways[k]->tag = 0;
        }
    }

    // Read each line until end of file
    while (fscanf(file, "%d %x", &operation, &address) == 2) {

        // Breakdown address into proper components for storage in the cache
        extract_address_components(address, &tag, &set_index, &byte_select);

        // If Debug is Active, Display Operation and Address Components
        #ifdef DEBUG
            fprintf(stderr, "Operation: %d, Address: 0x%X\n", operation, address);
            fprintf(stderr, "Extracted Tag: 0x%X\n", tag);
            fprintf(stderr, "Extracted Index: 0x%X\n", set_index);
            fprintf(stderr, "Extracted Byte Select: 0x%X\n", byte_select);
        #endif

        // Run functions based on selected operation
            // Function descriptions listed in Function Prototypes 
            // Detailed function operation in functions section below main function
        switch (operation) {

            case READ_HD:           /* Read request from higher data cache */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 0\n");
                #endif
                CacheResult = hit_or_miss(index, set_index, tag);
                cache_statistics(operation, CacheResult, finished_program);
                if (CacheResult == 1) {
                    if(mode){printf("\nPrRd HIT @ 0x%08X, MESI State: %s\n", address, mesi_state);
                    inclusive_print(SENDLINE);
                    }
                }else if (CacheResult == 0){
                    if(mode){
                        printf("\nPrRd MISS @ 0x%08X\n", address);
                        printf("L2: BusRd @ 0x%08X, Snoop Result: %s\n", (address & ~(0x3F)), snoop_state);
                        if(strcmp(snoop_state,"HITM")==0){
                            printf("Snooped Operation: FlushWB @ 0x%08X\n", address);
                        }
                        printf("L2 Mesi State: %s\n", mesi_state);                        
                        inclusive_print(SENDLINE);
                    }
                } else if (CacheResult == 2){
                    if(mode){
                        if (old_mesi_state == MODIFIED){
                            printf("\nPrRd COLLISION MISS @ 0x%08X\n", address);
                            printf("L2: EVICTLINE 0x%08X\n", (old_address));
                            printf("L2: FlushWB @ 0x%08X, L2 MESI State: INVALID\n", (old_address));
                            printf("L2: BusRd @ 0x%08X, Snoop Result: %s, MESI State: %s\n", (address & ~(0x3F)), snoop_state, mesi_state);
                            if(strcmp(snoop_state,"HITM")==0){
                                printf("Snooped Operation: FLushWB @0x%08X\n", address);
                                inclusive_print(SENDLINE);
                            }
                            else{
                                inclusive_print(SENDLINE);
                            }
                        } else if ((old_mesi_state == EXCLUSIVE) || (old_mesi_state == SHARED)){
                        printf("\nPrRd COLLISION MISS @ 0x%08X\n", address);
                        printf("L2: INVALIDATELINE 0x%08X\n", (old_address));
                        printf("L2: BusRd @ 0x%08X, Snoop Result: %s, MESI State: %s\n", (address & ~(0x3F)), snoop_state, mesi_state);
                        inclusive_print(SENDLINE);
                        }
                    }
                }
                break;

            case WRITE_HD:		    /* Write request from higher data cache */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 1\n");
                #endif
                CacheResult = hit_or_miss(index, set_index, tag);
                cache_statistics(operation, CacheResult, finished_program);
                if (CacheResult == 1) {
                    if(mode){
                        printf("\nPrWr HIT @ 0x%08X, L2 Mesi State: %s\n", address, mesi_state);
                        if ( old_mesi_state == EXCLUSIVE || old_mesi_state == MODIFIED ){
                            inclusive_print(SENDLINE);
                        }
                        else if( old_mesi_state == SHARED ){
                            printf("L2: BusUpgr @ 0x%08X\n", (address & ~(0x3F)));
                            inclusive_print(SENDLINE);
                        }
                    }
                }else if (CacheResult == 0){
                        if(mode){
                            printf("\nPrWr MISS @ 0x%08X\n", address);
                            if(strcmp(snoop_state,"HITM") == 0){
                            printf("L2: BusRdx @ 0x%08X, MESI State: %s\n", (address & ~(0x3F)), mesi_state);
                            printf("Snooped Operation: FlushWB @ 0x%08X\n", address);
                            inclusive_print(SENDLINE);
                            }
                            else{
                            printf("L2: BusRdx @ 0x%08X, MESI State: %s\n", (address & ~(0x3F)), mesi_state);
                            inclusive_print(SENDLINE);
                            }
                        }
                } else if (CacheResult == 2){
                    if(mode){
                        if (old_mesi_state == MODIFIED){
                            printf("\nPrWr COLLISION MISS @ 0x%08X\n", address);
                            printf("L2: EVICTLINE 0x%08X\n", (old_address));
                            printf("L2: FlushWB @ 0x%08X, L2 MESI State: INVALID\n", (old_address)); //Writeback Old Address
                            printf("L2: BusRdX @ 0x%08X, MESI State: %s\n", (address & ~(0x3F)), mesi_state);
                            if(strcmp(snoop_state,"HITM")==0){
                                printf("Snooped Operation: FLushWB @0x%08X\n", address);
                                inclusive_print(SENDLINE);
                            }
                            else{
                                inclusive_print(SENDLINE);
                            }
                        } else if ((old_mesi_state == EXCLUSIVE) || (old_mesi_state == SHARED)){
                        printf("\nPrWr COLLISION MISS @ 0x%08X\n", address);
                       printf("L2: INVALIDATELINE 0x%08X\n", (old_address));
                        printf("L2: BusRdX @ 0x%08X, MESI State: %s\n", (address & ~(0x3F)), mesi_state);
                        inclusive_print(SENDLINE);
                        }
                    }
                }
                break;

                
            case READ_HI: 		    /* Read request from higher instruction cache */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 2\n");
                #endif
                CacheResult = hit_or_miss(index, set_index, tag);
                cache_statistics(operation, CacheResult, finished_program);
                if (CacheResult ==1 ) {
                    if(mode){ printf("\nPrRd HIT @ 0x%08X, MESI State: %s\n", address, mesi_state);
                    inclusive_print(SENDLINE);
                    }
                }else if (CacheResult == 0){
                    if(mode){ 
                        printf("\nPrRd MISS @ 0x%08X\n", address);
                        printf("L2: BusRd @ 0x%08X, Snoop Result: %s, MESI State: %s\n", (address & ~(0x3F)), snoop_state, mesi_state);
                        inclusive_print(SENDLINE);
                        if(strcmp(snoop_state,"HITM")==0){
                            printf("Snooped Operation: FLushWB @0x%08X\n", address);
                            inclusive_print(SENDLINE); //Add Mesi Bit
                        }
                    }
                }
                else if (CacheResult == 2){
                    if(mode){
                        if (old_mesi_state == MODIFIED){
                            printf("\nPrRd COLLISION MISS @ 0x%08X\n", address);
                            printf("L2: EVICTLINE 0x%08X\n", (old_address));
                            printf("L2: FlushWB @ 0x%08X, L2 MESI State: INVALID\n", (old_address));
                            printf("L2: BusRd @ 0x%08X, Snoop Result: %s, MESI State: %s\n", (address & ~(0x3F)), snoop_state, mesi_state);
                            if(strcmp(snoop_state,"HITM")==0){
                                printf("Snooped Operation: FLushWB @0x%08X\n", address);
                                inclusive_print(SENDLINE);
                            }
                            else{
                                inclusive_print(SENDLINE);
                            }
                        } else if ((old_mesi_state == EXCLUSIVE) || (old_mesi_state == SHARED)){
                        printf("\nPrRd COLLISION MISS @ 0x%08X\n", address);
                       printf("L2: INVALIDATELINE 0x%08X\n", (old_address));
                        printf("L2: BusRd @ 0x%08X, Snoop Result: %s, MESI State: %s\n", (address & ~(0x3F)), snoop_state, mesi_state);
                        inclusive_print(SENDLINE);
                        }
                    }
                }
                break;


            case READ_S: 		    /* Snoop read request */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 3\n");
                #endif
                SnoopReply = SnoopChecker(index, set_index, tag);
                SendSnoopResult(SnoopReply, snoop_reply);
                    if(mode){
                        printf("\nSnooped Operation: BusRd @ 0x%08X, L2 Snoop Result: %s\n", address, snoop_reply);
                        if(strcmp(snoop_reply,"HITM")==0){
                            inclusive_print(GETLINE);
                            printf("L2: FlushWB @ 0x%08X, L2 MESI State: %s\n", (address & ~(0x3F)), mesi_state);       
                        } else if((strcmp(mesi_state,"EXCLUSIVE")==0) || (strcmp(mesi_state,"SHARED")==0)){
                            printf("L2 MESI State: %s\n", mesi_state);
                        }
                    }
                break;

                
            case WRITE_S: 		    /* Snoop write request */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 4\n");
                #endif
                if(mode) printf("\nSnooped Operation: FlushWB @ 0x%08X\n", address);
                break;


            case RWIM_S: 		    /* Snoop read with intent to modify request */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 5\n");
                #endif
                SnoopReply = SnoopChecker(index, set_index, tag);
                SendSnoopResult(SnoopReply, snoop_reply);
                    if(mode){
                        printf("\nSnooped Operation: BusRdX @ 0x%08X, L2 Snoop Result: %s\n", address, snoop_reply);
                        if(strcmp(snoop_reply,"HITM")==0){
                            inclusive_print(EVICTLINE);
                            printf("L2: FlushWB @ 0x%08X, L2 MESI State: %s\n", address, mesi_state);       
                        } else if(strcmp(snoop_reply,"HIT")==0){
                            inclusive_print(INVALIDATELINE);
                            printf("L2 MESI State: %s\n", mesi_state);
                        }
                    }
                break;


            case INVALIDATE_S: 	    /* Snoop invalidate command */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 6\n");

                #endif
                SnoopReply = SnoopChecker(index, set_index, tag);
                SendSnoopResult(SnoopReply, snoop_reply);
                    if(mode){
                        printf("\nSnooped Operation: BusUpgr @ 0x%08X, L2 Snoop Result: %s\n", address, snoop_reply);
                        if(strcmp(snoop_reply,"HIT")==0){
                            inclusive_print(INVALIDATELINE);
                            printf("L2 MESI State: %s\n", mesi_state);
                        }
                    }
                break;


            case CLEAR: 		    /* Clear the cache and reset all states */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 8\n");
                #endif
                if(mode){
                    printf("\n--------------------------------Clearing Cache Contents--------------------------------\n");
                }
                clear_cache(index, SETS, PLRU_ARRAY_SIZE, ASSOCIATIVITY);
                if(mode){
                    printf("\n---------------------------------------------------------------------------------------\n");
                }
                break;


            case PRINT: 		    /* Print content and state of each valid cache line */
                #ifdef DEBUG
                    fprintf(stderr, "\nCase 9\n");
                #endif
                printf("\n--------------------------------Printing Cache Contents--------------------------------\n");      
                print_cache(index, SETS, PLRU_ARRAY_SIZE, ASSOCIATIVITY);
                printf("\n---------------------------------------------------------------------------------------\n");
                break;


            default:
                fprintf(stderr,"Invalid Operation");
                exit(-1);
        }


    }

    // Trigger end-of-file sequence and report cache statistics
    finished_program = true;
    cache_statistics(operation, CacheResult, finished_program);
    fclose(file);  // Close the file

    for (int i = 0; i < SETS; i++) {
    if (index[i] != NULL) {
        // Free each Way in the Set
        for (int k = 0; k < ASSOCIATIVITY; k++) {
            if (index[i]->ways[k] != NULL) {
                free(index[i]->ways[k]);
                index[i]->ways[k] = NULL;
            }
        }

        // Free the Ways array
        if (index[i]->ways != NULL) {
            free(index[i]->ways);
            index[i]->ways = NULL;
        }

        // Free the PLRU array
        if (index[i]->plru != NULL) {
            free(index[i]->plru);
            index[i]->plru = NULL;
        }

        // Free the Set itself
        free(index[i]);
        index[i] = NULL;
    }
}



return 0;
}

/*#################### Function declarations ####################*/

// Function to extract tag, index, and byte select from an address
void extract_address_components(unsigned int address, int *tag, int *set_index, int *byte_select) {
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
    strcmp(mesi_state,"INVALID");
    int InvalidWays = -1;
    // Read through each way in the active set to determine if hit
    for (int i = 0; i < ASSOCIATIVITY; i++){
        Way *way = index[set_index]->ways[i];

        // Excessive debug statement for high-detail MESI tracking
        #ifdef DEBUG
        //fprintf(stderr, "Current tag: 0x%2X, New tag: 0x%2X\n" , way->tag, tag );
        //fprintf(stderr, "Current MESI bits: %d\n", way->mesi);
        #endif
        // Hit if MESI bits are not "INVALID" and address tag == cached tag
        if (way->mesi != INVALID && way->tag == tag) {
            // Update PLRU to reflect MRU (Hit)
            UpdatePLRU(index[set_index]->plru, i);
            old_mesi_state = (index[set_index]->ways[i]->mesi);
            MESI_set( &(index[set_index]->ways[i]->mesi), operation, 1);
            strcpy(mesi_state, (index[set_index]->ways[i]->mesi == INVALID) ? "INVALID" : 
            ((index[set_index]->ways[i]->mesi == EXCLUSIVE) ? "EXCLUSIVE" : 
            ((index[set_index]->ways[i]->mesi == SHARED) ? "SHARED" : 
            ((index[set_index]->ways[i]->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));
            #ifdef DEBUG
                fprintf(stderr, "MESI result: %s\n", mesi_state);
            #endif
            return 1; //Hit
        }
        //If not a hit, and there is an invalid way, mark the invalid way to be "filled" upon miss
        else if (way->mesi == INVALID){
            InvalidWays = i;
            break;
        }
    }

    // Miss with invalid ways in the current set
    if(InvalidWays >= 0) {
        Way *invalid_way =index[set_index]->ways[InvalidWays]; // Navigate to way in active set with an INVALID MESI state
        invalid_way->tag = tag; // Place new tag at current way
        old_mesi_state = (index[set_index]->ways[InvalidWays]->mesi);
        MESI_set(&(index[set_index]->ways[InvalidWays]->mesi), operation, 0); // Update MESI State based off snoop results
        strcpy(mesi_state, (index[set_index]->ways[InvalidWays]->mesi == INVALID) ? "INVALID" : 
        ((index[set_index]->ways[InvalidWays]->mesi == EXCLUSIVE) ? "EXCLUSIVE" : 
        ((index[set_index]->ways[InvalidWays]->mesi == SHARED) ? "SHARED" : 
        ((index[set_index]->ways[InvalidWays]->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));
        #ifdef DEBUG
            fprintf(stderr, "MESI result: %s\n", mesi_state);
        #endif
        UpdatePLRU(index[set_index]->plru, InvalidWays); // Update PLRU for the newly entered way
        return 0; //Miss
    // Miss with a full set
    } else {
        int victim_line = VictimPLRU(index[set_index]->plru, *index[set_index]->ways); // Decide which way is a the LRU
        Way *victim_eviction = index[set_index]->ways[victim_line];
        old_address = (((index[set_index]->ways[victim_line]->tag) << (byte_select_bits + index_bits)) + (set_index << byte_select_bits));
        victim_eviction ->tag = tag; // Replace the victim ways tag with current tag
        old_mesi_state = (index[set_index]->ways[victim_line]->mesi);
        MESI_set(&(victim_eviction->mesi), operation, 0); // Update MESI State based off snoop results
        strcpy(mesi_state, (index[set_index]->ways[victim_line]->mesi == INVALID) ? "INVALID" :
        ((index[set_index]->ways[victim_line]->mesi == EXCLUSIVE) ? "EXCLUSIVE" :
        ((index[set_index]->ways[victim_line]->mesi == SHARED) ? "SHARED" :
        ((index[set_index]->ways[victim_line]->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));
        #ifdef DEBUG
            fprintf(stderr, "MESI result: %s\n", mesi_state);
        #endif
        UpdatePLRU(index[set_index]->plru, victim_line); // Update PLRU for the newly entered way
        return 2;
    }
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
    
    strcpy(snoop_state, (val == HIT ? "HIT" : (val == HITM ? "HITM" : "NOHIT")));
    
    return val;

}

// Return snoop result for snooped operations 
int SnoopChecker(Set *index[], int set_index, int tag) {
    
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        Way *way = index[set_index]->ways[i];
        // Check for a valid MESI state and matching tag
        if (way->mesi != INVALID && way->tag == tag) {
            if (way->mesi == MODIFIED) {
                MESI_set(&(index[set_index]->ways[i]->mesi), operation, 0);
                strcpy(mesi_state, (index[set_index]->ways[i]->mesi == INVALID) ? "INVALID" : 
                ((index[set_index]->ways[i]->mesi == EXCLUSIVE) ? "EXCLUSIVE" : 
                ((index[set_index]->ways[i]->mesi == SHARED) ? "SHARED" : 
                ((index[set_index]->ways[i]->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));
                #ifdef DEBUG
                fprintf(stderr, "Modified Line\n");
                #endif
                return HITM; // Hit in Modified state
            } else if (way->mesi == EXCLUSIVE || way->mesi == SHARED) {
                MESI_set(&(index[set_index]->ways[i]->mesi), operation, 0);
                strcpy(mesi_state, (index[set_index]->ways[i]->mesi == INVALID) ? "INVALID" :
                 ((index[set_index]->ways[i]->mesi == EXCLUSIVE) ? "EXCLUSIVE" :
                  ((index[set_index]->ways[i]->mesi == SHARED) ? "SHARED" :
                   ((index[set_index]->ways[i]->mesi == MODIFIED) ? "MODIFIED" : "NaN"))));   
                #ifdef DEBUG
                fprintf(stderr, "Exclusive or Shared Line\n");
                #endif
                return HIT; // Hit in Exclusive or Shared state
            }
        }
    }
    #ifdef DEBUG
    fprintf(stderr, "Invalid Line\n");
    #endif
    // No hit found
    strcpy(mesi_state, "INVALID");
    return NOHIT1;
}

// Return snoop result as a string for better print statements
void SendSnoopResult(int SnoopCheck, char* snoop_reply) {
    if (snoop_reply != NULL) {
        // Ensure the reply matches the snoop result
        strcpy(snoop_reply, 
               (SnoopCheck == HITM ? "HITM" : 
               (SnoopCheck == HIT ? "HIT" : "NOHIT")));
    }
    return;
}

// Determine MESI state updates based upon Snoop Results
void MESI_set(int* mesi, int operation, int hm){

    int snoop = GetSnoopResult(snoop_state);

    // Update MESI bits based on operation
    switch (operation){

        
        case READ_HD: // n = 0
        case READ_HI: // n = 2

            if(hm) break;

            if (snoop == HIT){
                *mesi = SHARED;
            }
            else if (snoop == HITM) {
                *mesi = SHARED;
            }
            else{
                *mesi = EXCLUSIVE;
            }

            break;
        case WRITE_HD: // n = 1
            *mesi = MODIFIED;
            break;

        case READ_S: // n = 3

            if (*mesi == EXCLUSIVE) {
                *mesi = SHARED;
            }
            else if (*mesi == MODIFIED) {
                *mesi = SHARED;
            }
            else if (*mesi == SHARED) {
                *mesi = SHARED;
            }
            else{
                *mesi = INVALID;
            }
            break;

        case RWIM_S: // n = 5
            *mesi = INVALID;
            break;

        case WRITE_S: // n = 4
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

// Empty cache contents and echo Inclusivity statements to L1 cache
void clear_cache (Set *index[], int sets, int plru_size, int assoc) {

    for (int i = 0; i < sets; i++){

        // Erase PLRU
        for (int j = 0; j   < plru_size; j++) {
            index[i]->plru[j] = 0;
        }

        // Erase tag and set MESI to INVALID
        for (int k = 0; k < assoc; k++) {
            if(index[i]->ways[k]->mesi != 0x0){
                address = (((index[i]->ways[k]->tag) << (byte_select_bits + index_bits)) + (i << byte_select_bits));
                if(index[i]->ways[k]->mesi == MODIFIED){
                    inclusive_print(EVICTLINE);
                    printf("L2: FlushWB @ 0x%08X\n", address);
                } else if ((index[i]->ways[k]->mesi == EXCLUSIVE)||(index[i]->ways[k]->mesi == SHARED)){
                    if (mode) printf("\n");
                    inclusive_print(INVALIDATELINE);
                }
            }
            index[i]->ways[k]->mesi = INVALID;
            index[i]->ways[k]->tag = 0;
        }
    }
    return;
}

// Print active/valid contents of cache
void print_cache (Set *index[], int sets, int plru_size, int assoc) {
    bool check = false;
    
    for (int i = 0; i < sets; i++){

        check = false;

        // Check for any valid ways in Set
        for (int a = 0; a < assoc; a++){
            if (index[i]->ways[a]->mesi != INVALID){
                check = true;
            }

        }

        // If there is at least one valid way, print PLRU bits
        if (check) {
            printf("Set %d PLRU: ", i);
            for (int j = 0; j < plru_size; j++){

                printf("%d", index[i]->plru[j]);
            }
        
        printf("\n");
        }
        // Print any valid Way 
        for (int k = 0; k < assoc; k++) {
            if (index[i]->ways[k]->mesi != INVALID) {
                
                printf("    Cache contents for Set %d way %02d: MESI STATE: %s, TAG: %04X\n", i, k, (index[i]->ways[k]->mesi == INVALID) ? "INVALID" : ((index[i]->ways[k]->mesi == EXCLUSIVE) ? "EXCLUSIVE" : ((index[i]->ways[k]->mesi == SHARED) ? "SHARED" : ((index[i]->ways[k]->mesi == MODIFIED) ? "MODIFIED" : "NaN"))), index[i]->ways[k]->tag );
            }
        }
    }
    return;
}

//This gives the overall statistics of the cache including the ratio after the program finishes
void cache_statistics(int operation, int CacheResult, bool finished_program){

    static int cache_reads = 0;
    static int cache_writes = 0;
    static int cache_hits = 0;
    static int cache_misses = 0;
    static float cache_hit_miss_ratio = 0;

    // Needs to print in silent and normal mode and if program is finished
    // Needs to stay up here since "operation" will remain and will count last one twice if not
    if (finished_program) {
        cache_hit_miss_ratio = (cache_hits + cache_misses > 0) ? (float)cache_hits / (cache_hits + cache_misses) : 0.0;
        printf("\n----------------------------------------Cache Results----------------------------------------\nOverall Reads: %d, Overall Writes: %d, Cache hits: %d, Cache misses: %d, Cache hit ratio: %.5f\n---------------------------------------------------------------------------------------------\n", 
            cache_reads, cache_writes, cache_hits, cache_misses, cache_hit_miss_ratio);
        return;
    }

    // Check to see if hit or miss
    if (CacheResult) 
        cache_hits++;
    else 
        cache_misses++;

    // Check to see if read bus operation or read to modify
    if (operation == READ_HD || operation == READ_HI) 
        cache_reads++;

    // Check to see if write bus operation
    if (operation == WRITE_HD) 
        cache_writes++;

    return;
}

// Send inclusivity statements depending on given command
void inclusive_print(int state){

    switch(state) {

        case GETLINE:
            if(mode) printf("L2: GETLINE 0x%08X\n", (address & ~(0x3F)));
            break;
        case SENDLINE:
            if(mode) printf("L2: SENDLINE 0x%08X\n", (address & ~(0x3F)));
            break;
        case INVALIDATELINE:
            if(mode) printf("L2: INVALIDATELINE 0x%08X\n", (address & ~(0x3F)));
            break;
        case EVICTLINE:
            if(mode) printf("L2: EVICTLINE 0x%08X\n", (address & ~(0x3F)));
            break;
        default:
            fprintf(stderr, "Invalid state in inclusive_print function\n");
            exit(-1);
    }

    return;
}
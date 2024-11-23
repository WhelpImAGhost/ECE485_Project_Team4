/*
 * ECE 485/585 Final Prroject
 * Team 4
 * 
 * Ameer Melli
 * Caleb Monti
 * Chris Kane-Pardy
 * Evan Brown
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

// Global variables
uint64_t counter = 0;


// Function Prototypes



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


    // Set up cache arrays
    
    typedef struct {
        int mesi;                   // One per way
        int tag;
    } Way;


    typedef struct {
        int plru[PLRU_ARRAY_SIZE]; // One per Set
        Way *ways[ASSOCIATIVITY];                  // One per set

    } Set;


    
    Set *index[SETS];
    // Allocate memory for index and its components


    for (int i = 0; i < SETS; i++) {


        
        index[i] = (Set *)malloc(sizeof(Set));
        if (index[i] == NULL) {
            perror("Failed to allocate memory for Set");
            exit(EXIT_FAILURE);
        }

        for (int l = 0; l < PLRU_ARRAY_SIZE; l++) {
            index[i]->plru[l] = 0;
        }

        
        for (int k = 0; k < ASSOCIATIVITY; k++) {

            index[i]->ways[k] = (Way *)malloc(sizeof(Way));
            if (index[i]->ways[k] == NULL) {
                perror("Failed to allocate memory for Way");
                exit(EXIT_FAILURE);
            }

            index[i]->ways[k]->mesi = INVALID;

            
                counter++;
                index[i]->ways[k]->tag = 0;
        }
        
    }

    #ifdef DEBUG
    fprintf(stderr, "Number of sets: %d\n", SETS);
    fprintf(stderr, "Total number of initialized Ways: %llu\n", counter);
    #endif

    for (int i = 0; i < ASSOCIATIVITY; i++){
        index[0]->ways[i]->tag = 15 - i;
        // index[0]->ways[i]->tag
        // index[0]->ways[i]->tag
    }

    for (int i = 0; i < ASSOCIATIVITY; i++){
        fprintf(stderr, "Tag val of way %d in set 0: %d\n", i, index[0]->ways[i]->tag);
    }

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

    // Read each line until end of file
    while (fscanf(file, "%d %x", &operation, &address) == 2) {
        #ifdef DEBUG
        fprintf(stderr, "Operation: %d, Address: 0x%X\n", operation, address);
        #endif
        // Process the values here if needed
    }

    fclose(file);  // Close the file
    return 0;
}

// Function declarations


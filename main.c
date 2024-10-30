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
*/


// Header file inclusion

#include <stdio.h>
#include <stdlib.h>

// Defines and Globals



// Function prototypes



int main(int argc, char *argv[]) {
    // Set default filename
    const char *filename = "Default.din";

    // Check if user provided a filename as an argument
    if (argc > 1) {
        filename = argv[1];
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    printf("Using file: %s\n", filename);

    int operation;
    unsigned int address;

    // Read each line until end of file
    while (fscanf(file, "%d %x", &operation, &address) == 2) {
        #ifdef DEBUG
        printf("Operation: %d, Address: 0x%X\n", operation, address);
        #endif
        // Process the values here if needed

    }

    fclose(file);  // Close the file
    return 0;
}
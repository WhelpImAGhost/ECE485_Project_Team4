/*
 * ECE 485/585 Final Prroject
 * Team 4
 * 
 * Ameer Melli
 * Caleb Monti
 * Chris Kane-Pardy
 * Evan Brown
 * 

*/


// Header file inclusion

#include <stdio.h>
#include <stdlib.h>

// Defines and Globals



// Function prototypes



int main() {
    FILE *file = fopen("rwims.din", "r");  // Open the file in read mode
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

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
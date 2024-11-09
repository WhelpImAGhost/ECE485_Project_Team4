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

int main(int argc, char *argv[]) {
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

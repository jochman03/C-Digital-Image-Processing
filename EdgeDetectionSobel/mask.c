#include <stdio.h>
#include <stdlib.h>
#include "mask.h"

// Print error message and terminate program on memory allocation failure
_Noreturn static void allocationFailure() {
    fprintf(stderr, "There is not enough memory available.\n");
    exit(EXIT_FAILURE);
}

mask* maskCreate(unsigned int rows, unsigned int cols) {
    // Allocate memory for mask structure
    mask* m = malloc(sizeof(mask));
    if (!m) {
        allocationFailure();
    }

    // Allocate zero-initialized memory for mask elements
    m->data = calloc(rows * cols, sizeof(float));
    if (!m->data) {
        free(m);
        allocationFailure();
    }

    // Store dimensions
    m->rows = rows;
    m->cols = cols;

    // Return pointer to created mask
    return m;
}

void maskFree(mask* m) {
    if (m) {
        if(m->data){
            // Free mask data array
            free(m->data);
        }
        // Free mask structure itself
        free(m);
    }
}
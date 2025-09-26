#ifndef MASK_H
#define MASK_H

/**
 * @brief Structure representing a convolution mask (kernel).
 */
typedef struct {
    unsigned int rows;   /// number of rows in the mask
    unsigned int cols;   /// number of columns in the mask
    float *data;         /// pointer to mask data stored in row-major order
} mask;

/**
 * @brief Allocate and initialize a new mask with given dimensions.
 *
 * @param rows Number of rows.
 * @param cols Number of columns.
 * @return Pointer to the newly created mask, or NULL if allocation fails.
 */
mask* maskCreate(unsigned int rows, unsigned int cols);

/**
 * @brief Free the memory associated with a mask.
 *
 * @param m Pointer to the mask to be freed.
 */
void maskFree(mask* m);

#endif // MASK_H
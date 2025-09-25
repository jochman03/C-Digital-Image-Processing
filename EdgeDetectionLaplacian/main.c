#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "mask.h"

// Define BMP header size
#define BMP_HEADER_SIZE 54
// Size of color table for 8-bit BMP
#define BMP_COLOR_TABLE_SIZE 1024
// Maximum value of a pixel
#define MAX_BRIGHTNESS 255
// Minimum value of a pixel
#define MIN_BRIGHTNESS 0

// Macro definitions for minimum and maximum values
#define MIN(a, b) ((a) < (b) ? (a) : (b))   // returns the smaller of (a) and (b)
#define MAX(a, b) ((a) > (b) ? (a) : (b))   // returns the larger of (a) and (b)

// Structure to hold 8-bit BMP image data
typedef struct {
    unsigned char header[BMP_HEADER_SIZE];          // BMP file header (54 bytes)
    unsigned char colorTable[BMP_COLOR_TABLE_SIZE]; // Color table for 8-bit BMP (256 * 4 bytes)
    unsigned char *data;                            // Pointer to pixel data
    int width;                                      // Image width in pixels
    int height;                                     // Image height in pixels
    int bitDepth;                                   // Bits per pixel (8 for grayscale)
    int imgSize;                                    // Total size of pixel data in bytes
} BMP8Image;

// Function to read an 8-bit BMP image from a file
BMP8Image* BMP8read(const char* filename) {
    // open file
    FILE *fInput = fopen(filename, "rb");
    if (!fInput) {
        fprintf(stderr, "Unable to open file %s!\n", filename);
        return NULL;
    }

    // allocate struct
    BMP8Image *img = (BMP8Image*)malloc(sizeof(BMP8Image));
    if (!img) {
        fprintf(stderr, "Memory allocation failed!\n");
        fclose(fInput);
        return NULL;
    }

    // Read header (54 bytes)
    fread(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, fInput);

    // Extract metadata from header
    img->width = *(int*)&img->header[18];
    img->height = *(int*)&img->header[22];
    img->bitDepth = *(short*)&img->header[28];

    // Compute padded row size and image size
    int rowSize = (img->width + 3) & ~3;
    img->imgSize = rowSize * img->height;

    // Read color table (only for 8-bit BMP)
    if (img->bitDepth <= 8) {
        fread(img->colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, fInput);
    }

    // Allocate memory for pixel data
    img->data = (unsigned char*)malloc(img->imgSize);
    if (!img->data) {
        fprintf(stderr, "Memory allocation failed!\n");
        free(img);
        fclose(fInput);
        return NULL;
    }

    // Read pixel data
    fread(img->data, sizeof(unsigned char), img->imgSize, fInput);

    fclose(fInput);
    return img;
}

// Function to apply convolution with a given mask to an 8-bit BMP image
BMP8Image* BMP8Convolution(BMP8Image* img, mask* m){
    if(!img || !m){
        fprintf(stderr, "Convolution Error: Either there is no image or mask.\n");
        return NULL;
    }

    // Allocate memory for new image
    BMP8Image* convImg = malloc(sizeof(BMP8Image));
    if(!convImg){
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    // Copy BMP header and metadata
    memcpy(convImg->header, img->header, BMP_HEADER_SIZE);
    convImg->width = img->width;
    convImg->height = img->height;
    convImg->bitDepth = img->bitDepth;

    // Copy color table if grayscale (8-bit)
    if (img->bitDepth <= 8) {
        memcpy(convImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    // Compute padded row size and total image size
    int rowSize = (img->width + 3) & ~3;
    convImg->imgSize = rowSize * img->height;

    // Allocate memory for pixel data of the result
    convImg->data = malloc(convImg->imgSize);
    if (!convImg->data) {
        fprintf(stderr, "Memory allocation failed for pixel data!\n");
        free(convImg);
        return NULL;
    }

    // Compute center of the mask (for correct alignment during convolution)
    int iCenter = m->rows / 2;
    int jCenter = m->cols / 2;

    // Iterate through every pixel in the image
    for(int y = 0; y < convImg->height; y++){
        for(int x = 0; x < convImg->width; x++){
            float val = 0.0f;

            // Apply convolution mask
            for(int i = 0; i < m->rows; i++){
                for(int j = 0; j < m->cols; j++){
                    int idx = x + (j - jCenter);  // pixel x offset
                    int idy = y + (i - iCenter);  // pixel y offset

                    // Check if neighbor is inside image bounds
                    if(idx >= 0 && idx < img->width && idy >= 0 && idy < img->height){
                        float ms = m->data[i * m->cols + j];       // mask coefficient
                        float im = img->data[idy * rowSize + idx]; // image pixel value
                        val += ms * im;
                    }
                }
            }

            // Clamp to valid grayscale range [0..255]
            val = MIN(val, MAX_BRIGHTNESS);
            val = MAX(val, MIN_BRIGHTNESS);

            // Store result pixel
            convImg->data[y * convImg->width + x] = (unsigned char)val;
        }
    }

    return convImg;
}

BMP8Image* BMP8EdgeDetectionLaplacianNegative(BMP8Image* img){
    if(!img){
        fprintf(stderr, "Edge Detection Error: No image provided.\n");
        return NULL;
    }

    // Create a 3x3 convolution mask
    mask* m = maskCreate(3, 3);
    if(!m || !m->data){
        fprintf(stderr, "Error: Could not allocate mask.\n");
        return NULL;
    }

    // Laplacian kernel (negative version)
    // Detects edges with center = +4, neighbors = -1
    int maskL[3][3] = {
        { 0, -1,  0},
        {-1,  4, -1},
        { 0, -1,  0}
    };

    // Copy kernel values into the mask struct
    for(int j = 0; j < m->cols; j++){
        for(int i = 0; i < m->rows; i++){
            m->data[i * m->cols + j] = maskL[i][j];
        }
    }

    // Apply convolution to the image using the Laplacian mask
    BMP8Image *convolved = BMP8Convolution(img, m);

    // Free mask memory
    maskFree(m);

    // Return the result image with detected edges
    return convolved;
}

BMP8Image* BMP8EdgeDetectionLaplacianPositive(BMP8Image* img){
    if(!img){
        fprintf(stderr, "Edge Detection Error: No image provided.\n");
        return NULL;
    }

    // Create a 3x3 convolution mask
    mask* m = maskCreate(3, 3);
    if(!m || !m->data){
        fprintf(stderr, "Error: Could not allocate mask.\n");
        return NULL;
    }

    // Laplacian kernel (positive version)
    // Detects edges with center = -4, neighbors = +1
    int LaplacianV[3][3] = {
        { 0,  1,  0},
        { 1, -4,  1},
        { 0,  1,  0}
    };

    // Copy kernel values into the mask struct
    for(int j = 0; j < m->cols; j++){
        for(int i = 0; i < m->rows; i++){
            m->data[i * m->cols + j] = LaplacianV[i][j];
        }
    }

    // Apply convolution to the image using the Laplacian mask
    BMP8Image *convolved = BMP8Convolution(img, m);

    // Free mask memory
    maskFree(m);

    // Return the result image with detected edges
    return convolved;
}


// Free memory used by BMP8Image
void BMP8Free(BMP8Image* img) {
    if (img) {
        free(img->data);
        free(img);
    }
}

// Function to save an 8-bit BMP image to a file
void BMP8save(const char* filename, BMP8Image* img) {
    FILE *fOutput = fopen(filename, "wb");
    if (!fOutput) {
        fprintf(stderr, "Unable to create file %s!\n", filename);
        return;
    }

    fwrite(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, fOutput);

    if (img->bitDepth <= 8) {
        fwrite(img->colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, fOutput);
    }

    fwrite(img->data, sizeof(unsigned char), img->imgSize, fOutput);

    fclose(fOutput);
}

int main(){
    // Input BMP file (8-bit grayscale)
    const char *inputFile  = "../Test_Images/lizard_greyscale8bit.bmp";

    // Output BMP file paths
    const char *outputNegative = "images/lizard_laplacian_negative.bmp";
    const char *outputPositive = "images/lizard_laplacian_positive.bmp";

    // Step 1: Read the original image
    BMP8Image *image = BMP8read(inputFile);
    if(!image){
        fprintf(stderr, "Error: Could not read input BMP file.\n");
        return 1;
    }

    // Step 2: Apply Laplacian Negative edge detection
    BMP8Image *negative = BMP8EdgeDetectionLaplacianNegative(image);
    BMP8save(outputNegative, negative);

    // Step 3: Apply Laplacian Positive edge detection
    BMP8Image *positive = BMP8EdgeDetectionLaplacianPositive(image);
    BMP8save(outputPositive, positive);

    // Step 5: Free all allocated memory
    BMP8Free(image);
    BMP8Free(negative);
    BMP8Free(positive);

    // Step 6: Print completion message
    printf("Laplacian edge detection completed!\n");
    printf("Negative: %s\nPositive: %s\n",
           outputNegative, outputPositive);

    return 0;
}
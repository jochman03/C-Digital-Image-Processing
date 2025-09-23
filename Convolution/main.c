#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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
                        float im = img->data[idy * img->width + idx]; // image pixel value
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


// Free memory used by BMP8Image
void BMP8Free(BMP8Image* img) {
    if (img) {
        free(img->data);
        free(img);
    }
}


int main(){
    // Input and output file paths
    const char *inputFile = "../Test_Images/lizard_greyscale8bit.bmp";
    const char *outputFile = "images/lizard_convolved.bmp";

    // Step 1: Read the original BMP image
    BMP8Image *image = BMP8read(inputFile);
    if(!image){
        fprintf(stderr, "Error: Could not read input BMP file.\n");
        return 1;
    }

    // Step 2: Create a 5x5 convolution mask
    // Using a Laplacian-like kernel to enhance edges
    mask* m = maskCreate(5, 5);
    if(!m || !m->data){
        fprintf(stderr, "Error: Could not allocate mask.\n");
        BMP8Free(image);
        return 1;
    }

    // Step 3: Fill the mask with values
    // Center pixel gets large positive weight,
    // all neighbors get -1 (detects intensity changes)
    for(int j = 0; j < m->cols; j++){
        for(int i = 0; i < m->rows; i++){
            if((j == (int)m->cols / 2) && (i == (int)m->rows / 2)){
                // strong positive weight
                m->data[i * m->cols + j] = m->cols * m->rows - 1; 
            }
            else{
                // negative weight for neighbors
                m->data[i * m->cols + j] = -1;
            }
        }
    }

    // Step 4: Apply convolution to the image
    BMP8Image *convolved = BMP8Convolution(image, m);

    // Step 5: Save the convolved image
    BMP8save(outputFile, convolved);

    // Step 6: Free all allocated memory
    BMP8Free(image);
    BMP8Free(convolved);
    maskFree(m);

    printf("Convolution completed! Saved result as %s\n", outputFile);
    return 0;
}
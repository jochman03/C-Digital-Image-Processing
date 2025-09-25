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

BMP8Image* BMP8EdgeDetectionPrewittHorizontal(BMP8Image* img){
    if(!img){
        fprintf(stderr, "Edge Detection Error: No image provided.\n");
        return NULL;
    }

    // Create a 3x3 mask for convolution
    mask* m = maskCreate(3, 3);
    if(!m || !m->data){
        fprintf(stderr, "Error: Could not allocate mask.\n");
        return NULL;
    }

    // Fill mask with Prewitt horizontal values
    // Loop over all mask rows and columns
    for(int j = 0; j < m->cols; j++){
        for(int i = 0; i < m->rows; i++){
            if(i == 0) m->data[i * m->cols + j] = -1; // top row: negative weight
            else if(i == 1) m->data[i * m->cols + j] = 0; // middle row: no weight
            else m->data[i * m->cols + j] = 1; // bottom row: positive weight
        }
    }

    // Apply convolution with the horizontal mask
    BMP8Image *convolved = BMP8Convolution(img, m);

    // Free mask memory
    maskFree(m);

    // Return the convolved image containing horizontal edges
    return convolved;
}

BMP8Image* BMP8EdgeDetectionPrewittVertical(BMP8Image* img){
    if(!img){
        fprintf(stderr, "Edge Detection Error: No image provided.\n");
        return NULL;
    }

    // Create a 3x3 mask for convolution
    mask* m = maskCreate(3, 3);
    if(!m || !m->data){
        fprintf(stderr, "Error: Could not allocate mask.\n");
        return NULL;
    }

    // Fill mask with Prewitt vertical values
    for(int j = 0; j < m->cols; j++){
        for(int i = 0; i < m->rows; i++){
            if(j == 0) m->data[i * m->cols + j] = -1; // left column: negative weight
            else if(j == 1) m->data[i * m->cols + j] = 0; // middle column: no weight
            else m->data[i * m->cols + j] = 1; // right column: positive weight
        }
    }

    // Apply convolution with the vertical mask
    BMP8Image *convolved = BMP8Convolution(img, m);

    // Free mask memory
    free(m);

    // Return the convolved image containing vertical edges
    return convolved;
}

// Free memory used by BMP8Image
void BMP8Free(BMP8Image* img) {
    if (img) {
        free(img->data);
        free(img);
    }
}

BMP8Image* BMP8EdgeDetectionPrewittCombined(BMP8Image* img){
    if(!img){
        fprintf(stderr, "Edge Detection Error: No image provided.\n");
        return NULL;
    }

    // Calculate horizontal edges
    BMP8Image* horizontal = BMP8EdgeDetectionPrewittHorizontal(img);
    // Calculate vertical edges
    BMP8Image* vertical   = BMP8EdgeDetectionPrewittVertical(img);

    // Allocate new image for combined edges
    BMP8Image* edgeImg = malloc(sizeof(BMP8Image));
    memcpy(edgeImg->header, img->header, BMP_HEADER_SIZE);
    edgeImg->width = img->width;
    edgeImg->height = img->height;
    edgeImg->bitDepth = img->bitDepth;

    // Copy color table for 8-bit grayscale images
    if (img->bitDepth <= 8){
        memcpy(edgeImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    int rowSize = (img->width + 3) & ~3;
    edgeImg->imgSize = rowSize * img->height;

    // Allocate memory for pixel data
    edgeImg->data = malloc(edgeImg->imgSize);

    // Combine horizontal and vertical edges
    for(int y = 0; y < img->height; y++){
        for(int x = 0; x < img->width; x++){
            int idx = y * img->width + x;
            // horizontal
            int gx = horizontal->data[idx];
            // vertical
            int gy = vertical->data[idx];
            // Compute mean magnitude
            int g = (int)(sqrt(gx*gx + gy*gy));

            edgeImg->data[idx] = (unsigned char)g;
        }
    }

    // Free temporary horizontal and vertical images
    BMP8Free(horizontal);
    BMP8Free(vertical);

    // Return combined edge image
    return edgeImg;
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
    const char *inputFile  = "../Test_Images/lizard_greyscale8bit.bmp";

    // Output files
    const char *outputHorizontal = "images/lizard_edges_horizontal.bmp";
    const char *outputVertical   = "images/lizard_edges_vertical.bmp";
    const char *outputCombined   = "images/lizard_edges_combined.bmp";

    // Read the original BMP image
    BMP8Image *image = BMP8read(inputFile);
    if(!image){
        fprintf(stderr, "Error: Could not read input BMP file.\n");
        return 1;
    }

    // Apply horizontal edge detection
    BMP8Image *horizontal = BMP8EdgeDetectionPrewittHorizontal(image);
    BMP8save(outputHorizontal, horizontal);

    // Apply vertical edge detection
    BMP8Image *vertical = BMP8EdgeDetectionPrewittVertical(image);
    BMP8save(outputVertical, vertical);

    // Apply combined edge detection
    BMP8Image *combined = BMP8EdgeDetectionPrewittCombined(image);
    BMP8save(outputCombined, combined);

    // Free all allocated memory
    BMP8Free(image);
    BMP8Free(horizontal);
    BMP8Free(vertical);
    BMP8Free(combined);

    printf("Prewitt edge detection completed!\nHorizontal: %s\nVertical: %s\nCombined: %s\n",
           outputHorizontal, outputVertical, outputCombined);

    return 0;
}
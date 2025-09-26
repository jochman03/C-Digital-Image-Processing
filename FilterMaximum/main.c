#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Define BMP header size
#define BMP_HEADER_SIZE 54
// Size of color table for 8-bit BMP
#define BMP_COLOR_TABLE_SIZE 1024
// Maximum value of a pixel
#define MAX_BRIGHTNESS 255
// Minimum value of a pixel
#define MIN_BRIGHTNESS 0


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

BMP8Image* BMP8FilterMaximum(BMP8Image* img, int kernelSize){
    // Allocate memory for the new image
    BMP8Image* filteredImg = malloc(sizeof(BMP8Image));
    if(!filteredImg){
        fprintf(stderr, "Filter Error: Memory allocation failed!\n");
        return NULL;
    }

    // Copy image metadata
    memcpy(filteredImg->header, img->header, BMP_HEADER_SIZE);
    filteredImg->width = img->width;
    filteredImg->height = img->height;
    filteredImg->bitDepth = img->bitDepth;

    // Copy color table if 8-bit grayscale
    if (img->bitDepth <= 8) {
        memcpy(filteredImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    // Compute padded row size
    int rowSize = (img->width + 3) & ~3;
    filteredImg->imgSize = rowSize * img->height;

    // Allocate memory for pixel data
    filteredImg->data = malloc(filteredImg->imgSize);
    if (!filteredImg->data) {
        fprintf(stderr, "Filter Error: Memory allocation failed for pixel data!\n");
        free(filteredImg);
        return NULL;
    }
    // Copy original image data
    memcpy(filteredImg->data, img->data, img->imgSize);

    // Apply maximum filter
    for(int j = kernelSize/2; j < (filteredImg->height - kernelSize/2); j++){
        for(int i = kernelSize/2; i < (filteredImg->width - kernelSize/2); i++){
            unsigned char maxValue = MIN_BRIGHTNESS;
            for(int y = -kernelSize/2; y < kernelSize/2; y++){
                for(int x = -kernelSize/2; x < kernelSize/2; x++){
                    unsigned char value = img->data[(j + y) * rowSize + i + x];
                    if (maxValue < value){
                        maxValue = value;
                    }
                }
            }

            filteredImg->data[j * rowSize + i] = maxValue;
        }
    }

    return filteredImg;
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


int main() {
    const char* inputFile = "../Test_Images/lizard_greyscale8bit.bmp";
    const char* outputFile = "images/lizard_filtered_max_3.bmp";

    // Read the input BMP image
    BMP8Image *image = BMP8read(inputFile);
    if (!image) {
        return 1;
    }

    // Apply maximum filter with a 3x3 kernel
    BMP8Image *filtered = BMP8FilterMaximum(image, 3);
    if (!filtered) {
        BMP8Free(image);
        return 1;
    }

    // Save the filtered image to file
    BMP8save(outputFile, filtered);

    // Free allocated memory
    BMP8Free(image);
    BMP8Free(filtered);

    return 0;
}
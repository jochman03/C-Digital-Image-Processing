#include <stdio.h>
#include <stdlib.h>

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

// Define a structure to hold 8-bit BMP image data
typedef struct {
    unsigned char header[BMP_HEADER_SIZE];          // BMP file header
    unsigned char colorTable[BMP_COLOR_TABLE_SIZE]; // Color table for 8-bit BMP
    unsigned char *data;                            // Pointer to pixel data
    int width;                                      // Image width
    int height;                                     // Image height
    int bitDepth;                                   // Bits per pixel
    int imgSize;                                    // Total number of pixels
} BMP8Image; // Structure holding 8-bit BMP image data


// Function to read an 8-bit BMP image from a file
BMP8Image* BMP8read(const char* filename) {
    // Open BMP file in binary read mode
    FILE *fInput = fopen(filename, "rb");
    if (!fInput) {
        fprintf(stderr, "Unable to open file %s!\n", filename);
        return NULL;
    }

    // Allocate memory for BMP8Image structure
    BMP8Image *img = (BMP8Image*)malloc(sizeof(BMP8Image));
    if (!img) {
        fprintf(stderr, "Memory allocation failed!\n");
        fclose(fInput);
        return NULL;
    }

    // Read BMP header
    fread(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, fInput);

    // Extract image width, height, bit depth, and calculate image size
    img->width = *(int*)&img->header[18];
    img->height = *(int*)&img->header[22];
    img->bitDepth = *(short*)&img->header[28];

    // padded row size
    int rowSize = (img->width + 3) & ~3;
    img->imgSize = rowSize * img->height;

    // Read color table if BMP is 8-bit or less
    if (img->bitDepth <= 8) {
        fread(img->colorTable, sizeof(unsigned char), 1024, fInput);
    }

    // Allocate memory for image pixel data
    img->data = (unsigned char*)malloc(img->imgSize);
    if (!img->data) {
        fprintf(stderr, "Memory allocation failed!\n");
        free(img);
        fclose(fInput);
        return NULL;
    }

    // Read pixel data
    fread(img->data, sizeof(unsigned char), img->imgSize, fInput);
    // Close input file
    fclose(fInput);

    return img;
}

// Function to increase brightness of an 8-bit BMP image
void BMP8IncreaseBrightness(BMP8Image* img, int brightnessFactor) {
    // Each row of BMP pixel data is padded to a multiple of 4 bytes
    int rowSize = (img->width + 3) & ~3;

    // Loop through all image rows
    for (int y = 0; y < img->height; y++) {
        // Loop through all pixels in a row (ignore padding bytes)
        for (int x = 0; x < img->width; x++) {
            // Compute linear index for pixel (row offset + column)
            int idx = y * rowSize + x;

            // Increase brightness and clamp to maximum allowed value (255)
            img->data[idx] = MIN(img->data[idx] + brightnessFactor, MAX_BRIGHTNESS);
        }
    }
}

// Function to decrease brightness of an 8-bit BMP image
void BMP8DecreaseBrightness(BMP8Image* img, int brightnessFactor) {
    // Each row of BMP pixel data is padded to a multiple of 4 bytes
    int rowSize = (img->width + 3) & ~3;

    // Loop through all image rows
    for (int y = 0; y < img->height; y++) {
        // Loop through all pixels in a row (ignore padding bytes)
        for (int x = 0; x < img->width; x++) {
            // Compute linear index for pixel (row offset + column)
            int idx = y * rowSize + x;

            // Decrease brightness and clamp to minimum allowed value (0)
            img->data[idx] = MAX(img->data[idx] - brightnessFactor, MIN_BRIGHTNESS);
        }
    }
}


// Function to save an 8-bit BMP image to a file
void BMP8save(const char* filename, BMP8Image* img) {
    // Open file in binary write mode
    FILE *fOutput = fopen(filename, "wb");
    if (!fOutput) {
        fprintf(stderr, "Unable to create file %s!\n", filename);
        return;
    }

    // Write BMP header
    fwrite(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, fOutput);

    // Write color table if BMP is 8-bit or less
    if (img->bitDepth <= 8) {
        fwrite(img->colorTable, sizeof(unsigned char), 1024, fOutput);
    }

    // Write pixel data
    fwrite(img->data, sizeof(unsigned char), img->imgSize, fOutput);
    // Close output file
    fclose(fOutput);
}

// Function to free memory used by BMP8Image structure
void BMP8Free(BMP8Image* img) {
    if (img) {
        // Free pixel data
        free(img->data);
        // Free structure itself
        free(img);
    }
}

int main(){
    // Input BMP file path
    const char *inputFile = "../Test_Images/lizard_greyscale8bit.bmp";
    // Brightness to increase
    int brightnessIncreaseFactor = 100;
    // Read BMP image
    BMP8Image *image = BMP8read(inputFile);
    if (!image) exit(1);
    // Apply brightening
    BMP8IncreaseBrightness(image, brightnessIncreaseFactor);
    // Prepare output file name
    char outFilename[100];
    sprintf(outFilename, "images/lizard_brightnessInc%d.bmp", brightnessIncreaseFactor);
    // Save brightened image
    BMP8save(outFilename, image);
    // Free allocated memory
    BMP8Free(image);
    // Print confirmation message
    fprintf(stdout, "Created %s with brightness increased by %d\n", outFilename, brightnessIncreaseFactor);



    // Read BMP image
    image = BMP8read(inputFile);
    if (!image) exit(1);
    // Brightness to decrease
    int brightnessDecreaseFactor = 100;
    // Apply darkening
    BMP8DecreaseBrightness(image, brightnessDecreaseFactor);
    // Prepare output file name
    sprintf(outFilename, "images/lizard_brightnessDec%d.bmp", brightnessDecreaseFactor);
    // Save darkened image
    BMP8save(outFilename, image);
    // Free allocated memory
    BMP8Free(image);

    
    return 0;
}

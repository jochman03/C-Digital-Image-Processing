#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
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

// Add Gaussian noise to an 8-bit BMP image
BMP8Image* BMP8NoiseGaussian(BMP8Image* img, float mean, float var){
    if(!img){
        fprintf(stderr, "Noise Error: Image does not exists.\n");
        return NULL;
    }

    // Allocate memory for the new image
    BMP8Image* noisedImg = malloc(sizeof(BMP8Image));
    if(!noisedImg){
        fprintf(stderr, "Noise Error: Memory allocation failed!\n");
        return NULL;
    }

    // Copy image metadata
    memcpy(noisedImg->header, img->header, BMP_HEADER_SIZE);
    noisedImg->width = img->width;
    noisedImg->height = img->height;
    noisedImg->bitDepth = img->bitDepth;

    // Copy color table if 8-bit grayscale
    if (img->bitDepth <= 8) {
        memcpy(noisedImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    // Compute padded row size
    int rowSize = (img->width + 3) & ~3;
    noisedImg->imgSize = rowSize * img->height;

    // Allocate memory for pixel data
    noisedImg->data = malloc(noisedImg->imgSize);
    if (!noisedImg->data) {
        fprintf(stderr, "Noise Error: Memory allocation failed for pixel data!\n");
        free(noisedImg);
        return NULL;
    }

    // Iterate through every pixel
    for(int j = 0; j < noisedImg->height; j++){
        for(int i = 0; i < noisedImg->width; i++){
            // Generate two uniform random numbers in (0,1]
            float u1 = ((float)rand() + 1) / ((float)RAND_MAX + 1);
            float u2 = ((float)rand() + 1) / ((float)RAND_MAX + 1);

            // Box-Muller transform to get standard normal variable
            float z0 = sqrt(-2.0f * log(u1)) * cos(2.0f * M_PI * u2);

            // Scale by variance and shift by mean
            float noise = mean + sqrt(var) * z0;

            // Add noise to the original pixel
            float px = (float)img->data[j * rowSize + i] + noise;

            // Clamp to valid range [0, 255]
            px = MAX(MIN_BRIGHTNESS, px);
            px = MIN(MAX_BRIGHTNESS, px);

            // Store the result
            noisedImg->data[j * rowSize + i] = (unsigned char)px;
        }
    }

    return noisedImg;
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
    const char* inputFile = "../Test_Images/lizard_greyscale8bit.bmp";
    const char* outputFile = "images/lizard_gaussian_100.bmp";
    srand(time(NULL));

    // Read original BMP image
    BMP8Image *image = BMP8read(inputFile);
    if(!image){
        return 1;
    }
    BMP8Image *noised = BMP8NoiseGaussian(image, 0.0, 100);
    if(!noised){
        BMP8Free(image);
        return 1;
    }
    BMP8save(outputFile, noised);

    BMP8Free(image);
    BMP8Free(noised);

    return 0;
}

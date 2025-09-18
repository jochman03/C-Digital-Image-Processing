#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Define BMP header size
#define BMP_HEADER_SIZE 54
// Size of color table for 8-bit BMP
#define BMP_COLOR_TABLE_SIZE 1024

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

BMP8Image* BMP8Negative(BMP8Image* img) {
    if (!img) return NULL;

    // Allocate memory for negative image
    BMP8Image* negativeImg = malloc(sizeof(BMP8Image));
    if (!negativeImg) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    // Copy metadata
    memcpy(negativeImg->header, img->header, BMP_HEADER_SIZE);
    negativeImg->width = img->width;
    negativeImg->height = img->height;
    negativeImg->bitDepth = img->bitDepth;

    // Copy color table if 8-bit
    if (img->bitDepth <= 8) {
        memcpy(negativeImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    // Compute padded row size
    int rowSize = (img->width + 3) & ~3;
    negativeImg->imgSize = rowSize * img->height;

    // Allocate memory for pixel data
    negativeImg->data = malloc(negativeImg->imgSize);
    if (!negativeImg->data) {
        fprintf(stderr, "Memory allocation failed for pixel data!\n");
        free(negativeImg);
        return NULL;
    }

    // Compute negative: invert each pixel
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int idx = y * rowSize + x;
            negativeImg->data[idx] = 255 - img->data[idx];
        }
    }

    return negativeImg;
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
    // Input BMP file path (8-bit grayscale)
    const char *inputFile = "../Test_Images/lizard_greyscale8bit.bmp";

    // Read original BMP image
    BMP8Image *image = BMP8read(inputFile);
    if (!image) {
        fprintf(stderr, "Failed to read BMP file!\n");
        return 1;
    }

    // Create negative image
    BMP8Image *negativeImage = BMP8Negative(image);
    if (!negativeImage) {
        fprintf(stderr, "Failed to create negative image!\n");
        BMP8Free(image);
        return 1;
    }

    // Save negative image to file
    BMP8save("images/lizard_negative.bmp", negativeImage);

    // Free allocated memory
    BMP8Free(image);
    BMP8Free(negativeImage);

    // Confirmation message
    printf("Negative image saved as 'images/lizard_negative.bmp'.\n");

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define BMP header size
#define BMP_HEADER_SIZE 54
// Size of color table for 8-bit BMP
#define BMP_COLOR_TABLE_SIZE 1024

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

typedef enum {
    CLOCKWISE, COUNTER_CLOCKWISE, ROTATE_180
} rotation;

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

BMP8Image* BMP8Rotate(BMP8Image* img, rotation r) {
    if (!img) {
        // Check if input image is valid
        fprintf(stderr, "Image does not exists.\n");
        return NULL; 
    }

    // Allocate memory for the rotated image structure
    BMP8Image* rotatedImg = malloc(sizeof(BMP8Image));
    if (!rotatedImg) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    // Copy bit depth and color table (if 8-bit)
    rotatedImg->bitDepth = img->bitDepth;
    if (img->bitDepth <= 8) {
        memcpy(rotatedImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    int newWidth, newHeight;

    // Determine new image dimensions based on rotation type
    switch (r) {
        case CLOCKWISE:
        case COUNTER_CLOCKWISE:
            // Width becomes original height
            newWidth = img->height;
            // Height becomes original width
            newHeight = img->width;
            break;
        case ROTATE_180:
            // Width stays the same
            newWidth = img->width;
            // Height stays the same
            newHeight = img->height;
            break;
        default:
            free(rotatedImg);
            return NULL;
    }

    // Copy header and update width and height fields
    memcpy(rotatedImg->header, img->header, BMP_HEADER_SIZE);
    *(int*)&rotatedImg->header[18] = newWidth;
    *(int*)&rotatedImg->header[22] = newHeight;

    rotatedImg->width = newWidth;
    rotatedImg->height = newHeight;

    // Calculate padded row sizes
    // Input row aligned to 4 bytes
    int rowSizeIn = (img->width + 3) & ~3;
    // Output row aligned to 4 bytes
    int rowSizeOut = (newWidth + 3) & ~3;
    rotatedImg->imgSize = rowSizeOut * newHeight;

    // Allocate memory for pixel data of rotated image
    rotatedImg->data = calloc(rotatedImg->imgSize, 1);
    if (!rotatedImg->data) {
        fprintf(stderr, "Memory allocation failed for pixel data!\n");
        free(rotatedImg);
        return NULL;
    }

    // Pointer to original pixels
    unsigned char* inData = img->data;
    // Pointer to rotated pixels
    unsigned char* outData = rotatedImg->data;

    // Loop through all pixels of the original image
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            // Read pixel
            unsigned char pixel = inData[y * rowSizeIn + x];

            // Write pixel to new location based on rotation type
            switch (r) {
                case CLOCKWISE:
                    outData[x * rowSizeOut + (newWidth - y - 1)] = pixel;
                    break;
                case COUNTER_CLOCKWISE:
                    outData[(newHeight - x - 1) * rowSizeOut + y] = pixel;
                    break;
                case ROTATE_180:
                    outData[(newHeight - y - 1) * rowSizeOut + (newWidth - x - 1)] = pixel;
                    break;
                default:
                    break;
            }
        }
    }

    return rotatedImg; // Return pointer to rotated image
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

int main() {
    // Input BMP file path (8-bit grayscale)
    const char *inputFile = "../Test_Images/lizard_greyscale8bit.bmp";

    // Output BMP file path
    const char *outputFile = "images/lizard_rotated_cw.bmp";

    // Read BMP image
    BMP8Image *image = BMP8read(inputFile);
    if (!image) {
        fprintf(stderr, "Failed to read BMP file!\n");
        return 1;
    }

    // Choose rotation type (CLOCKWISE, COUNTER_CLOCKWISE, FLIP)
    rotation r = CLOCKWISE;

    // Rotate the image
    BMP8Image *rotated = BMP8Rotate(image, r);
    if (!rotated) {
        fprintf(stderr, "Failed to rotate image!\n");
        BMP8Free(image);
        return 1;
    }

    // Save the rotated image to a file
    BMP8save(outputFile, rotated);

    // Free memory for both images
    BMP8Free(image);
    BMP8Free(rotated);

    // Print confirmation message
    printf("Image saved as '%s' after rotation type %d.\n", outputFile, r);

    return 0;
}


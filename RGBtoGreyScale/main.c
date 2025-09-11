#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define BMP header size
#define BMP_HEADER_SIZE 54
// Size of color table for 8-bit BMP
#define BMP_COLOR_TABLE_SIZE 1024

// Structure to store 24-bit BMP image data
typedef struct {
    unsigned char header[BMP_HEADER_SIZE]; // BMP file header
    unsigned char *data;                   // Pointer to pixel data
    int width;                             // Image width
    int height;                            // Image height
    int bitDepth;                           // Bits per pixel (should be 24)
    int rowSize;                            // Size of one row (including padding)
} BMP24Image; // Structure to store 24-bit BMP image data

// Structure to store 8-bit BMP image data
typedef struct {
    unsigned char header[BMP_HEADER_SIZE];    // BMP file header
    unsigned char colorTable[BMP_COLOR_TABLE_SIZE]; // 8-bit color table
    unsigned char *data;                       // Pointer to pixel data
    int width;                                 // Image width
    int height;                                // Image height
    int bitDepth;                              // Bits per pixel (should be 8)
} BMP8Image; // Structure to store 8-bit BMP image data

// Convert RGB color to grayscale
unsigned char rgbToGray(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned char)(0.3*r + 0.59*g + 0.11*b);
}

// Read a 24-bit BMP image from file
BMP24Image* BMP24Read(const char* filename) {
    // Open file in binary read mode
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        return NULL;
    }

    // Allocate memory for BMP24Image
    BMP24Image *img = (BMP24Image*)malloc(sizeof(BMP24Image));
    // Read BMP header
    fread(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, file);

    // Extract image width from header
    img->width = *(int*)&img->header[18];
    // Extract image height from header
    img->height = *(int*)&img->header[22];
    // Extract bit depth from header
    img->bitDepth = *(short*)&img->header[28];

    // Check if the image is 24-bit
    if (img->bitDepth != 24) {
        fprintf(stderr, "Not a 24-bit BMP\n");
        free(img);
        fclose(file);
        return NULL;
    }

    // Calculate row size (with padding)
    img->rowSize = (img->width * 3 + 3) & (~3);
    // Allocate memory for pixel data
    img->data = (unsigned char*)malloc(img->rowSize * img->height);
    // Read pixel data
    fread(img->data, sizeof(unsigned char), img->rowSize * img->height, file); 

    // Close file
    fclose(file);
    return img;
}

// Convert a 24-bit BMP image to grayscale in-place
void BMP24ConvertToGrayscale(BMP24Image* img24) {
    for (int y = 0; y < img24->height; y++) {
        // Pointer to the start of row
        unsigned char *row = img24->data + y * img24->rowSize;
        for (int x = 0; x < img24->width; x++) {
            // Pointer to pixel (BGR)
            unsigned char *pixel = row + x * 3;
            // Convert to grayscale
            unsigned char gray = rgbToGray(pixel[2], pixel[1], pixel[0]);
            // Set all channels to grayscale
            pixel[0] = pixel[1] = pixel[2] = gray;
        }
    }
}

// Save a 24-bit BMP image to file
void BMP24Save(const char* filename, BMP24Image* img24) {
    // Open file in binary write mode
    FILE *file = fopen(filename, "wb");
    // Write BMP header
    fwrite(img24->header, sizeof(unsigned char), BMP_HEADER_SIZE, file);
    // Write pixel data
    fwrite(img24->data, sizeof(unsigned char), img24->rowSize * img24->height, file);
    // Close file
    fclose(file);
}

// Convert 24-bit BMP image to 8-bit grayscale BMP image
BMP8Image* BMP24ConvertTo8(BMP24Image* img24) {
    // Allocate memory for BMP8Image
    BMP8Image *img8 = (BMP8Image*)malloc(sizeof(BMP8Image));
    // Copy header from 24-bit image
    memcpy(img8->header, img24->header, BMP_HEADER_SIZE);

    img8->width = img24->width;
    img8->height = img24->height;
    img8->bitDepth = 8;

    // Set bit depth to 8
    *(short*)&img8->header[28] = 8;
    // Set offset to pixel data (header + color table)
    *(int*)&img8->header[10] = 54 + 1024;

    // Row size with padding
    int rowSize = (img8->width + 3) & (~3);
    // Set image size in header
    *(int*)&img8->header[34] = rowSize * img8->height;

    // Fill grayscale color table
    for (int i = 0; i < 256; i++) {
        img8->colorTable[i*4 + 0] = i;
        img8->colorTable[i*4 + 1] = i;
        img8->colorTable[i*4 + 2] = i;
        img8->colorTable[i*4 + 3] = 0;
    }

    // Allocate memory for pixel data
    unsigned char* dataWithPadding = (unsigned char*)malloc(rowSize * img8->height);

    // Convert each pixel from 24-bit to 8-bit grayscale
    for(int y = 0; y < img8->height; y++) {
        unsigned char* row24 = img24->data + y * img24->rowSize;
        unsigned char* row8 = dataWithPadding + y * rowSize;

        for(int x = 0; x < img8->width; x++) {
            unsigned char* pixel24 = row24 + x*3;
            row8[x] = rgbToGray(pixel24[2], pixel24[1], pixel24[0]);
        }

        for(int p = img8->width; p < rowSize; p++) {
            // Padding bytes
            row8[p] = 0;
        }
    }

    // Assign pixel data to image structure
    img8->data = dataWithPadding;
    return img8;
}

// Save an 8-bit BMP image to file
void BMP8Save(const char* filename, BMP8Image* img8) {
    // Open file in binary write mode
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Cannot create file %s\n", filename);
        return;
    }

    // Write BMP header
    fwrite(img8->header, sizeof(unsigned char), BMP_HEADER_SIZE, file);
    // Write color table
    fwrite(img8->colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, file);

    // Row size with padding
    int rowSize = (img8->width + 3) & (~3);
    // Write pixel data
    fwrite(img8->data, sizeof(unsigned char), rowSize * img8->height, file);
    // Close file
    fclose(file);
}

// Free memory allocated for 24-bit BMP image
void BMP24Free(BMP24Image* img24) {
    if (img24 != NULL){
        free(img24->data);
        free(img24);
    }
}

// Free memory allocated for 8-bit BMP image
void BMP8Free(BMP8Image* img8) {
    if (img8 != NULL){
        free(img8->data);
        free(img8);
    }
}

int main() {
    // Read 24-bit BMP
    BMP24Image *img24 = BMP24Read("../Test_Images/lizard.bmp");
    if (!img24) return 1;

    // Convert to 24-bit grayscale
    BMP24ConvertToGrayscale(img24);
    // Save 24-bit grayscale
    BMP24Save("images/lizard_greyscale24.bmp", img24);

    // Convert 24-bit grayscale to 8-bit grayscale
    BMP8Image *img8 = BMP24ConvertTo8(img24);
    // Save 8-bit grayscale
    BMP8Save("images/lizard_greyscale8bit.bmp", img8);

    // Free memory for 24-bit image
    BMP24Free(img24);
    // Free memory for 8-bit image
    BMP8Free(img8);

    fprintf(stdout, "Done: 24-bit greyscale and 8-bit grayscale created.\n");
    return 0;
}

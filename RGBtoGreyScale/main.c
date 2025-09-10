#include <stdio.h>
#include <stdlib.h>

// Size of BMP header in bytes
#define BMP_HEADER_SIZE 54
// Size of color table
#define BMP_COLOR_TABLE_SIZE 1024

// --- Function Prototypes ---
void readBMP(const char* filename, unsigned char* header, unsigned char* colorTable, unsigned char** buf, int* width, int* height, int* bitDepth);
void writeBMP(const char* filename, unsigned char* header, unsigned char* colorTable, unsigned char* buf, int width, int height, int bitDepth);
unsigned char rgbToGray(unsigned char r, unsigned char g, unsigned char b);
void convertToGrayscale(unsigned char* buf, int width, int height);

int main() {
    // Arrays and pointers to hold image data
    // BMP header
    unsigned char header[BMP_HEADER_SIZE];
    // Color table
    unsigned char colorTable[BMP_COLOR_TABLE_SIZE];
    // Buffer for pixel data
    unsigned char* buf = NULL;
    // Image properties
    int width, height, bitDepth;

    // Read the BMP image
    readBMP("../Test_Images/lizard.bmp", header, colorTable, &buf, &width, &height, &bitDepth);

    // Convert the image to grayscale
    convertToGrayscale(buf, width, height);

    // Write the modified image to a new file
    writeBMP("images/lizard_greyscale.bmp", header, colorTable, buf, width, height, bitDepth);

    // Free allocated memory
    free(buf);

    printf("Image successfully converted to greyscale.\n");
    return 0;
}

// Reads a BMP image into memory
void readBMP(const char* filename, unsigned char* header, unsigned char* colorTable, unsigned char** buf, int* width, int* height, int* bitDepth) {
    // Open file in binary read mode
    FILE* fInput = fopen(filename, "rb");
    if (!fInput) {
        fprintf(stderr, "Unable to open file!\n");
        exit(1);
    }

    // Read BMP header (first 54 bytes)
    fread(header, sizeof(unsigned char), BMP_HEADER_SIZE, fInput);

    // Extract image properties from header
    // Image width
    *width = *(int*)&header[18];
    // Image height
    *height = *(int*)&header[22];
    // Bits per pixel
    *bitDepth = *(int*)&header[28];

    // Read color table if present
    if (*bitDepth <= 8) {
        fread(colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, fInput);
    }

    // Allocate memory for pixel data
    int imgSize = (*width) * (*height) * ((*bitDepth) / 8);
    *buf = (unsigned char*)malloc(imgSize);
    if (!(*buf)) {
        fprintf(stderr, "Memory allocation failed!\n");
        fclose(fInput);
        exit(1);
    }

    // Read pixel data into buffer
    fread(*buf, sizeof(unsigned char), imgSize, fInput);
    fclose(fInput);
}

// Writes a BMP image from memory to file
void writeBMP(const char* filename, unsigned char* header, unsigned char* colorTable, unsigned char* buf, int width, int height, int bitDepth) {
    // Open file in binary write mode
    FILE* fOutput = fopen(filename, "wb");
    if (!fOutput) {
        fprintf(stderr, "Unable to create file!\n");
        exit(1);
    }

    // Write BMP header
    fwrite(header, sizeof(unsigned char), BMP_HEADER_SIZE, fOutput);

    // Write color table if needed
    if (bitDepth <= 8) {
        fwrite(colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, fOutput);
    }

    // Write pixel data
    int imgSize = width * height * (bitDepth / 8);
    fwrite(buf, sizeof(unsigned char), imgSize, fOutput);

    fclose(fOutput);
}

// Converts an RGB pixel into a grayscale value
unsigned char rgbToGray(unsigned char r, unsigned char g, unsigned char b) {
    // Weighted sum based on human perception
    return (unsigned char)(0.3 * r + 0.59 * g + 0.11 * b);
}

// Converts the entire image buffer from RGB to grayscale
void convertToGrayscale(unsigned char* buf, int width, int height) {
    // 3 bytes per pixel (R, G, B)
    int imgSize = width * height * 3;
    for (int i = 0; i < imgSize; i += 3) {
        unsigned char gray = rgbToGray(buf[i], buf[i+1], buf[i+2]);
        // RED
        buf[i]   = gray;
        // GREEN
        buf[i+1] = gray;
        // BLUE
        buf[i+2] = gray;
    }
}

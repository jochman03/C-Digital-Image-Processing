#include <stdio.h>
#include <stdlib.h>

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

    // Read BMP header (first 54 bytes)
    fread(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, fInput);

    // Extract image width, height, and bit depth from header
    // Width stored at byte offset 18
    img->width = *(int*)&img->header[18];
    // Height stored at byte offset 22
    img->height = *(int*)&img->header[22];
    // Bits per pixel stored at byte offset 28
    img->bitDepth = *(short*)&img->header[28];

    // Calculate padded row size (rows aligned to 4 bytes)
    int rowSize = (img->width + 3) & ~3;
    // Total size of pixel data
    img->imgSize = rowSize * img->height;

    // Read color table for 8-bit BMP (1024 bytes)
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

    return img;  // Return pointer to BMP8Image structure
}


// Function to compute histogram of an 8-bit BMP image and save to a file
float* BMP8Histogram(BMP8Image* img, const char* filename){
    // Open file for writing histogram data
    FILE* fptr = fopen(filename, "w");
    if (!fptr) {
        fprintf(stderr, "Unable to create file %s!\n", filename);
        return NULL;
    }

    // Array to count pixel occurrences
    long int ihist[256];
    // Array to store normalized histogram
    float* hist = malloc(256 * sizeof(float));
    if(!hist){
        fprintf(stderr, "Memory allocation failed!\n");
        fclose(fptr);
        return NULL;
    }

    // Initialize histogram count array to zero
    for(int i = 0; i < 256; i++){
        ihist[i] = 0;
    }

    // Compute row size (padded to 4 bytes)
    int rowSize = (img->width + 3) & ~3;

    // Loop through all pixels and count occurrences of each intensity
    for(int y = 0; y < img->height; y++){
        for(int x = 0; x < img->width; x++){
            // Index in data array
            int idx = y * rowSize + x;
            unsigned char pixel = img->data[idx];
            // Increment count for pixel value
            ihist[pixel]++;
        }
    }

    // Total number of pixels in the image
    long int sum = img->width * img->height;

    // Normalize histogram (divide counts by total number of pixels)
    for(int i = 0; i < 256; i++){
        hist[i] = (float)ihist[i] / (float)sum;
    } 

    // Save normalized histogram to file (one value per line)
    for(int i = 0; i < 256; i++){
        fprintf(fptr, "%f\n", hist[i]);
    }

    // Close histogram file
    fclose(fptr);
    // Return pointer to normalized histogram
    return hist;
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

    // Write color table if BMP is 8-bit
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
    const char *inputFile = "../Test_Images/lizard_greyscale8bit.bmp";

    // Read BMP image from file
    BMP8Image *image = BMP8read(inputFile);

    // Compute histogram and save to text file
    float* imgHist = BMP8Histogram(image, "data/lizard_histogram.txt");

    // Free allocated memory
    free(imgHist);
    BMP8Free(image);

    return 0;
}

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


// Function to compute histogram (normalized) of an 8-bit BMP image
// Optionally saves histogram values to a text file
float* BMP8Histogram(BMP8Image* img, bool saveFile, const char* filename){
    FILE* fptr = NULL;
    if(saveFile){
        fptr = fopen(filename, "w");
        if (!fptr) {
            fprintf(stderr, "Unable to create file %s!\n", filename);
            return NULL;
        }
    }

    // raw histogram counts
    long int ihist[256] = {0};
    // normalized histogram
    float* hist = malloc(256 * sizeof(float));
    if(!hist){
        fprintf(stderr, "Memory allocation failed!\n");
        if(saveFile){
            fclose(fptr);
        }
        return NULL;
    }

    // row size including padding
    int rowSize = (img->width + 3) & ~3;

    // Count occurrences of each intensity
    for(int y = 0; y < img->height; y++){
        for(int x = 0; x < img->width; x++){
            int idx = y * rowSize + x;
            unsigned char pixel = img->data[idx];
            ihist[pixel]++;
        }
    }

    // Normalize histogram
    long int totalPixels = img->width * img->height;
    for(int i = 0; i < 256; i++){
        hist[i] = (float)ihist[i] / (float)totalPixels;
    } 

    // Optionally save histogram to file
    if(saveFile){
        for(int i = 0; i < 256; i++){
            fprintf(fptr, "%f\n", hist[i]);
        }
        fclose(fptr);
    }
    return hist;
}


// Function to perform histogram equalization on 8-bit BMP image
BMP8Image* BMP8HistogramEqualization(BMP8Image* img){
    BMP8Image* equalizedImg = malloc(sizeof(BMP8Image));
    if(!equalizedImg){
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    // Copy metadata
    memcpy(equalizedImg->header, img->header, BMP_HEADER_SIZE);
    equalizedImg->width = img->width;
    equalizedImg->height = img->height;
    equalizedImg->bitDepth = img->bitDepth;

    // Copy color table if 8-bit
    if (img->bitDepth <= 8) {
        memcpy(equalizedImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    // Compute image size
    int rowSize = (img->width + 3) & ~3;
    equalizedImg->imgSize = rowSize * img->height;

    // Allocate memory for new pixel data
    equalizedImg->data = malloc(equalizedImg->imgSize);
    if (!equalizedImg->data) {
        fprintf(stderr, "Memory allocation failed for pixel data!\n");
        free(equalizedImg);
        return NULL;
    }

    // Compute histogram
    float* hist = BMP8Histogram(img, false, NULL);

    // Compute cumulative distribution function (CDF) and mapping
    int histeq[256];
    for(int i = 0; i < 256; i++){
        float sum = 0.0f;
        for(int j = 0; j <= i; j++){
            sum += hist[j];
        }
        // map to [0,255]
        histeq[i] = (int)(255 * sum + 0.5f);
    }
    // free normalized histogram
    free(hist);

    // Apply equalization mapping to pixels
    for(int y = 0; y < img->height; y++){
        for(int x = 0; x < img->width; x++){
            int idx = y * rowSize + x;
            equalizedImg->data[idx] = histeq[img->data[idx]];
        }
    }

    return equalizedImg;
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
    const char *inputFile = "../Test_Images/lena512.bmp";

    // Read original BMP image
    BMP8Image *image = BMP8read(inputFile);

    // Compute histogram of original image and save to file
    float* imgHist = BMP8Histogram(image, true, "data/lena512_histogram.txt");

    // Perform histogram equalization
    BMP8Image *equalized = BMP8HistogramEqualization(image);

    // Save equalized image
    BMP8save("images/lena512_equalized.bmp", equalized);

    // Save histogram of equalized image
    float* eqHist = BMP8Histogram(equalized, true, "data/lena512_equalized_histogram.txt");

    // Free memory
    free(imgHist);
    free(eqHist);
    BMP8Free(image);
    BMP8Free(equalized);

    return 0;
}

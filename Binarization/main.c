#include <stdio.h>
#include <stdlib.h>

// Define BMP header size
#define BMP_HEADER_SIZE 54
// Define white color value for 8-bit BMP
#define WHITE 255
// Define black color value for 8-bit BMP
#define BLACK 0
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
    img->imgSize = img->width * img->height;

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

// Function to binarize an 8-bit BMP image using a threshold
void BMP8Binarize(BMP8Image* img, int threshold) {
    for (int i = 0; i < img->imgSize; i++) {
        // Set pixel to white if above threshold, otherwise black
        img->data[i] = (img->data[i] > threshold) ? WHITE : BLACK;
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
    // Threshold for binarization
    int threshold = 150;

    // Read BMP image
    BMP8Image *image = BMP8read(inputFile);
    if (!image) exit(1);

    // Apply binarization
    BMP8Binarize(image, threshold);

    // Prepare output file name
    char outFilename[100];
    sprintf(outFilename, "images/lizard_binary_thr%d.bmp", threshold);
    // Save the binarized image
    BMP8save(outFilename, image);

    // Free allocated memory
    BMP8Free(image);

    // Print confirmation message
    fprintf(stdout, "Created %s with threshold %d\n", outFilename, threshold);

    return 0;
}

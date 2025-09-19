#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fixed BMP header size
#define BMP_HEADER_SIZE 54
// Fixed color table size for 8-bit BMP (256 * 4 bytes = 1024)
#define BMP_COLOR_TABLE_SIZE 1024

// Structure to hold 8-bit BMP image data
typedef struct {
    unsigned char header[BMP_HEADER_SIZE];          // BMP file header (54 bytes)
    unsigned char colorTable[BMP_COLOR_TABLE_SIZE]; // Color table (if <= 8-bit image)
    unsigned char *data;                            // Pixel data
    int width;                                      // Image width in pixels
    int height;                                     // Image height in pixels
    int bitDepth;                                   // Bits per pixel (8 for grayscale)
    int imgSize;                                    // Total pixel data size (with padding)
} BMP8Image;


// Function to read an 8-bit BMP image from file
BMP8Image* BMP8read(const char* filename) {
    // open file in binary mode
    FILE *fInput = fopen(filename, "rb");
    if (!fInput) {
        fprintf(stderr, "Unable to open file %s!\n", filename);
        return NULL;
    }

    // Allocate memory for image structure
    BMP8Image *img = (BMP8Image*)malloc(sizeof(BMP8Image));
    if (!img) {
        fprintf(stderr, "Memory allocation failed!\n");
        fclose(fInput);
        return NULL;
    }

    // Read header (54 bytes)
    fread(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, fInput);

    // Extract metadata from BMP header
    // width (offset 18)
    img->width = *(int*)&img->header[18];
    // height (offset 22)
    img->height = *(int*)&img->header[22];
    // bits per pixel (offset 28)
    img->bitDepth = *(short*)&img->header[28];

    // Compute row size (aligned to 4 bytes)
    int rowSize = (img->width + 3) & ~3;
    img->imgSize = rowSize * img->height;

    // Read color table (only if <= 8-bit image)
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

    // return pointer to loaded image
    return img;
}

// Function to blur image using averaging filter
BMP8Image* BMP8Blur(BMP8Image* img, unsigned int size) {
    // Allocate memory for blurred image structure
    BMP8Image* blurredImg = malloc(sizeof(BMP8Image));
    if (!blurredImg) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    // Copy BMP header and metadata
    memcpy(blurredImg->header, img->header, BMP_HEADER_SIZE);
    blurredImg->width = img->width;
    blurredImg->height = img->height;
    blurredImg->bitDepth = img->bitDepth;

    // Copy color table if image is 8-bit
    if (img->bitDepth <= 8) {
        memcpy(blurredImg->colorTable, img->colorTable, BMP_COLOR_TABLE_SIZE);
    }

    // Compute padded row size (each row aligned to 4 bytes)
    int rowSize = (img->width + 3) & ~3;
    blurredImg->imgSize = rowSize * img->height;

    // Allocate memory for pixel data of blurred image
    blurredImg->data = malloc(blurredImg->imgSize);
    if (!blurredImg->data) {
        fprintf(stderr, "Memory allocation failed for pixel data!\n");
        free(blurredImg);
        return NULL;
    }

    // Copy original image data to blurred image (so border pixels remain unchanged)
    memcpy(blurredImg->data, img->data, blurredImg->imgSize);

    // Create averaging kernel of size (size × size)
    float* kernel = malloc(size * size * sizeof(float));
    if (!kernel) {
        fprintf(stderr, "Memory allocation failed for kernel!\n");
        free(blurredImg->data);
        free(blurredImg);
        return NULL;
    }

    // Fill kernel with equal weights
    float value = 1.0f / (size * size);
    for (int i = 0; i < size * size; i++) {
        kernel[i] = value;
    }

    // Convolution offset (half of kernel size)
    int offset = size / 2;

    // Apply convolution (ignoring border pixels)
    for (int y = offset; y < img->height - offset; y++) {
        for (int x = offset; x < img->width - offset; x++) {
            float sum = 0.0f;

            for (int j = -offset; j <= offset; j++) {
                for (int i = -offset; i <= offset; i++) {
                    int pixelVal = img->data[(y + j) * rowSize + (x + i)];
                    float weight = kernel[(j + offset) * size + (i + offset)];
                    sum += weight * pixelVal;
                }
            }

            // Clamp result to valid grayscale range [0, 255]
            if (sum < 0) sum = 0;
            if (sum > 255) sum = 255;

            blurredImg->data[y * rowSize + x] = (unsigned char)sum;
        }
    }

    // Free kernel memory
    free(kernel);

    return blurredImg;
}

// Function to save BMP image to file
void BMP8save(const char* filename, BMP8Image* img) {
    FILE *fOutput = fopen(filename, "wb");
    if (!fOutput) {
        fprintf(stderr, "Unable to create file %s!\n", filename);
        return;
    }

    // Write BMP header
    fwrite(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, fOutput);

    // Write color table if <= 8-bit image
    if (img->bitDepth <= 8) {
        fwrite(img->colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, fOutput);
    }

    // Write pixel data
    fwrite(img->data, sizeof(unsigned char), img->imgSize, fOutput);

    fclose(fOutput);
}


// Function to free allocated memory
void BMP8Free(BMP8Image* img) {
    if (img) {
        // free pixel data
        free(img->data);
        // free struct
        free(img);
    }
}

int main(){
    const char *inputFile = "../Test_Images/lizard_greyscale8bit.bmp";

    // Read BMP image
    BMP8Image *image = BMP8read(inputFile);

    // Apply blur filter with 3×3 averaging kernel
    BMP8Image *blurred = BMP8Blur(image, 3);
    if (blurred) {
        BMP8save("images/lizard_blurred_3x3.bmp", blurred);
        BMP8Free(blurred);
    }

    BMP8Free(image);

    return 0;
}

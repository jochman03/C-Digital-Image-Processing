#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define BMP header size
#define BMP_HEADER_SIZE 54     
// Size of color table for 8-bit BMP
#define BMP_COLOR_TABLE_SIZE 1024 
// Maximum value of a pixel
#define MAX_BRIGHTNESS 255
// Minimum value of a pixel
#define MIN_BRIGHTNESS 0

// Macro definitions for minimum and maximum values
// Returns the smaller of a and b
#define MIN(a, b) ((a) < (b) ? (a) : (b))
 // Returns the larger of a and b
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Structure to store 24-bit BMP image data
typedef struct {
    unsigned char header[BMP_HEADER_SIZE]; // BMP file header
    unsigned char *data;                   // Pointer to pixel data (BGR format)
    int width;                             // Image width in pixels
    int height;                            // Image height in pixels
    int bitDepth;                           // Bits per pixel (should be 24)
    int rowSize;                            // Size of one row including padding
} BMP24Image; // Structure for storing 24-bit BMP image

// Read a 24-bit BMP image from file
BMP24Image* BMP24Read(const char* filename) {
    // Open file in binary read mode
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        return NULL;
    }

    // Allocate memory for BMP24Image structure
    BMP24Image *img = (BMP24Image*)malloc(sizeof(BMP24Image));
    // Read BMP header from file
    fread(img->header, sizeof(unsigned char), BMP_HEADER_SIZE, file);

    // Extract image width from BMP header (bytes 18-21)
    img->width = *(int*)&img->header[18];
    // Extract image height from BMP header (bytes 22-25)
    img->height = *(int*)&img->header[22];
    // Extract bit depth from BMP header (bytes 28-29)
    img->bitDepth = *(short*)&img->header[28];

    // Check if the image is 24-bit
    if (img->bitDepth != 24) {
        fprintf(stderr, "Not a 24-bit BMP\n");
        free(img);
        fclose(file);
        return NULL;
    }

    // Calculate row size including padding to multiple of 4 bytes
    img->rowSize = (img->width * 3 + 3) & (~3);
    // Allocate memory for pixel data
    img->data = (unsigned char*)malloc(img->rowSize * img->height);
    // Read pixel data from file
    fread(img->data, sizeof(unsigned char), img->rowSize * img->height, file); 

    // Close file
    fclose(file);
    return img;
}

// Apply sepia filter to 24-bit BMP image
BMP24Image* BMP24Sepia(BMP24Image* img){
    // Allocate new BMP image structure for sepia image
    BMP24Image* sepiaImg = malloc(sizeof(BMP24Image));
    if (!sepiaImg) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }
    // Copy header and metadata
    memcpy(sepiaImg->header, img->header, BMP_HEADER_SIZE);
    sepiaImg->width = img->width;
    sepiaImg->height = img->height;
    sepiaImg->bitDepth = img->bitDepth;
    sepiaImg->rowSize = img->rowSize;

    // Allocate memory for sepia pixel data
    sepiaImg->data = malloc(sepiaImg->rowSize * sepiaImg->height);
    if (!sepiaImg->data) {
        fprintf(stderr, "Memory allocation failed for pixel data!\n");
        free(sepiaImg);
        return NULL;
    }

    // Loop through each pixel
    for (int y = 0; y < sepiaImg->height; y++) {
        for (int x = 0; x < sepiaImg->width; x++) {
            
            int idx = y * sepiaImg->rowSize + x * 3;
            // Extract original BGR components
            int blue = img->data[idx];
            int green = img->data[idx + 1];
            int red = img->data[idx + 2];

            // Apply sepia transformation
            int tr = (int)(0.393 * red + 0.769 * green + 0.189 * blue);
            int tg = (int)(0.349 * red + 0.686 * green + 0.168 * blue);
            int tb = (int)(0.272 * red + 0.534 * green + 0.131 * blue);

            // Clamp values to maximum brightness
            // Blue
            sepiaImg->data[idx] = MIN(tb, 255);
            // Green
            sepiaImg->data[idx + 1] = MIN(tg, 255);
            // Red
            sepiaImg->data[idx + 2] = MIN(tr, 255);
        }
    }
    return sepiaImg;
}

// Save a 24-bit BMP image to file
void BMP24Save(const char* filename, BMP24Image* img24) {
    // Open file in binary write mode
    FILE *file = fopen(filename, "wb");
    // Write BMP header to file
    fwrite(img24->header, sizeof(unsigned char), BMP_HEADER_SIZE, file);
    // Write pixel data to file
    fwrite(img24->data, sizeof(unsigned char), img24->rowSize * img24->height, file);
    // Close file
    fclose(file);
}

// Free memory allocated for 24-bit BMP image
void BMP24Free(BMP24Image* img24) {
    if (img24 != NULL){
        // Free pixel data
        free(img24->data);
        // Free structure
        free(img24);
    }
}

int main() {
    // Read 24-bit BMP image
    BMP24Image *img24 = BMP24Read("../Test_Images/lizard.bmp");
    if (!img24) {
        // Exit if reading failed
        return 1;
    } 

    // Apply sepia filter
    BMP24Image *sepiaImg = BMP24Sepia(img24);

    // Save sepia image to file
    BMP24Save("images/lizard_sepia.bmp", sepiaImg);

    // Free allocated memory
    BMP24Free(img24);
    BMP24Free(sepiaImg);

    fprintf(stdout, "Sepia image saved.\n");
    return 0;
}

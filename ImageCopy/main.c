#include <stdio.h>
#include <stdlib.h>

/*
=========================================================
   BMP FILE STRUCTURE (simplified, little-endian format)
=========================================================

A BMP file is composed of several parts:

1) BITMAP FILE HEADER (14 bytes)
   - Signature (2 bytes): "BM" for Bitmap
   - File size (4 bytes): Total size of BMP file in bytes
   - Reserved (4 bytes): Reserved for application use
   - Data offset (4 bytes): Offset from start of file to pixel data

2) BITMAP INFO HEADER (40 bytes)  [part of the "54-byte header"]
   - Header size (4 bytes): Size of this header (40 bytes)
   - Width (4 bytes): Image width in pixels
   - Height (4 bytes): Image height in pixels
   - Planes (2 bytes): Number of color planes (always 1)
   - Bit count (2 bytes): Bits per pixel (1, 4, 8, 24, or 32)
   - Compression (4 bytes): Compression type (0 = none)
   - Image size (4 bytes): Raw bitmap data size (can be 0 for uncompressed)
   - X pixels per meter (4 bytes): Horizontal resolution
   - Y pixels per meter (4 bytes): Vertical resolution
   - Colors used (4 bytes): Number of colors in palette
   - Important colors (4 bytes): Number of important colors

3) COLOR TABLE (optional, only for <= 8 bits per pixel)
   - Up to 256 entries (4 bytes each: Blue, Green, Red, Reserved)
   - Size = 1024 bytes for 8-bit BMP

4) PIXEL ARRAY (bitmap data)
   - Actual image pixels, starting at offset given in File Header
   - Each row padded to 4-byte boundary
   - For 24-bit BMP: Each pixel = 3 bytes (B, G, R)

---------------------------------------------------------
   Total header size = 14 (file header) + 40 (info header)
                     = 54 bytes
   + Color table (if needed)
   + Pixel array
---------------------------------------------------------
*/

// Define constants for BMP format
#define BMP_HEADER_SIZE 54          // BMP header size in bytes
#define BMP_COLOR_TABLE_SIZE 1024   // Color table size for 8-bit images
#define CUSTOM_IMAGE_SIZE 1024*1024 // Fixed buffer size for image data (1 MB)

// Function prototypes
void imageReader(const char* imgName, int* _width, int* _height, int* _bitDepth, unsigned char* _header, unsigned char* _colorTable, unsigned char* _buf);
void imageWriter(const char* imgName, unsigned char* header, unsigned char* colorTable, unsigned char* buf, int bitDepth);

int main(){
    // Variables to hold image properties
    int imgWidth, imgHeight, imgBitDepth;

    // Buffers for header, color table, and pixel data
    unsigned char imgheader[BMP_HEADER_SIZE];
    unsigned char imgColorTable[BMP_COLOR_TABLE_SIZE];
    unsigned char imgBuf[CUSTOM_IMAGE_SIZE];

    // Input and output image file names
    const char imgName[] = "../Test_Images/cameraman.bmp";
    const char newImgName[] = "images/cameraman_cpy.bmp";

    // Read the image into memory
    imageReader(imgName, &imgWidth, &imgHeight, &imgBitDepth, imgheader, imgColorTable, imgBuf);
    fprintf(stdout, "Found an Image. Processing.\n");

    // Write the copied image to a new file
    imageWriter(newImgName, imgheader, imgColorTable, imgBuf, imgBitDepth);
    fprintf(stdout, "File copied successfully.\n");
    return 0;
}

// Reads BMP image into memory (header, color table, and pixel data)
void imageReader(const char* imgName, int* _width, int* _height, int* _bitDepth, unsigned char* _header, unsigned char* _colorTable, unsigned char* _buf){
    FILE* streamIn;
    streamIn = fopen(imgName, "rb"); // Open input file in binary read mode
    if(streamIn == (FILE*) 0){
        fprintf(stderr, "Unable to open file!\n");
        exit(0);
    }

    // Read BMP header (54 bytes)
    for(int i = 0; i < 54; i++){
        _header[i] = getc(streamIn);
    }

    // Extract image metadata from header
    *_width = *(int*) &_header[18];    // Image width
    *_height = *(int*) &_header[22];   // Image height
    *_bitDepth = *(int*) &_header[28]; // Color depth (bits per pixel)

    // If image is grayscale or indexed (<= 8-bit), read color table
    if(*_bitDepth <= 8){
        fread(_colorTable, sizeof(unsigned char), 1024, streamIn);
    }

    // Read raw pixel data into buffer
    fread(_buf, sizeof(unsigned char), CUSTOM_IMAGE_SIZE, streamIn);

    fclose(streamIn); // Close input file
}

// Writes BMP image from memory (header, color table, and pixel data) to a file
void imageWriter(const char* imgName, unsigned char* header, unsigned char* colorTable, unsigned char* buf, int bitDepth){
    FILE* fOut = fopen(imgName, "wb"); // Open output file in binary write mode
    if(fOut == (FILE*) 0){
        fprintf(stderr, "Unable to create file!\n");
        exit(0);
    }

    // Write BMP header
    fwrite(header, sizeof(unsigned char), BMP_HEADER_SIZE, fOut);

    // If necessary, write color table
    if(bitDepth <= 8){
        fwrite(colorTable, sizeof(unsigned char), 1024, fOut);
    }

    // Write pixel data
    fwrite(buf, sizeof(unsigned char), CUSTOM_IMAGE_SIZE, fOut);

    fclose(fOut); // Close output file
}

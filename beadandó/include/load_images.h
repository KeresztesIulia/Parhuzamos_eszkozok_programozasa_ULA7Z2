#ifndef LOAD_IMAGES_H
#define LOAD_IMAGES_H

#include "image_data.h"

int ImageToPixelData(unsigned char* originalImageData, Pixel* pixelData, int size, int channels);
int LoadAndConvert(char* path, Pixel** pixelData, int* width, int* height, int* channels);
int PixelDataToArray(Pixel* pixelData, unsigned char* convertedImage, int size, int channels);

int LoadAsColorMask(char* path, Image* image, int threshold);
int LoadAsAlphaMask(char* path, Image* image, int threshold);
int LoadAsImage(char* path, Image* image);
int SaveImage(char* path, Image* image);

int CopyImage(Image* original, Image* copy);
int CopyPixelData(Pixel* original, Pixel* copy, int size);

#endif
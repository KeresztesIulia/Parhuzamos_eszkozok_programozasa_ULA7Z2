#ifndef LOAD_IMAGES_H
#define LOAD_IMAGES_H

#include "time_info.h"
#include "image_data.h"

int ImageToPixelData(unsigned char* originalImageData, Pixel* pixelData, int size, int channels);
int LoadAndConvert(char* path, Pixel** pixelData, int* width, int* height, int* channels);
int PixelDataToArray(Pixel* pixelData, unsigned char* convertedImage, int size, int channels);
int ExpandMask(Mask* mask, unsigned char* fullImage);

int LoadAsImage(char* path, Image* image);
int LoadAsColorMask(char* path, Mask* mask, int threshold, TimeInfo* timeInfo);
int LoadAsAlphaMask(char* path, Mask* mask, int threshold, TimeInfo* timeInfo);

int SaveImage(char* path, Image* image);
int SaveMask(char* path, Mask* mask);

int CopyImage(Image* original, Image* copy);
int CopyPixelData(Pixel* original, Pixel** copy, int size);
void freeImage(Image* image);
void freeMask(Mask* mask);

#endif
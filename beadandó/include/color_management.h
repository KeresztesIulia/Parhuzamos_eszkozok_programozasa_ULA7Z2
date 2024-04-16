#ifndef COLOR_MANAGEMENT_H
#define COLOR_MANAGEMENT_H

#include "image_data.h"

#define RGB_MAX 255

double LinearRGB(unsigned char value);

// PixelData to pixelData
int ToGrayscale(Pixel* original, Pixel* grayscale, int size);
int ToBlackAndWhite(Pixel* original, Pixel* blackandwhite, int size, int threshold);
int AlphaToGreyscale(Pixel* original, Pixel* grayscale, int size, int threshold);
int Luminance(Pixel* original, Pixel* grayed);

// Pixel to array
int ToGrayscaleMASK(Pixel* original, unsigned char* grayscale, int size);
int ToBlackAndWhiteMASK(Pixel* original, unsigned char* blackandwhite, int size, int threshold);
int AlphaToGreyscaleMASK(Pixel* original, unsigned char* grayscale, int size, int threshold);
int LuminanceMASK(Pixel* original, unsigned char* grayed);

#endif
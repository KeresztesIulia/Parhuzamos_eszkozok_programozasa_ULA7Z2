#ifndef COLOR_MANAGEMENT_H
#define COLOR_MANAGEMENT_H

#include "image_data.h"

int ToGrayscale(Pixel* original, Pixel* grayscale, int size);
int ToBlackAndWhite(Pixel* original, Pixel* blackandwhite, int size, int threshold);
int AlphaToGreyscale(Pixel* original, Pixel* grayscale, int size, int threshold);
double LinearRGB(unsigned char value);
int Luminance(Pixel* original, Pixel* grayed);

#endif
// combine on alpha: first image has alpha channel, it serves as an alpha mask for the second image
// combine on alpha mask: two images combined based on a third image that serves as an alpha mask
// combine on color mask: two images combined based on a third image that serves as a color mask

// pre = use pre-loaded images

#ifndef COMBINE_IMAGES_H
#define COMBINE_IMAGES_H

#include "image_data.h"

int CombineOnAlpha(char* img1Path, char* img2Path, Image** result, int threshold);
int CombineOnAlphaMask(char* img1Path, char* img2Path, char* maskPath, Image** result, int threshold);
int CombineOnColorMask(char* img1Path, char* img2Path, char* maskPath, Image** result, int threshold);

int preCombineOnAlpha(Image* img1, Image* img2, Image** result, int threshold);
int preCombine(Image* img1, Image* img2, Mask* mask, Image** result, int threshold);

#endif
#include <stdio.h>
#include <stdlib.h>

#include "load_images.h"
#include "combine_images.h"
#include "color_management.h"

int CombineOnAlpha(char* img1Path, char* img2Path, Image** result, int threshold)
{
    Image* img1 = (Image*)malloc(sizeof(Image));
    if (img1 == NULL)
    {
        printf("CombineOnAlpha: Failed to allocate first image!\n");
        return -1;
    }
    Image* img2 = (Image*)malloc(sizeof(Image));
    if (img2 == NULL)
    {
        printf("CombineOnAlpha: Failed to allocate second image!\n");
        return -1;
    }

    int err = LoadAsImage(img1Path, img1);
    if (err != 0)
    {
        printf("CombineOnAlpha: Failed to load first image!\n");
        return -2;
    }
    err = LoadAsImage(img2Path, img2);
    if (err != 0)
    {
        printf("CombineOnAlpha: Failed to load second image!\n");
        return -2;
    }

    preCombineOnAlpha(img1, img2, result, threshold);
}

int CombineOnAlphaMask(char* img1Path, char* img2Path, char* maskPath, Image** result, int threshold);
int CombineOnColorMask(char* img1Path, char* img2Path, char* maskPath, Image** result, int threshold);

int preCombineOnAlpha(Image* img1, Image* img2, Image** result, int threshold)
{
    Image* mask = (Image*)malloc(sizeof(Image));
    int err = CopyImage(img1, mask);
    AlphaToGreyscale(img1->pixelData, mask->pixelData, img1->size, threshold);
    preCombineOnAlphaMask(img1, img2, mask, result, threshold);
}

int preCombineOnAlphaMask(Image* img1, Image* img2, Image* mask, Image** result, int threshold)
{
    SaveImage("imgs/converted/combine_test.png", mask);
}
int preCombineOnColorMask(Image* img1, Image* img2, Image* mask, Image** result, int threshold);
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

    err = preCombineOnAlpha(img1, img2, result, threshold);
    if (err != 0)
    {
        printf("CombineOnAlpha: Combination failed!");
        return -3;
    }
    freeImage(img1);
    freeImage(img2);
    return 0;
}
int CombineOnAlphaMask(char* img1Path, char* img2Path, char* maskPath, Image** result, int threshold)
{
    Image* img1 = (Image*)malloc(sizeof(Image));
    if (img1 == NULL)
    {
        printf("CombineOnAlphaMask: Failed to allocate first image!\n");
        return -1;
    }
    Image* img2 = (Image*)malloc(sizeof(Image));
    if (img2 == NULL)
    {
        printf("CombineOnAlphaMask: Failed to allocate second image!\n");
        return -1;
    }
    Mask* mask = (Mask*)malloc(sizeof(Mask));
    if (img2 == NULL)
    {
        printf("CombineOnAlphaMask: Failed to allocate mask!\n");
        return -1;
    }

    int err = LoadAsImage(img1Path, img1);
    if (err != 0)
    {
        printf("CombineOnAlphaMask: Failed to load first image!\n");
        return -2;
    }
    err = LoadAsImage(img2Path, img2);
    if (err != 0)
    {
        printf("CombineOnAlphaMask: Failed to load second image!\n");
        return -2;
    }
    err = LoadAsAlphaMask(maskPath, mask, threshold);
    if (err != 0)
    {
        printf("CombineOnAlphaMask: Failed to load mask!\n");
        return -2;
    }

    err = preCombine(img1, img2, mask, result, threshold);
    if (err != 0)
    {
        printf("CombineOnAlphaMask: Combination failed!");
        return -3;
    }
    freeImage(img1);
    freeImage(img2);
    freeMask(mask);
    return 0;
}
int CombineOnColorMask(char* img1Path, char* img2Path, char* maskPath, Image** result, int threshold)
{
    Image* img1 = (Image*)malloc(sizeof(Image));
    if (img1 == NULL)
    {
        printf("CombineOnColorMask: Failed to allocate first image!\n");
        return -1;
    }
    Image* img2 = (Image*)malloc(sizeof(Image));
    if (img2 == NULL)
    {
        printf("CombineOnColorMask: Failed to allocate second image!\n");
        return -1;
    }
    Mask* mask = (Mask*)malloc(sizeof(Mask));
    if (img2 == NULL)
    {
        printf("CombineOnColorMask: Failed to allocate mask!\n");
        return -1;
    }

    int err = LoadAsImage(img1Path, img1);
    if (err != 0)
    {
        printf("CombineOnColorMask: Failed to load first image!\n");
        return -2;
    }
    err = LoadAsImage(img2Path, img2);
    if (err != 0)
    {
        printf("CombineOnColorMask: Failed to load second image!\n");
        return -2;
    }
    err = LoadAsColorMask(maskPath, mask, threshold);
    if (err != 0)
    {
        printf("CombineOnColorMask: Failed to load mask!\n");
        return -2;
    }

    err = preCombine(img1, img2, mask, result, threshold);
    if (err != 0)
    {
        printf("CombineOnColorMask: Combination failed!");
        return -3;
    }
    freeImage(img1);
    freeImage(img2);
    freeMask(mask);
    return 0;
}

int preCombineOnAlpha(Image* img1, Image* img2, Image** result, int threshold)
{
    if (img1 == NULL || img2 == NULL)
    {
        printf("preCombineOnAlpha: One of the images if null!\n");
        return -1;
    }
    Mask* mask = (Mask*)malloc(sizeof(Mask));
    if (mask == NULL)
    {
        printf("preCombineOnAlpha: Couldn't allocate mask!\n");
        return -2;
    }
    mask->width = img1->width;
    mask->height = img1->height;
    mask->size = img1->size;
    mask->array = (unsigned char*)malloc(mask->size * sizeof(unsigned char));
    int err = AlphaToGreyscaleMASK(img1->pixelData, mask->array, img1->size, threshold);
    if (err != 0)
    {
        printf("preCombineOnAlpha: Alpha mask creation error!\n");
        return -4;
    }
    err = preCombine(img1, img2, mask, result, threshold);
    if (err != 0)
    {
        printf("preCombineOnAlpha: Combine error!\n");
        return -5;
    }
    freeMask(mask);
    return 0;
}
int preCombine(Image* img1, Image* img2, Mask* mask, Image** result, int threshold)
{
    if (img1 == NULL || img2 == NULL || mask == NULL)
    {
        printf("preCombineOnAlpha: One of the images is null!\n");
        return -1;
    }
    *result = (Image*)malloc(sizeof(Image));
    if (*result == NULL)
    {
        printf("Memory allocation failed!\n");
        return -2;
    }
    (*result)->height = img1->height;
    (*result)->width = img1->width;
    (*result)->size = img1->size;
    (*result)->pixelData = (Pixel*)malloc(sizeof(Pixel)*(*result)->size);
    (*result)->channels = fmax(img1->channels, img2->channels);
    

    for (int i = 0; i < img1->size; i++)
    {
        int img2I = i % img2->size;
        int maskI = i % mask->size;

        float maskValue = mask->array[maskI];
        if (threshold > 0 && threshold < RGB_MAX)
        {
            maskValue = maskValue < threshold ? 0 : RGB_MAX;
        }
        maskValue /= (float)RGB_MAX;
        Pixel pixel1 = img1->pixelData[i];
        Pixel pixel2 = img2->pixelData[img2I];

        (*result)->pixelData[i].R = (unsigned char)round(pixel1.R * maskValue * (pixel1.A / (float)RGB_MAX) + pixel2.R * (1 - maskValue) * (pixel2.A / (float)RGB_MAX));
        (*result)->pixelData[i].G = (unsigned char)round(pixel1.G * maskValue + pixel2.G * (1 - maskValue));
        (*result)->pixelData[i].B = (unsigned char)round(pixel1.B * maskValue + pixel2.B * (1 - maskValue));
        if ((*result)->channels == 3)
        {
            (*result)->pixelData[i].A = 1;
        }
        else
        {
            // int alpha = pixel1.A + pixel2.A;
            int alpha = pixel1.A * maskValue + pixel2.A * (1 - maskValue);
            (*result)->pixelData[i].A = alpha > RGB_MAX ? RGB_MAX : alpha;
        }
    }
    
    return 0;
}
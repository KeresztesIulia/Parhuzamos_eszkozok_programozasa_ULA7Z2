#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "color_management.h"

#define RGB_MAX 255

int ToGrayscale(Pixel* original, Pixel* grayscale, int size)
{
    if (original == NULL || grayscale == NULL){
        printf("ToGrayscale: one of the pixeldata is null!");
        return -1;
    }
    for (int i = 0; i < size; i++)
    {
        int err = Luminance(&original[i], &grayscale[i]);
        if (err != 0){
            printf("Luminance calculation error!");
            return -2;
        }
    }
    return 0;
}
int ToBlackAndWhite(Pixel* original, Pixel* blackandwhite, int size, int threshold)
{
    if (original == NULL || blackandwhite == NULL){
        printf("ToBlackAndWhite: one of the pixeldata is null!");
        return -1;
    }
    int err = ToGrayscale(original, blackandwhite, size);
    if (err != 0)
    {
        return err;
    }
    for (int i = 0; i < size; i++)
    {
        if (blackandwhite[i].R < threshold)
        {
            blackandwhite[i].R = 0;
            blackandwhite[i].G = 0;
            blackandwhite[i].B = 0;
        }
        else
        {
            blackandwhite[i].R = RGB_MAX;
            blackandwhite[i].G = RGB_MAX;
            blackandwhite[i].B = RGB_MAX;
        }
        blackandwhite[i].A = RGB_MAX;
    }
    return 0;
}
int AlphaToGreyscale(Pixel* original, Pixel* grayscale, int size, int threshold)
{
    if (original == NULL || grayscale == NULL){
        printf("AlphaToGreyscale: one of the pixel data is null!\n");
        return -1;
    }
    for (int i = 0; i < size; i++)
    {
        int value;
        if (threshold > 0 && threshold <= RGB_MAX)
        {
            value = original[i].A < threshold ? 0 : RGB_MAX;
        }
        else
        {
            value = original[i].A;
        }
        grayscale[i].R = value;
        grayscale[i].G = value;
        grayscale[i].B = value;
        grayscale[i].A = RGB_MAX;
    }
    return 0;
}


double LinearRGB(unsigned char value)
{
    double linearValue;
    linearValue = value / (double)RGB_MAX;
    if (linearValue <= 0.04045) linearValue = linearValue / 12.92;
    else linearValue = pow((linearValue + 0.055)/1.055, 2.4);
    return linearValue;
}
int Luminance(Pixel* original, Pixel* grayed)
{
    if (original == NULL || grayed == NULL){
        printf("Luminance: pixelData is null");
        return -2;
    }
    unsigned char luminance = (0.2126 * LinearRGB(original->R) + 0.7152 * LinearRGB(original->G) + 0.0722 * LinearRGB(original->B)) * RGB_MAX;
    grayed->R = luminance;
    grayed->G = luminance;
    grayed->B = luminance;
    grayed->A = RGB_MAX;
    return 0;
}

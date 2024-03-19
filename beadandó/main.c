#include <stdio.h>
#include <stdlib.h>

#include "load_images.h"
#include "color_management.h"
#include "combine_images.h"

int main(void)
{
    char* img1Path = "imgs/resources/bothpng1.png";
    char* img2Path = "imgs/resources/bothpng2.png";
    char* maskPath = "imgs/resources/nonsharp_mask.jpg";
    char* savePath = "imgs/combined/bothpng_test.png";

    Image* result = (Image*)malloc(sizeof(Image));
    int err = CombineOnColorMask(img1Path, img2Path, maskPath, &result, 0);
    if (err != 0)
    {
        printf("Combine error!\n");
        return -1;
    }

    err = SaveImage(savePath, result);
    if (err != 0)
    {
        printf("Save error!\n");
        return -2;
    }
    return 0;
}


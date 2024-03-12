#include <stdio.h>
#include <stdlib.h>

#include "load_images.h"
#include "color_management.h"

int main(void)
{
    //printf("%d", argc);
    char* loadPath = "imgs/resources/sky.png";
    char* saveOrig = "imgs/converted/sky_copy.png";
    char* saveAlphaMask = "imgs/converted/sky_alphamask.jpg";
    char* saveColorMask = "imgs/converted/sky_colormask.jpg";
    int err;
    Image* original = (Image*)malloc(sizeof(Image));
    Image* alphamask = (Image*)malloc(sizeof(Image));
    Image* colormask = (Image*)malloc(sizeof(Image));
    if (original == NULL){
        printf("Mask allocation failed!\n");
        return -1;
    }
    if (alphamask == NULL){
        printf("Mask allocation failed!\n");
        return -1;
    }
    if (colormask == NULL){
        printf("Mask allocation failed!\n");
        return -1;
    }

    err = LoadAsImage(loadPath, original);
    if (err != 0){
        printf("imageload error");
        return -1;
    }
    err = LoadAsAlphaMask(loadPath, alphamask, 200);
    if (err != 0){
        printf("alphamask error");
        return -2;
    }
    err = LoadAsColorMask(loadPath, colormask, 200);
    if (err != 0){
        printf("colormask error");
        return -3;
    }

    SaveImage(saveOrig, original);
    SaveImage(saveAlphaMask, alphamask);
    SaveImage(saveColorMask, colormask);
    free(original);
    free(alphamask);
    free(colormask);  
    return 0;
}


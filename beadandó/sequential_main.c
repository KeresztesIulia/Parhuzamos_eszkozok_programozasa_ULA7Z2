#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "time_info.h"
#include "load_images.h"
#include "color_management.h"
#include "combine_images.h"

#define COMBINE_METHOD "colormask" // alpha, alphamask, colormask
#define THRESHOLD -1

int main(void)
{
    const char* IMG1PATH = "imgs/resources/final1.png";
    const char* IMG2PATH = "imgs/resources/final2.png";
    const char* MASKPATH = "imgs/resources/final_mask.png";
    const char* SAVEPATH = "imgs/combined/combineSEQ.png";

    char* timesPath = "times/seq.txt";
    TimeInfo* timeInfos = (TimeInfo*)malloc(sizeof(TimeInfo));

    Image* result = (Image*)malloc(sizeof(Image));

    int err;
    if (strcmp(COMBINE_METHOD, "alpha") == 0)
    {
        err = CombineOnAlpha(IMG1PATH, IMG2PATH, &result, THRESHOLD, timeInfos);
        if (err != 0)
        {
            printf("Combine error!\n");
            return -1;
        }
    }
    else if (strcmp(COMBINE_METHOD, "alphamask") == 0)
    {
        err = CombineOnAlphaMask(IMG1PATH, IMG2PATH, MASKPATH, &result, THRESHOLD, timeInfos);
        if (err != 0)
        {
            printf("Combine error!\n");
            return -1;
        }
    }
    else
    {
        err = CombineOnColorMask(IMG1PATH, IMG2PATH, MASKPATH, &result, THRESHOLD, timeInfos);
        if (err != 0)
        {
            printf("Combine error!\n");
            return -1;
        }
    }
    
    err = SaveImage(SAVEPATH, result);
    if (err != 0)
    {
        printf("Save error!\n");
        return -2;
    }
    double maskTime = (double)(timeInfos->maskEnd - timeInfos->maskStart);
    double combineTime = (double)(timeInfos->combineEnd - timeInfos->combineStart);
    
    FILE* timeFile = fopen(timesPath, "a");
    fprintf(timeFile, "[%dx%d]\t|\t%s\t|\t%.5lf ms\t\t|\t\t%.5lf ms\n", result->width, result->height, COMBINE_METHOD, maskTime, combineTime);
    fclose(timeFile);
    free(timeInfos);
    freeImage(result);
    return 0;
}



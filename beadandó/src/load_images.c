#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "load_images.h"
#include "color_management.h"


int ImageToPixelData(unsigned char* originalImageData, Pixel* pixelData, int size, int channels)
{
    if (pixelData == NULL){
        printf("ImageToPixelData: pixelData is not allocated!");
        return -1;
    }
    int j = 0;
    for (int i = 0; i < size * channels; i+=channels, j++)
    {
        Pixel pixel;
        pixel.R = originalImageData[i];
        pixel.G = originalImageData[i+1];
        pixel.B = originalImageData[i+2];
        if (channels == 4) pixel.A = originalImageData[i+3];
        else pixel.A = 1;
        pixelData[j] = pixel;
    }
    return 0;
}
int LoadAndConvert(char* path, Pixel** pixelData, int* width, int* height, int* channels)
{
    unsigned char *img = stbi_load(path, width, height, channels, 0);
    if(img == NULL) {
        printf("Error in loading the image\n");
        return -1;
    }
    free(*pixelData);
    *pixelData = (Pixel*)malloc(*width * *height * sizeof(Pixel));
    if (*pixelData == NULL){
        printf("Memory allocation failed!");
        return -2;
    }
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", *width, *height, *channels);
    int err = ImageToPixelData(img, *pixelData, *width * *height, *channels);
    if (err != 0){
        printf("conversion error!");
        return -3;
    }
    free(img);
    return 0;
}
int PixelDataToArray(Pixel* pixelData, unsigned char* convertedImage, int size, int channels)
{
    for (int i = 0, j = 0; j < size; i += channels, j++)
    {
        convertedImage[i] = pixelData[j].R;
        convertedImage[i+1] = pixelData[j].G;
        convertedImage[i+2] = pixelData[j].B;
        if (channels == 4) convertedImage[i+3] = pixelData[j].A;
    }
    return 0;
}

int LoadAsColorMask(char* path, Image* image, int threshold)
{
    int err = LoadAndConvert(path, &image->pixelData, &image->width, &image->height, &image->channels);
    if (image->pixelData == NULL){
        printf("PixelData is null!");
        return -1;
    }

    image->size =  image->width * image->height;

    if (threshold > 0 && threshold <= 255)
    {
        err = ToBlackAndWhite(image->pixelData, image->pixelData, image->size, threshold);
        if (err != 0){
            printf("Black and white conversion error!\n");
            return err;
        }
    }
    else
    {
        err = ToGrayscale(image->pixelData, image->pixelData, image->size);
        if (err != 0){
            printf("Greyscale conversion error!\n");
            return err;
        }
    }
    return 0;
}
int LoadAsAlphaMask(char* path, Image* image, int threshold)
{
    int err = LoadAndConvert(path, &image->pixelData, &image->width, &image->height, &image->channels);
    if (image->pixelData == NULL){
        printf("PixelData is null!");
        return -1;
    }
    if (image->channels != 4)
    {
        printf("Can't load non-transparent image as alpha mask!\n");
        return -2;
    }

    image->size =  image->width * image->height;

    err = AlphaToGreyscale(image->pixelData, image->pixelData, image->size, threshold);
    if (err != 0)
    {
        return -3;
    }

    return 0;
}
int LoadAsImage(char* path, Image* image)
{
    int err = LoadAndConvert(path, &image->pixelData, &image->width, &image->height, &image->channels);
    if (image->pixelData == NULL){
        printf("PixelData is null!");
        return -1;
    }

    image->size =  image->width * image->height;

    return 0;
}


int SaveImage(char* path, Image* image)
{
    unsigned char* endImage = (unsigned char*)malloc(image->size * image->channels);
    if (endImage == NULL){
        free(endImage);
        printf("Memory allocation failed!");
        return -1;
    }
    int err = PixelDataToArray(image->pixelData, endImage, image->size, image->channels);
    if (err != 0)
    {
        printf("Save: Error converting to array!\n");
        return -2;
    }

    stbi_write_png(path, image->width, image->height, image->channels, endImage, image->width * image->channels);
    printf("Image exported!\n");

    free(endImage); 
}
int CopyImage(Image* original, Image* copy)
{ 
    int err = CopyPixelData(original->pixelData, copy->pixelData, original->size);
    if (err != 0)
    {
        printf("Error copying image!\n");
        return -1;
    }
    copy->width = original->width;
    copy->height = original->height;
    copy->channels = original->channels;
    copy->size = original->size;  

    return 0;  
}
int CopyPixelData(Pixel* original, Pixel** copy, int size)
{
    *copy = (Pixel*)malloc(size * sizeof(Pixel));
    if (*copy == NULL)
    {
        printf("Unable to allocate for copied pixel data!\n");
        return -1;
    }
    for (int i = 0; i < size; i++)
    {
        (*copy)[i].R = original[i].R;
        (*copy)[i].G = original[i].G;
        (*copy)[i].B = original[i].B;
        (*copy)[i].A = original[i].A;
    }
    return 0;
}
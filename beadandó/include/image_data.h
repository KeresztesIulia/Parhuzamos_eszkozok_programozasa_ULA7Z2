#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

typedef struct Pixel{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char A;
} Pixel;

typedef struct Image{
    Pixel* pixelData;
    int width;
    int height;
    int size;
    int channels;
} Image;

typedef struct Mask{
    unsigned char* array;
    int width;
    int height;
    int size;
} Mask;

typedef struct ImageInfo{
    int width;
    int height;
    int size;
    int channels;
} ImageInfo;

#endif
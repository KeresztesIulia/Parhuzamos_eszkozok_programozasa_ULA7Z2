#include <stdio.h>
#include <stdlib.h>

#include "stb_image.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main()
{
    FILE *file = fopen("imgs/bin", "rb");
    int size = 3072000;
    unsigned char* img = (unsigned char*)malloc(size);
    fread(img, 1, size, file);
    printf("%d\n", img[0x270]);
    fclose(file);

    stbi_write_png("imgs/test.png", 1280, 800, 3, img, 1280*3);
    return 0;
}
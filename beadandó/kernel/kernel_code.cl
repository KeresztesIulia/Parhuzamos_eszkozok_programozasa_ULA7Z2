#pragma OPENCL EXTENSION cl_intel_printf : enable

typedef struct Pixel{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char A;
} Pixel;

typedef struct ImageInfo{
    int width;
    int height;
    int size;
    int channels;
} ImageInfo;

#define RGB_MAX 255

// ------ GRAYSCALE -------
__kernel void ToGrayscale(__global Pixel* original, __global unsigned char* grayscale, int size, int n, __global int* err);
__kernel void ToBlackAndWhite(__global Pixel* original, __global unsigned char* blackandwhite, int threshold, int size, int n, __global int* err);
__kernel void AlphaToGreyscale(__global Pixel* original, __global unsigned char* grayscale, int threshold, int size, int n, __global int* err);
int Luminance(__global Pixel* original, __global unsigned char* grayed);

float LinearRGB(unsigned char value);

// ------- COMBINE --------
__kernel void Combine(__global Pixel* img1, __global Pixel* img2, __global unsigned char* mask, __global Pixel* result, __global ImageInfo* img1Info, __global ImageInfo* img2Info, __global ImageInfo* maskInfo, __global ImageInfo* resultInfo, int threshold, int n, __global int* err);




// ------ GRAYSCALE -------
__kernel void ToGrayscale(__global Pixel* original, __global unsigned char* grayscale, int size, int n, __global int* err)
{
    if (original == NULL || grayscale == NULL){
        printf("ToGrayscale: one of the arrays is null!");
        *err--;
        return;
    }

    int interval = size / n;
    int from, to;
    int id = get_global_id(0);
    if (id < n-1)
    {
        from = id * interval;
        to = (id + 1) * interval;
    }
    else if (id == n-1)
    {
        from = id * interval;
        to = size;
    }
    else
    {
        return;
    }

    for (int i = from; i < to; i++)
    {
        *err += Luminance(&original[i], &grayscale[i]);
        if (*err != 0){
            printf("Luminance calculation error!");
            return;
        }
    }
    return;
}

//__kernel void ToBlackAndWhite(__global Pixel* original, __global unsigned char* blackandwhite/*, int threshold*/, int size, /*int n,*/ __global int* err)
__kernel void ToBlackAndWhite(__global Pixel* original, __global unsigned char* blackandwhite, int threshold, int size, int n, __global int* err)
{
    if (original == NULL || blackandwhite == NULL){
        printf("ToBlackAndWhite: one of the arrays is null!");
        *err--;
        return;
    }

    int interval = size / n;
    int from, to;
    int id = get_global_id(0);
    if (id < n-1)
    {
        from = id * interval;
        to = (id + 1) * interval;
    }
    else if (id == n-1)
    {
        from = id * interval;
        to = size;
    }
    else
    {
        return;
    }
    ToGrayscale(original, blackandwhite, size, n, err);
    if (*err != 0)
    {
        return;
    }
    if (threshold > 0)
    {
        for (int i = from; i < to; i++)
        {
            if (blackandwhite[i] < threshold)
            {
                blackandwhite[i] = 0;
            }
            else
            {
                blackandwhite[i] = RGB_MAX;
            }
        }
    }
    
    return;
}
__kernel void AlphaToGreyscale(__global Pixel* original, __global unsigned char* grayscale, int threshold, int size, int n, __global int* err)
{
    if (original == NULL || grayscale == NULL){
        printf("AlphaToGreyscale: one of the arrays is null!\n");
        *err--;
        return;
    }

    int interval = size / n;
    int from, to;
    int id = get_global_id(0);
    if (id < n-1)
    {
        from = id * interval;
        to = (id + 1) * interval;
    }
    else if (id == n-1)
    {
        from = id * interval;
        to = size;
    }
    else
    {
        return;
    }

    for (int i = from; i < to; i++)
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
        grayscale[i] = value;
    }
    return;
}
int Luminance(__global Pixel* original, __global unsigned char* grayed)
{
    if (original == NULL || grayed == NULL){
        printf("Luminance: pixelData is null");
        return -2;
    }
    unsigned char luminance = (0.2126f * LinearRGB(original->R) + 0.7152f * LinearRGB(original->G) + 0.0722f * LinearRGB(original->B)) * RGB_MAX;
    *grayed = luminance;
    return 0;
}

float LinearRGB(unsigned char value)
{
    float linearValue;
    linearValue = value / (float)RGB_MAX;
    if (linearValue <= 0.04045f) linearValue = linearValue / 12.92f;
    else linearValue = pow((linearValue + 0.055f)/1.055f, 2.4f);
    return linearValue;
}


// ------- COMBINE -------- 
__kernel void Combine(__global Pixel* img1, __global Pixel* img2, __global unsigned char* mask, __global Pixel* result, __global ImageInfo* img1Info, __global ImageInfo* img2Info, __global ImageInfo* maskInfo, __global ImageInfo* resultInfo, int threshold, int n, __global int* err)
{
    if (img1 == NULL || img2 == NULL || mask == NULL)
    {
        printf("preCombineOnAlpha: One of the images is null!\n");
        err--;
        return;
    }
    if (result == NULL)
    {
        printf("Result memory has to be preallocated!\n");
        err--;
        return;
    }

    int interval = resultInfo->size / n;
    int from, to;
    int id = get_global_id(0);
    

    if (id < n-1)
    {
        from = id * interval;
        to = (id + 1) * interval;
    }
    else if (id == n-1)
    {
        from = id * interval;
        to = resultInfo->size;
    }
    else
    {
        return;
    }


    for (int i = from; i < to; i++)
    {
        int img2I = i % img2Info->size;
        int maskI = i % maskInfo->size;

        float maskValue = mask[maskI];

        if (threshold > 0 && threshold < RGB_MAX)
        {
            maskValue = maskValue < threshold ? 0 : RGB_MAX;
        }
        maskValue /= (float)RGB_MAX;
        Pixel pixel1 = img1[i];
        Pixel pixel2 = img2[img2I];

        result[i].R = (unsigned char)round(pixel1.R * maskValue * (pixel1.A / (float)RGB_MAX) + pixel2.R * (1 - maskValue) * (pixel2.A / (float)RGB_MAX));
        result[i].G = (unsigned char)round(pixel1.G * maskValue + pixel2.G * (1 - maskValue));
        result[i].B = (unsigned char)round(pixel1.B * maskValue + pixel2.B * (1 - maskValue));
        if (resultInfo->channels == 3)
        {
            result[i].A = 1;
        }
        else
        {
            int alpha = pixel1.A * maskValue + pixel2.A * (1 - maskValue);
            result[i].A = alpha > RGB_MAX ? RGB_MAX : alpha;
        }
    }
    return;
}


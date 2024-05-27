#include "kernel_loader.h"
#include "load_images.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

#include "time_info_CL.h"


int LoadImages(Image* img1, Image* img2, Image* maskImage, Mask* mask, Image* result);
int MaskKernel(char* type, Image* maskImage, Mask* mask, cl_program program, cl_context context, cl_device_id device_id, int mask_binary_size, int maskImage_binary_size, TimeInfo* timeInfo);
int CombineKernel(Image* img1, Image* img2, Mask* mask, Image* result, cl_program program, cl_context context, cl_device_id device_id, int img1_binary_size, int img2_binary_size, int mask_binary_size, int result_binary_size, TimeInfo* timeInfo);

const char* IMG1PATH = "imgs/resources/final1.png";
const char* IMG2PATH = "imgs/resources/final2.png";
const char* MASKPATH = "imgs/resources/final_mask.png";
const char* SAVEPATH_MASK = "imgs/combined/maskCL.png";
const char* SAVEPATH_IMAGE = "imgs/combined/combineCL.png";

const char* TIMESPATH = "times/openCL.txt";

const int THRESHOLD = -1;
char* COMBINE_METHOD = "colormask"; // alpha, alphamask, colormask

const int WORK_GROUPS = 4;

int main(void)
{
// ------------------ PREP DATA -------------------------------------------------------
    Image* img1 = (Image*)malloc(sizeof(Image));
    Image* img2 = (Image*)malloc(sizeof(Image));
    Image* maskImage = (Image*)malloc(sizeof(Image));
    Mask* mask = (Mask*)malloc(sizeof(Mask));
    Image* result = (Image*)malloc(sizeof(Image)); // host buffer
    if (img1 == NULL || img2 == NULL || maskImage == NULL || mask == NULL || result == NULL)
    {
        printf("Allocation failed!\n"); 
        return -1;
    }
    int error = LoadImages(img1, img2, maskImage, mask, result);
    if (error != 0)
    {
        printf("Load error!\n");
        return error;
    }
    printf("Finished loading images!\n");

    int maskImage_binary_size = maskImage->size * sizeof(Pixel);
    int mask_binary_size = mask->size * sizeof(unsigned char);
    int img1_binary_size = img1->size * sizeof(Pixel);
    int img2_binary_size = img2->size * sizeof(Pixel);
    int result_binary_size = result->size * sizeof(Pixel);

// ----------------------- PREP OPENCL STUFF -------------------------------------------
    cl_int err;
    int error_code;

    // Get platform
    cl_uint n_platforms;
	cl_platform_id platform_id;
    err = clGetPlatformIDs(1, &platform_id, &n_platforms);
	if (err != CL_SUCCESS) {
		printf("[ERROR] Error calling clGetPlatformIDs. Error code: %d\n", err);
		return 0;
	}

    // Get device
	cl_device_id device_id;
	cl_uint n_devices;
	err = clGetDeviceIDs(
		platform_id,
		CL_DEVICE_TYPE_GPU,
		1,
		&device_id,
		&n_devices
	);
	if (err != CL_SUCCESS) {
		printf("[ERROR] Error calling clGetDeviceIDs. Error code: %d\n", err);
		return 0;
	}
    printf("no. of devices: %u\n", n_devices);

    size_t size;
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, NULL, &size);
    char* name = (char*)malloc(sizeof(char) * (size + 1));
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, size + 1, name, &size);
    printf("selected device: %s\n", name);

    // Create OpenCL context
    cl_context context = clCreateContext(NULL, n_devices, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS){
        printf("context error\n");
        return 0;
    }

    // Build the program
    const char* kernel_code = load_kernel_source("kernel/kernel_code.cl", &error_code);
    if (error_code != 0) {
        printf("Source code loading error!\n");
        return 0;
    }

    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &err);
    if (err != CL_SUCCESS){
        printf("program creation error \n");
        return 0;
    }
    err = clBuildProgram(
        program,
        1,
        &device_id,
        NULL,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS) {
        printf("Build error! Code: %d\n", err);
        size_t real_size;
        err = clGetProgramBuildInfo(
            program,
            device_id,
            CL_PROGRAM_BUILD_LOG,
            0,
            NULL,
            &real_size
        );
        char* build_log = (char*)malloc(sizeof(char) * (real_size + 1));
        err = clGetProgramBuildInfo(
            program,
            device_id,
            CL_PROGRAM_BUILD_LOG,
            real_size + 1,
            build_log,
            &real_size
        );
        // build_log[real_size] = 0;
        printf("Real size : %d\n", real_size);
        printf("Build log : %s\n", build_log);
        free(build_log);
        return 0;
    }
    size_t sizes_param[10];
    size_t real_size;
    err = clGetProgramInfo(
        program,
        CL_PROGRAM_BINARY_SIZES,
        10,
        sizes_param,
        &real_size
    );
    printf("Real size   : %d\n", real_size);
    printf("Binary size : %d\n", sizes_param[0]);

// ------------------------------ KERNELS ----------------------------------------
    TimeInfo* timeInfos = (TimeInfo*)malloc(sizeof(TimeInfo));
    if (maskImage == NULL)
    {
        if (strcmp(IMG1PATH, MASKPATH) == 0) COMBINE_METHOD = "alpha";
        err = MaskKernel(COMBINE_METHOD, img1, mask, program, context, device_id, mask_binary_size, maskImage_binary_size, timeInfos);
    }
    else
    {
        err = MaskKernel(COMBINE_METHOD, maskImage, mask, program, context, device_id, mask_binary_size, maskImage_binary_size, timeInfos);
    }
    if (err != 0)
    {
        printf("Mask error!\n");
        return err;
    }

    //_sleep(5000);

    err = CombineKernel(img1, img2, mask, result, program, context, device_id, img1_binary_size, img2_binary_size, mask_binary_size, result_binary_size, timeInfos);
    if (err != 0)
    {
        printf("Combine error!\n");
        return err;
    }

// ------------------ SAVE TIMES -----------------------
    FILE* timeFile = fopen(TIMESPATH, "a");
    double maskTime = (double)((timeInfos->maskEnd - timeInfos->maskStart) * 1e-6);
    double combineTime = (double)((timeInfos->combineEnd - timeInfos->combineStart) * 1e-6);
    fprintf(timeFile, "[%dx%d]\t|\t%s\t|\t%.10lf ms\t\t|\t\t%.10lf ms\n", result->width, result->height, COMBINE_METHOD, maskTime, combineTime);
    fclose(timeFile);

// ------------------ END STUFF ------------------------
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseDevice(device_id);

    freeImage(img1);
    freeImage(img2);
    freeImage(maskImage);
    freeMask(mask);
    freeImage(result);

    printf("ended\n");
}

int LoadImages(Image* img1, Image* img2, Image* maskImage, Mask* mask, Image* result)
{
    int err = LoadAsImage(IMG1PATH, img1);
    if (err != 0)
    {
        printf("Failed to load images!\n");
        return err;
    }
    err = LoadAsImage(IMG2PATH, img2);
    if (err != 0)
    {
        printf("Failed to load images!\n");
        return err;
    }
    if (strlen(MASKPATH) == 0)
    {
        printf("LoadImages: No maskpath given! If the mask is the same as the first image, the path shouldn't be left empty, but copied!");
        return -5;
    }
    err = LoadAsImage(MASKPATH, maskImage);
    if (err != 0)
    {
        printf("Failed to load images!\n");
        return err;
    }
    printf("maskImage size: %d\n", maskImage->size);
    mask->width = maskImage->width;
    mask->height = maskImage->height;
    mask->size = maskImage->size;
    mask->array = (unsigned char*)malloc(mask->size * sizeof(unsigned char));
    
    result->width = img1->width;
    result->height = img1->height;
    result->size = img1->size;
    result->channels = img1->channels > img2->channels ? img1->channels : img2->channels;
    result->pixelData = (Pixel*)malloc(result->size * sizeof(Pixel));
    return 0;
}

int MaskKernel(char* type, Image* maskImage, Mask* mask, cl_program program, cl_context context, cl_device_id device_id, int mask_binary_size, int maskImage_binary_size, TimeInfo* timeInfo)
{
    char* maskKernelFunction = "";
    if (strcmp(type, "alpha") == 0 || strcmp(type, "alphamask") == 0)
    {
        maskKernelFunction = "AlphaToGreyscale";
    }
    else
    {
        maskKernelFunction = "ToBlackAndWhite";
    }
    if (maskImage == NULL)
    {
        printf("No mask image!\n");
        return -9;
    }
    cl_event mask_event;
    cl_int err;
    cl_kernel mask_kernel = clCreateKernel(program, maskKernelFunction, &err);
    if (err != CL_SUCCESS){
        printf("Kernel error\n");
        return -1;
    }

    // Create the device buffer
    cl_mem mask_data_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, mask_binary_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("MaskKernel: Couldn't allocate buffer for mask data! err: %d, size: %d\n", err, mask_binary_size);
        return -1;
    }
    cl_mem pixelData_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, maskImage_binary_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("MaskKernel: Couldn't allocate buffer for pixelData!\n");
        return -1;
    }
    printf("maskImage size: %d\n", maskImage->size);
    cl_mem errorb = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("MaskKernel: Couldn't allocate buffer for error buffer!\n");
        return -1;
    }

    // Set kernel arguments
    err = clSetKernelArg(mask_kernel, 0, sizeof(cl_mem), (void*)&pixelData_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg0 error!\n");
        return err;
    }
    err = clSetKernelArg(mask_kernel, 1, sizeof(cl_mem), (void*)&mask_data_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg1 error!\n");
        return err;
    }
    err = clSetKernelArg(mask_kernel, 2, sizeof(int), (void*)&THRESHOLD);
    if (err != CL_SUCCESS)
    {
        printf("arg3 error!\n");
        return err;
    }
    err = clSetKernelArg(mask_kernel, 3, sizeof(int), (void*)&(mask->size));
    if (err != CL_SUCCESS)
    {
        printf("arg3 error!\n");
        return err;
    }
    err = clSetKernelArg(mask_kernel, 4, sizeof(int), (void*)&WORK_GROUPS);
    if (err != CL_SUCCESS)
    {
        printf("arg4 error!\n");
        return err;
    }
    err = clSetKernelArg(mask_kernel, 5, sizeof(cl_mem), (void*)&errorb);
    if (err != CL_SUCCESS)
    {
        printf("arg5 error %d!\n", err);
        return err;
    }

    // Create the command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
    if (err != CL_SUCCESS)
    {
        printf("Command queue error!\n");
        return err;
    }

    printf("size: %d\n", maskImage->size * sizeof(Pixel));


    // Host buffer -> Device buffer (pixelData)
    err = clEnqueueWriteBuffer(
        command_queue,
        pixelData_buffer,
        CL_TRUE,
        0,
        maskImage_binary_size,
        maskImage->pixelData,
        0,
        NULL,
        NULL
    );

    if (err != CL_SUCCESS){
        printf("pixelData enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (mask)
    err = clEnqueueWriteBuffer(
        command_queue,
        mask_data_buffer,
        CL_FALSE,
        0,
        mask_binary_size,
        mask->array,
        0,
        NULL,
        NULL
    );

    if (err != CL_SUCCESS){
        printf("mask enqueueWrite error\n");
        return err;
    }

    int error = 0;
    // Host buffer -> Device buffer (error)
    err = clEnqueueWriteBuffer(
        command_queue,
        errorb,
        CL_FALSE,
        0,
        sizeof(int),
        &error,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("errb enqueueWrite error\n");
        return err;
    }

    // Size specification
    size_t local_work_size = 32;
    size_t global_work_size = local_work_size * WORK_GROUPS;

    // Apply the kernel on the range
    err = clEnqueueNDRangeKernel(
        command_queue,
        mask_kernel,
        1,
        NULL,
        &global_work_size,
        &local_work_size,
        0,
        NULL,
        &mask_event
    );
    if (err == CL_INVALID_KERNEL_ARGS) printf("invalid args ND\n");
    if (err != CL_SUCCESS){
        printf("enqueueND error %d\n", err);
        return err;
    }

    // Host buffer <- Device buffer (mask)
    err = clEnqueueReadBuffer(
        command_queue,
        mask_data_buffer,
        CL_TRUE,
        0,
        mask_binary_size,
        mask->array,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("enqueueRead error\n");
        return err;
    }

    // Host buffer <- Device buffer (error)
    err = clEnqueueReadBuffer(
        command_queue,
        errorb,
        CL_TRUE,
        0,
        sizeof(int),
        &error,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("enqueueRead error\n");
        return err;
    }
    clFinish(command_queue);

    err = clGetEventProfilingInfo(mask_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &timeInfo->maskStart, NULL);
    if (err != CL_SUCCESS)
    {
        printf("profiling info error (start)! %d\n", err);
        return err;
    }

    err = clGetEventProfilingInfo(mask_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &timeInfo->maskEnd, NULL);
    if (err != CL_SUCCESS)
    {
        printf("profiling info error (end)! %d\n", err);
        return err;
    }

    clReleaseMemObject(errorb);
    clReleaseMemObject(pixelData_buffer);
    clReleaseMemObject(mask_data_buffer);
    clReleaseKernel(mask_kernel);
    SaveMask(SAVEPATH_MASK, mask);
    return error;
}

int CombineKernel(Image* img1, Image* img2, Mask* mask, Image* result, cl_program program, cl_context context, cl_device_id device_id, int img1_binary_size, int img2_binary_size, int mask_binary_size, int result_binary_size, TimeInfo* timeInfo)
{
    
    if (img1 == NULL || img2 == NULL || mask == NULL || result == NULL)
    {
        printf("Image or mask not allocated!\n");
        return -9;
    }
    cl_event combine_event;
    cl_int err;
    cl_kernel combine_kernel = clCreateKernel(program, "Combine", &err);
    if (err != CL_SUCCESS){
        printf("Kernel error\n");
        return -1;
    }


    ImageInfo* img1Info = (ImageInfo*)malloc(sizeof(ImageInfo));
    img1Info->width = img1->width;
    img1Info->height = img1->height;
    img1Info->size = img1->size;
    img1Info->channels = img1->channels;

    ImageInfo* img2Info = (ImageInfo*)malloc(sizeof(ImageInfo));
    img2Info->width = img2->width;
    img2Info->height = img2->height;
    img2Info->size = img2->size;
    img2Info->channels = img2->channels;

    ImageInfo* maskInfo = (ImageInfo*)malloc(sizeof(ImageInfo));
    maskInfo->width = mask->width;
    maskInfo->height = mask->height;
    maskInfo->size = mask->size;

    ImageInfo* resultInfo = (ImageInfo*)malloc(sizeof(ImageInfo));
    resultInfo->width = result->width;
    resultInfo->height = result->height;
    resultInfo->size = result->size;
    resultInfo->channels = result->channels;


    // Create the device buffers for image and mask pixel data
    cl_mem img1_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, img1_binary_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for img1 data!\n");
        return -1;
    }
    cl_mem img2_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, img2_binary_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for img2 data! err: %d, size: %d\n", err, img2_binary_size);
        return -1;
    }
    cl_mem mask_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, mask_binary_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for mask data!\n");
        return -1;
    }
    cl_mem result_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, result_binary_size, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for img1 data!\n");
        return -1;
    }

    // Create the device buffers for image and mask info
    cl_mem img1Info_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(ImageInfo), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for img1 info!\n");
        return -1;
    }
    cl_mem img2Info_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(ImageInfo), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for img2 info!\n");
        return -1;
    }
    cl_mem maskInfo_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(ImageInfo), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for mask info!\n");
        return -1;
    }
    cl_mem resultInfo_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(ImageInfo), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("CombineKernel: Couldn't allocate buffer for img1 info!\n");
        return -1;
    }

    // Create the device buffer for the error code
    cl_mem errorb = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("Couldn't allocate buffer for the error code!\n");
        return -1;
    }

    printf("cl_mems created\n");
    // Set kernel arguments
    err = clSetKernelArg(combine_kernel, 0, sizeof(cl_mem), (void*)&img1_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg0 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 1, sizeof(cl_mem), (void*)&img2_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg1 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 2, sizeof(cl_mem), (void*)&mask_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg2 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 3, sizeof(cl_mem), (void*)&result_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg3 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 4, sizeof(cl_mem), (void*)&img1Info_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg4 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 5, sizeof(cl_mem), (void*)&img2Info_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg5 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 6, sizeof(cl_mem), (void*)&maskInfo_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg6 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 7, sizeof(cl_mem), (void*)&resultInfo_buffer);
    if (err != CL_SUCCESS)
    {
        printf("arg7 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 8, sizeof(int), (void*)&THRESHOLD);
    if (err != CL_SUCCESS)
    {
        printf("arg8 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 9, sizeof(int), (void*)&WORK_GROUPS);
    if (err != CL_SUCCESS)
    {
        printf("arg9 error!\n");
        return err;
    }
    err = clSetKernelArg(combine_kernel, 10, sizeof(cl_mem), (void*)&errorb);
    if (err != CL_SUCCESS)
    {
        printf("arg10 error %d!\n", err);
        return err;
    }
    printf("kernelargs done\n");

    // Create the command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
    if (err != CL_SUCCESS)
    {
        printf("(Combine) Command queue error (%d)\n", err);
        return err;
    }

    printf("command queue created\n");

    // Host buffer -> Device buffer (img1)
    err = clEnqueueWriteBuffer(
        command_queue,
        img1_buffer,
        CL_TRUE,
        0,
        img1_binary_size,
        img1->pixelData,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("img1 enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (img2)
    err = clEnqueueWriteBuffer(
        command_queue,
        img2_buffer,
        CL_TRUE,
        0,
        img2_binary_size,
        img2->pixelData,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("img2 enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (mask)
    err = clEnqueueWriteBuffer(
        command_queue,
        mask_buffer,
        CL_TRUE,
        0,
        mask_binary_size,
        mask->array,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("mask enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (result)
    err = clEnqueueWriteBuffer(
        command_queue,
        result_buffer,
        CL_TRUE,
        0,
        result_binary_size,
        result->pixelData,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("result enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (img1Info)
    err = clEnqueueWriteBuffer(
        command_queue,
        img1Info_buffer,
        CL_TRUE,
        0,
        sizeof(ImageInfo),
        img1Info,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("img1Info enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (img2Info)
    err = clEnqueueWriteBuffer(
        command_queue,
        img2Info_buffer,
        CL_TRUE,
        0,
        sizeof(ImageInfo),
        img2Info,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("img2Info enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (maskInfo)
    err = clEnqueueWriteBuffer(
        command_queue,
        maskInfo_buffer,
        CL_TRUE,
        0,
        sizeof(ImageInfo),
        maskInfo,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("maskInfo enqueueWrite error\n");
        return err;
    }

    // Host buffer -> Device buffer (resultInfo)
    err = clEnqueueWriteBuffer(
        command_queue,
        resultInfo_buffer,
        CL_TRUE,
        0,
        sizeof(ImageInfo),
        resultInfo,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("resultInfo enqueueWrite error\n");
        return err;
    }

    int error = 0;
    // Host buffer -> Device buffer (error)
    err = clEnqueueWriteBuffer(
        command_queue,
        errorb,
        CL_TRUE,
        0,
        sizeof(int),
        &error,
        0,
        NULL,
        NULL
    );

    if (err != CL_SUCCESS){
        printf("mask enqueueWrite error\n");
        return err;
    }
    printf("transferred host to device\n");


    // Size specification
    size_t global_work_size = 1024;
    size_t local_work_size = global_work_size / WORK_GROUPS;

    // Apply the kernel on the range
    err = clEnqueueNDRangeKernel(
        command_queue,
        combine_kernel,
        1,
        NULL,
        &global_work_size,
        &local_work_size,
        0,
        NULL,
        &combine_event
    );
    if (err != CL_SUCCESS){
        printf("enqueueND error\n");
        return err;
    }

    clFinish(command_queue);
    printf("finished kernels\n");

    // Host buffer <- Device buffer (result)
    err = clEnqueueReadBuffer(
        command_queue,
        result_buffer,
        CL_TRUE,
        0,
        result->size * sizeof(Pixel),
        result->pixelData,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("enqueueRead error res %d\n", err);
        return err;
    }

    // Host buffer <- Device buffer (error)
    err = clEnqueueReadBuffer(
        command_queue,
        errorb,
        CL_TRUE,
        0,
        sizeof(int),
        &error,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS){
        printf("enqueueRead error err %d\n", err);
        return err;
    }
    clFinish(command_queue);
    printf("read buffers to host\n");

   

    err = clGetEventProfilingInfo(combine_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &timeInfo->combineStart, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Event profiling info error (start)! %d\n", err);
        return err;
    }
    err = clGetEventProfilingInfo(combine_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &timeInfo->combineEnd, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Event profiling info error (end)! %d\n", err);
        return err;
    }

    clFinish(command_queue);

    
    clReleaseMemObject(img1_buffer);
    clReleaseMemObject(img2_buffer);
    clReleaseMemObject(mask_buffer);
    clReleaseMemObject(result_buffer);
    clReleaseMemObject(img1Info_buffer);
    clReleaseMemObject(img2Info_buffer);
    clReleaseMemObject(maskInfo_buffer);
    clReleaseMemObject(resultInfo_buffer);
    clReleaseMemObject(errorb);
    clReleaseKernel(combine_kernel);

 //_sleep(5000);
 
    printf("image1 size: %d, result size: %d\n", img1->size, result->size);
    err = SaveImage(SAVEPATH_IMAGE, result);
    if (err != 0)
    {
        printf("Failed to save result image!\n");
    }
    printf("saved\n");

    
    return error;
}
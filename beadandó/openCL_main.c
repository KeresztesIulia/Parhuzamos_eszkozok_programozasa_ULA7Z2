#include "kernel_loader.h"
#include "load_images.h"

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int LoadImages(Image* img1, Image* img2, Image* maskImage, Mask* mask, Image* result);
int MaskKernel(char* type, Image* maskImage, Mask* mask, cl_program program, cl_context context, cl_device_id device_id, int mask_binary_size);

const char* IMG1PATH = "imgs/resources/bothpng1.png";
const char* IMG2PATH = "imgs/resources/bothpng2.png";
const char* MASKPATH = "imgs/resources/swirly.jpg";
const char* SAVEPATH = "imgs/combined/bothpng_test_CL.png";

const int THRESHOLD = 128;

const int WORK_GROUPS = 32;

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

    int mask_binary_size = mask->size * sizeof(unsigned char);
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

    size_t size;
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, NULL, &size);
    char* name = (char*)malloc(sizeof(char) * (size + 1));
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, size + 1, name, &size);
    printf("device: %s\n", name);

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
    if (maskImage == NULL)
    {
        err = MaskKernel("",img1, mask, program, context, device_id, mask_binary_size);
    }
    else
    {
        err = MaskKernel("",maskImage, mask, program, context, device_id, mask_binary_size);
    }
    if (err != 0)
    {
        printf("Mask error!\n");
        return err;
    }


// ------------------ END STUFF ------------------------
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseDevice(device_id);

    freeImage(img1);
    freeImage(img2);
    freeImage(maskImage);
    freeMask(mask);
    freeImage(result);
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
        freeImage(maskImage);
        mask->width = img1->width;
        mask->height = img1->height;
        mask->size = img1->size;
        mask->array = (unsigned char*)malloc(mask->size * sizeof(unsigned char));
    }
    else
    {
        err = LoadAsImage(MASKPATH, maskImage);
        if (err != 0)
        {
            printf("Failed to load images!\n");
            return err;
        }
        mask->width = maskImage->width;
        mask->height = maskImage->height;
        mask->size = maskImage->size;
        mask->array = (unsigned char*)malloc(mask->size * sizeof(unsigned char));
    }
    result->width = img1->width;
    result->height = img1->height;
    result->size = img1->size;
    result->channels = img1->channels > img2->channels ? img1->channels : img2->channels;
    result->pixelData = (Pixel*)malloc(result->size * sizeof(Pixel));
    return 0;
}

int MaskKernel(char* type, Image* maskImage, Mask* mask, cl_program program, cl_context context, cl_device_id device_id, int mask_binary_size)
{
    char* maskKernelFunction = "";
    if (strcmp(type, "alpha") == 0)
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
        printf("Couldn't allocate buffer for mask data!\n");
        return -1;
    }
    cl_mem pixelData_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, maskImage->size * sizeof(Pixel), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("Couldn't allocate buffer for pixelData!\n");
        return -1;
    }
    cl_mem errorb = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("Couldn't allocate buffer for pixelData!\n");
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
    err = clSetKernelArg(mask_kernel, 3, sizeof(int), (void*)&mask->size);
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
    err = clSetKernelArg(mask_kernel, 5, sizeof(void*), (void*)&errorb);
    if (err != CL_SUCCESS)
    {
        printf("arg5 error %d!\n", err);
        return err;
    }

    // Create the command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, NULL, NULL);

    // Host buffer -> Device buffer (pixelData)
    err = clEnqueueWriteBuffer(
        command_queue,
        pixelData_buffer,
        CL_FALSE,
        0,
        maskImage->size * sizeof(Pixel),
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
        printf("mask enqueueWrite error\n");
        return err;
    }


    // Size specification
    size_t global_work_size = 1024;
    size_t local_work_size = global_work_size / WORK_GROUPS;

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
        NULL
    );
    if (err != CL_SUCCESS){
        printf("enqueueND error\n");
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
    printf("%d\n", mask->array[0]);

    clReleaseKernel(mask_kernel);
    clReleaseMemObject(mask_data_buffer);
    clReleaseMemObject(pixelData_buffer);
    SaveMask(SAVEPATH, mask);
    return error;
}
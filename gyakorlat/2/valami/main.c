#include "kernel_loader.h"

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>

const int VECTOR_DIMENSION = 1;

int main(void)
{
    int i;
    cl_int err;
    int error_code;

    cl_float8 vector[] = {(cl_float8){1, 2, 3, 4, 5, 6, 7, 8}};

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

    // Create OpenCL context
    cl_context context = clCreateContext(NULL, n_devices, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS){
        printf("context error");
        return 0;
    }

    // Build the program
    const char* kernel_code = load_kernel_source("kernels/kernel_code.cl", &error_code);
    if (error_code != 0) {
        printf("Source code loading error!\n");
        return 0;
    }

    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &err);
    if (err != CL_SUCCESS){
        printf("program creation error");
        return 0;
    }
    const char options[] = "-D MULTIPLIER=3";
    err = clBuildProgram(
        program,
        1,
        &device_id,
        options,
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
    cl_kernel kernel = clCreateKernel(program, "multiply_vectors", &err);
    if (err != CL_SUCCESS){
        printf("kernel error");
        return 0;
    }

    // Create the host buffer and initialize it
    cl_float8* host_buffer = (cl_float8*)malloc(VECTOR_DIMENSION * sizeof(cl_float8));
    for (i = 0; i < VECTOR_DIMENSION; ++i) {
        host_buffer[i] = (cl_float8){0,0,0,0,0,0,0,0};
    }

    // Create the device buffer
    cl_mem device_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, VECTOR_DIMENSION * sizeof(cl_float8), NULL, NULL);
    cl_mem vec = clCreateBuffer(context, CL_MEM_READ_WRITE, VECTOR_DIMENSION * sizeof(cl_float8), NULL, NULL);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&vec);
    clSetKernelArg(kernel, 2, sizeof(int), (void*)&VECTOR_DIMENSION);

    // Create the command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, NULL, NULL);

    // Host buffer -> Device buffer
    clEnqueueWriteBuffer(
        command_queue,
        device_buffer,
        CL_FALSE,
        0,
        VECTOR_DIMENSION * sizeof(cl_float8),
        host_buffer,
        0,
        NULL,
        &err
    );

    if (err != CL_SUCCESS){
        printf("enqueue error! Code: %d\n", err);
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
        build_log[real_size] = 0;
        printf("Real size : %d\n", real_size);
        printf("Build log : %s\n", build_log);
        free(build_log);
        //return 0;
    }

    // Size specification
    size_t local_work_size = 256;
    size_t n_work_groups = (VECTOR_DIMENSION + local_work_size + 1) / local_work_size;
    size_t global_work_size = n_work_groups * local_work_size;

    // Apply the kernel on the range
    clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        1,
        NULL,
        &global_work_size,
        &local_work_size,
        0,
        NULL,
        NULL
    );

    // Host buffer <- Device buffer
    clEnqueueReadBuffer(
        command_queue,
        device_buffer,
        CL_TRUE,
        0,
        VECTOR_DIMENSION * sizeof(cl_float8),
        host_buffer,
        0,
        NULL,
        NULL
    );

    for (i = 0; i < VECTOR_DIMENSION; ++i) {
        for (int j = 0; j < 8; j++){
            printf("[%d] = %f, ", i, host_buffer[i].s[j]);
        }
         
    }


    // Release the resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseDevice(device_id);

    free(host_buffer);
}

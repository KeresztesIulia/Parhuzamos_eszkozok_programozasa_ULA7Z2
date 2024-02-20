#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>

const char* kernel_code =
    "__kernel void add_vectors(__global float* buffer, __global float* vec1, __global float* vec2, int vector_dimension) {\n"
    "   if (get_global_id(0) < vector_dimension) {\n"
    "       id = get_global_id(0);\n"
	"		buffer[id] = vec1[id] + vec2[id];\n"
    "   }\n"
    "}\n"
;

/*
    "   if (get_global_id(0) < n) {\n"
    "       buffer[get_global_id(0)] = 11;\n"
    "   }\n"
*/

/*
    "   if (get_global_id(0) < n) {\n"
    "       buffer[get_global_id(0)] = get_global_id(0) * 10;\n"
    "   }\n"
*/

/*
    "   if (get_global_id(0) % 2 == 0) {\n"
    "       buffer[get_global_id(0)] = 11;\n"
    "   } else {\n"
    "       buffer[get_global_id(0)] = 22;\n"
    "   }\n"
*/

const int VECTOR_DIMENSION = 1000;

int main(void)
{
    int i;
    cl_int err;

	int vec1[VECTOR_DIMENSION];
	int vec2[VECTOR_DIMENSION];

	for (int j = 0; j < VECTOR_DIMENSION; j++){
		vec1[j] = j;
		vec2[j] = 1;
	}

printf("..s.s.s.");
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
    cl_context context = clCreateContext(NULL, n_devices, &device_id, NULL, NULL, NULL);

    // Build the program
	cl_int err;
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &err);
	if (err != CL_SUCCESS){
		printf("what");
		return 0;
	}
	else{
		printf("yay");
		return 0;
	}
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        //printf("Build error! Code: %d\n", err);
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
        return 0;
    }
    cl_kernel kernel = clCreateKernel(program, "add_vectors", NULL);

    // Create the host buffer and initialize it
    float* host_buffer = (float*)malloc(VECTOR_DIMENSION * sizeof(float));
    for (i = 0; i < VECTOR_DIMENSION; ++i) {
        host_buffer[i] = i;
    }

    // Create the device buffer
    cl_mem device_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, VECTOR_DIMENSION * sizeof(float), NULL, NULL);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer);
	clSetKernelArg(kernel, 1, sizeof(float*), (void*)vec1);
	clSetKernelArg(kernel, 2, sizeof(float*), (void*)vec2);
    clSetKernelArg(kernel, 3, sizeof(int), (void*)&VECTOR_DIMENSION);

    // Create the command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, NULL, NULL);

    // Host buffer -> Device buffer
    clEnqueueWriteBuffer(
        command_queue,
        device_buffer,
        CL_FALSE,
        0,
        VECTOR_DIMENSION * sizeof(float),
        host_buffer,
        0,
        NULL,
        NULL
    );

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
        VECTOR_DIMENSION * sizeof(float),
        host_buffer,
        0,
        NULL,
        NULL
    );

    for (i = 0; i < VECTOR_DIMENSION; ++i) {
        printf("[%d] = %d, ", i, host_buffer[i]);
    }

    // Release the resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseDevice(device_id);

    free(host_buffer);
}

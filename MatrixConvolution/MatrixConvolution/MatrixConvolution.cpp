// MatrixConvolution.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <math.h>

#include <CL/cl.h>

#define K_ROWS 3
#define K_COLS 3
/*#define O_ROWS 3
#define O_COLS 3*/
#define O_ROWS 10
#define O_COLS 10

using namespace std;

#pragma comment(lib, "OpenCL.lib")

char* getDescriptionOfError(cl_int error) {
	switch (error) {
	case CL_SUCCESS:                            return "Success!";
	case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
	case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
	case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
	case CL_OUT_OF_RESOURCES:                   return "Out of resources";
	case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
	case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
	case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
	case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
	case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
	case CL_MAP_FAILURE:                        return "Map failure";
	case CL_INVALID_VALUE:                      return "Invalid value";
	case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
	case CL_INVALID_PLATFORM:                   return "Invalid platform";
	case CL_INVALID_DEVICE:                     return "Invalid device";
	case CL_INVALID_CONTEXT:                    return "Invalid context";
	case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
	case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
	case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
	case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
	case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
	case CL_INVALID_SAMPLER:                    return "Invalid sampler";
	case CL_INVALID_BINARY:                     return "Invalid binary";
	case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
	case CL_INVALID_PROGRAM:                    return "Invalid program";
	case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
	case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
	case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
	case CL_INVALID_KERNEL:                     return "Invalid kernel";
	case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
	case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
	case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
	case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
	case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
	case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
	case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
	case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
	case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
	case CL_INVALID_EVENT:                      return "Invalid event";
	case CL_INVALID_OPERATION:                  return "Invalid operation";
	case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
	case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
	case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
	default: return "Unknown";
	}
}

/**
* The matrixes will behave as a single dimension array.
*/
const char *KernelSource = "\n" \
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n" \
"__kernel void matrixConvolution(                                                  \n" \
"   __global double* matrix1,                                              \n" \
"   __global double* kernel_matrix,                                              \n" \
"   __global double* output,                                               \n" \
"   const unsigned int ROWS_M1,                                               \n" \
"   const unsigned int COLS_M1,                                               \n" \
"   const unsigned int ROWS_K,                                               \n" \
"   const unsigned int COLS_K) {                                             \n" \
"                                                                          \n" \
"   int id_row = get_global_id(0);                                         \n" \
"   int id_col = get_global_id(1);                                         \n" \
"   int center_rows = ROWS_K/2; // center of the filter                                         \n" \
"   int center_cols = COLS_K/2; // center of the filter                                        \n" \
"   int max_offset = ROWS_K*COLS_K;                                       \n"\
"   int kernel_offset = 0;                                       \n"\
"   int row_offset = 0;                                       \n"\
"   int col_offset = 0;                                       \n"\
"   int matrix_offset = (id_row*COLS_M1)+id_col;                                       \n"\
"   double acc = 0.0;                                                      \n" \
"                                                                          \n" \
"                                                                          \n" \
"   for(int i = 0; i<ROWS_K;i++){                                                                       \n" \
"       for(int j =0; j<COLS_K;j++){                                                                   \n" \
"                                                                          \n" \
"          kernel_offset = (i*COLS_K )+j;									\n" \
"          row_offset = id_row+(i-center_rows);                                                                 \n" \
"          col_offset = id_col+(j-center_cols);                                                                 \n" \
"                                                                          \n" \
"          if(                                                                \n" \
"          (row_offset >=0 && row_offset < ROWS_M1) &&                                                                \n" \
"          (col_offset >=0 && col_offset < COLS_M1)                                                                \n" \
"          ){                                                                \n" \
"              acc+=kernel_matrix[kernel_offset]*matrix1[(row_offset*COLS_M1)+col_offset];    \n" \
"          }                                                                \n" \
"       }                                                                   \n" \
"   }                                                                       \n" \
"   output[matrix_offset] = acc;                                                                       \n" \
"}                                                                         \n" \
"\n";



bool useGPU = true;
//cl_uint usePlatform = 1;
cl_uint usePlatform = 0;
cl_uint useDevice = 1;


/* DEBUGGING PURPOSES */
void printMatrix(cl_double** matrix, int rows, int cols) {
	string str_matrix = "";
	std::ostringstream strs;
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			strs << "[";
			double number = matrix[i][j];
			strs << number;
			strs << "] ";
		}
		strs << "\n";
	}

	cout << strs.str();
}

cl_double** convolute(cl_double** src, cl_double** kernel) {
	cl_double** result = new cl_double*[O_ROWS];
	for (size_t i = 0; i < O_ROWS; i++) {
		result[i] = new cl_double[O_COLS];
	}
	//cl_double** result = (cl_double**) malloc(sizeof(cl_double)*O_ROWS*O_COLS);
	int current_row;
	int current_col;
	cl_double acc = 0.0;
	for (size_t i = 0; i < O_ROWS; i++) {
		for (size_t j = 0; j < O_COLS; j++) {
			current_row = i - 1;
			current_col = j - 1;
			acc = 0.0;
			for (size_t KI = 0; KI < K_ROWS; KI++) {
				for (size_t KJ = 0; KJ < K_COLS; KJ++) {
					int c = current_row + KI;
					int d = current_col + KJ;
					if ((c >= 0 && c<O_ROWS)&&(d >= 0 && d<O_COLS)) {
						acc += src[c][d] * kernel[KI][KJ];
					}
				}
			}
			result[i][j] = acc;
		}
	}

	return result;
}

int main(void) {
	cl_int error;

	// Matrix declaration.
	cl_double result[O_ROWS][O_COLS];
	cl_double** src_matrix = new cl_double*[O_ROWS];

	//cl_double SRC[O_ROWS][O_COLS] = { 1,2,3,4,5,6,7,8,9 };
	cl_double SRC[O_ROWS][O_COLS];// = { 1,2,3,4,5,6,7,8,9 };
	for (size_t i = 0; i < O_ROWS; i++) {
		for (size_t j = 0; j < O_COLS; j++) {
			SRC[i][j] = rand() / (float)RAND_MAX;
		}
	}

	for (size_t i = 0; i < O_ROWS; i++) {
		src_matrix[i] = new cl_double[O_COLS];
		for (size_t j = 0; j < O_COLS; j++) {
			src_matrix[i][j] = SRC[i][j];
		}
	}

	

	cl_double** kernel_matrix = new cl_double*[K_ROWS];
	//cl_double km[K_ROWS][K_COLS] = { 2,2,2,2,2,2,2,2,2 };
	cl_double km[K_ROWS][K_COLS] = {
		(1 / (float)16), (1 / (float)8), (1 / (float)16),
		(1 / (float)8) , (1 / (float)4), (1 / (float)8),
		(1 / (float)16), (1 / (float)8), (1 / (float)16)
	};

	for (size_t i = 0; i < K_ROWS; i++) {
		kernel_matrix[i] = new cl_double[K_COLS];
		for (size_t j = 0; j < K_COLS; j++) {
			kernel_matrix[i][j] = km[i][j];
		}
	}
	
	cl_double** EXPECTED_RESULT = convolute(src_matrix, kernel_matrix);
	 
	//printMatrix(EXPECTED_RESULT, O_ROWS, O_COLS);

	unsigned int correct;
	// Amount of "correct" results that we'll have to find.
	unsigned int count = O_ROWS*O_COLS;

	size_t global;
	size_t local;

	cl_platform_id platform_id;
	cl_device_id device_id;             // compute device id
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_kernel kernel;                   // compute kernel

	cl_mem matrix1;                       // device memory used for the input array
	cl_mem matrix2;                       // device memory used for the input array
	cl_mem output;                      // device memory used for the output array

										
	

	// Platform
	cl_uint numPlatforms;
	error = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (error != CL_SUCCESS) {
		cout << "Error getting platform id: " << getDescriptionOfError(error) << endl;
		exit(error);
	}

	cl_platform_id* platforms_id = new cl_platform_id[numPlatforms];
	error = clGetPlatformIDs(numPlatforms, platforms_id, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error getting platform id: " << getDescriptionOfError(error) << endl;
		exit(error);
	}
	platform_id = platforms_id[usePlatform];

	// Connect to a compute device
	//
	cl_uint devices_n = 0;
	error = clGetDeviceIDs(platform_id, useGPU ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, useDevice, &device_id, &devices_n);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to create a device group: " << getDescriptionOfError(error) << endl;
		return EXIT_FAILURE;
	}

	// Create a compute context
	//
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &error);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to create a compute context: " << getDescriptionOfError(error) << endl;
		return EXIT_FAILURE;
	}

	// Create a command commands
	//
	commands = clCreateCommandQueue(context, device_id, 0, &error);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to create a command commands: " << getDescriptionOfError(error) << endl;
		return EXIT_FAILURE;
	}

	// Create the compute program from the source buffer
	//
	program = clCreateProgramWithSource(context, 1, (const char **)&KernelSource, NULL, &error);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to create compute program: " << getDescriptionOfError(error) << endl;
		return EXIT_FAILURE;
	}

	// Build the program executable
	//
	error = clBuildProgram(program, 0, NULL, "-cl-opt-disable", NULL, NULL);
	if (error != CL_SUCCESS) {
		size_t len;
		char buffer[2048];

		cout << "Error: Failed to build program executable: " << getDescriptionOfError(error) << endl;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		cout << buffer << endl;
		exit(1);
	}

	// Create the compute kernel in the program we wish to run
	//
	kernel = clCreateKernel(program, "matrixConvolution", &error);
	if (!kernel || error != CL_SUCCESS) {
		cout << "Error: Failed to create compute kernel: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Create the input and output arrays in device memory for our calculation
	//
	// The vectors to add.
	const unsigned int size_src = O_ROWS*O_COLS;
	const unsigned int size_kernel = K_ROWS*K_COLS;

	matrix1 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_double) * size_src, NULL, NULL);
	matrix2 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_double) * size_kernel, NULL, NULL);

	// Place where to store the obtained results 
	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_double) * size_src, NULL, NULL);
	if (!matrix1 || !matrix2 || !output) {
		cout << "Error: Failed to allocate device memory: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Write our data set into the input array in device memory
	//
	//cout << "test" << endl;
	printMatrix(src_matrix, O_ROWS, O_COLS);
	error = clEnqueueWriteBuffer(commands, matrix1, CL_TRUE, 0, sizeof(cl_double) * size_src, SRC, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to write to source array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}


	error = clEnqueueWriteBuffer(commands, matrix2, CL_TRUE, 0, sizeof(cl_double) * size_kernel, km, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to write to source array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// declaring into variables each value that we're going to need.
	unsigned int R_M1 = O_ROWS;
	unsigned int C_M1 = O_COLS;
	unsigned int R_M2 = K_ROWS;
	unsigned int C_M2 = K_COLS;

	// Set the arguments to our compute kernel
	//
	error = 0;
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &matrix1);    // matrix1
	error |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &matrix2);   // matrix 2
	error |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &output);    // output
	error |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &R_M1); // ROWS_SRC
	error |= clSetKernelArg(kernel, 4, sizeof(unsigned int), &C_M1); // COLS_SRC
	error |= clSetKernelArg(kernel, 5, sizeof(unsigned int), &R_M2); // ROWS_KERNEL
	error |= clSetKernelArg(kernel, 6, sizeof(unsigned int), &C_M2); // COLS_KERNEL

	if (error != CL_SUCCESS) {
		cout << "Error: Failed to set kernel arguments: " << getDescriptionOfError(error) << error << endl;
		exit(1);
	}


	// Get the maximum work group size for executing the kernel on the device
	//
	error = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to retrieve kernel work group info!" << endl;
		exit(1);
	}

	// Execute the kernel over the entire range of our 1d input data set
	// using the maximum number of work group items for this device
	//
	global = count;
	// 1024 is the size of a workgroup

	//global = ceil(ROWS_1*COLS_1 / (local +0.0)) * local;
	//error = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, &global, &local, 0, NULL, NULL);
	cl_uint dim = 2;
	cl_uint ROWS = O_ROWS;
	cl_uint COLS = O_COLS;

	size_t global_work_size[2] = { ROWS, COLS };
	size_t local_work_size[2] = { ROWS, COLS };

	error = clEnqueueNDRangeKernel(commands, kernel, dim, NULL, global_work_size, local_work_size, 0, NULL, NULL);

	if (error) {
		cout << "Error: Failed to execute kernel: " << getDescriptionOfError(error) << endl;
		system("pause");
		return EXIT_FAILURE;
	}

	// Wait for the command commands to get serviced before reading back results
	//
	clFinish(commands);

	// Read back the results from the device to verify the output
	//
	error = clEnqueueReadBuffer(commands, output, CL_TRUE, 0, sizeof(cl_double) * size_src, result, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to read output array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Validate our results
	//
	correct = 0;
	for (int i = 0; i < O_ROWS; i++) {
		for (int j = 0; j < O_COLS; j++) {
			if (fabsf(result[i][j] - EXPECTED_RESULT[i][j]) == 0) correct++;
		}
	}

	//printMatrix(result, O_ROWS, O_COLS);

	cout << "\n\n";
	printMatrix(EXPECTED_RESULT, O_ROWS, O_COLS);
	cout << endl << endl;

	cout << "\n\n";
	//printMatrix((cl_double**)result, O_ROWS, O_COLS);
	for (size_t i = 0; i < O_ROWS; i++) {
		for (size_t j = 0; j < O_COLS; j++) {
			cout << "[" << result[i][j] << "] ";
		}
		cout << endl;
	}
	cout << endl << endl;
	// Print a brief summary detailing the results
	//
	cout << "Computed '" << correct << "/" << count << "' correct values!" << endl;

	// Shutdown and cleanup
	//
	clReleaseMemObject(matrix1);
	clReleaseMemObject(matrix2);
	clReleaseMemObject(output);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);
	system("pause");
	return 0;
}

/*void exit(int code) {
	system("pause");
}*/
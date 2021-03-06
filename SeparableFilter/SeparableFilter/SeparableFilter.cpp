// SeparableFilter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <math.h>

#include <CL/cl.h>

#define K_ROWS 2
#define K_COLS 2
#define O_ROWS 3
#define O_COLS 3
/*#define O_ROWS 10
#define O_COLS 10*/

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
* output size should be ROWS_M * 
*/
const char *VerticalKernel = "\n" \
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n" \
"__kernel void convoluteVertical(                                                  \n" \
"   __global double* matrix,                                              \n" \
"   __global double* vertical_kernel,                                              \n" \
"   __global double* output,                                               \n" \
"   const unsigned int ROWS_M,                                               \n" \
"   const unsigned int COLS_M,                                               \n" \
"   const unsigned int ROWS_K,                                               \n" \
"   const unsigned int COLS_K) {                                             \n" \
"                                                                          \n" \
"   int id_row = get_global_id(0);                                         \n" \
"   int id_col = get_global_id(1);                                         \n" \
"   int center_rows = ROWS_K/2; // center of the filter                    \n" \
"   int center_cols = COLS_K/2; // center of the filter                    \n" \
"   int max_offset = ROWS_K*COLS_K;                                        \n" \
"   int kernel_offset = 0;                                                 \n" \
"   int row_offset = 0;                                                    \n" \
"   int col_offset = 0;                                                    \n" \
"   int matrix_offset = (id_row*COLS_M)+id_col;                            \n" \
"   double acc = 0.0;                                                      \n" \
"                                                                          \n" \
"                                                                          \n" \
"   for(int i = 0; i<ROWS_K;i++){                                                                       \n" \
"                                                                          \n" \
"       kernel_offset = i;												\n"\
"       row_offset = id_row+(i-center_rows);                            \n" \
"       col_offset = id_col;                                            \n" \
"       if(row_offset >=0 && row_offset < ROWS_M){                     \n" \
"                                                                          \n" \
"           acc+=vertical_kernel[kernel_offset]*matrix[(row_offset*COLS_M)+col_offset];    \n" \
"       }                                                                \n" \
"   }                                                                       \n" \
"   output[matrix_offset] = acc;                                                                       \n" \
"}                                                                         \n" \
"\n";


const char *HorizontalKernel = "\n" \
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n" \
"__kernel void convoluteHorizontal(                                                  \n" \
"   __global double* matrix,                                              \n" \
"   __global double* horizontal_kernel,                                              \n" \
"   __global double* output,                                               \n" \
"   const unsigned int ROWS_M,                                               \n" \
"   const unsigned int COLS_M,                                               \n" \
"   const unsigned int ROWS_K,                                               \n" \
"   const unsigned int COLS_K) {                                             \n" \
"                                                                          \n" \
"   int id_row = get_global_id(0);                                         \n" \
"   int id_col = get_global_id(1);                                         \n" \
"   int center_rows = ROWS_K/2; // center of the filter                    \n" \
"   int center_cols = COLS_K/2; // center of the filter                    \n" \
"   int max_offset = ROWS_K*COLS_K;                                        \n" \
"   int kernel_offset = 0;                                                 \n" \
"   int row_offset = 0;                                                    \n" \
"   int col_offset = 0;                                                    \n" \
"   int matrix_offset = (id_row*COLS_M)+id_col;                            \n" \
"   double acc = 0.0;                                                      \n" \
"                                                                          \n" \
"                                                                          \n" \
"                                                                          \n" \
"   for(int i = 0; i<COLS_K;i++){                                                                       \n" \
"                                                                         \n" \
"       kernel_offset = i;												\n"\
"       row_offset = id_row;                            \n" \
"       col_offset = id_col+(i-center_cols);                                            \n" \
"                                                                         \n" \
"                                                                         \n" \
"       if(col_offset >=0 && col_offset < COLS_M){                     \n" \
"                                                                          \n" \
"           acc+=horizontal_kernel[kernel_offset]*matrix[(row_offset*COLS_M)+col_offset];    \n" \
"       }                                                                \n" \
"   }                                                                       \n" \
"   output[matrix_offset] = acc;                                           \n" \
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
					if ((c >= 0 && c<O_ROWS) && (d >= 0 && d<O_COLS)) {
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
	cl_int error,error2;

	// Matrix declaration.
	cl_double result[O_ROWS][O_COLS];
	cl_double** src_matrix = new cl_double*[O_ROWS];

	cl_double SRC[O_ROWS][O_COLS] = { 1,2,3,4,5,6,7,8,9 };
	/*cl_double SRC[O_ROWS][O_COLS];// = { 1,2,3,4,5,6,7,8,9 };
	for (size_t i = 0; i < O_ROWS; i++) {
		for (size_t j = 0; j < O_COLS; j++) {
			SRC[i][j] = rand() / (float)RAND_MAX;
		}
	}*/

	for (size_t i = 0; i < O_ROWS; i++) {
		src_matrix[i] = new cl_double[O_COLS];
		for (size_t j = 0; j < O_COLS; j++) {
			src_matrix[i][j] = SRC[i][j];
		}
	}



	cl_double** kernel_matrix = new cl_double*[K_ROWS];
	cl_double kernel_data[K_ROWS][K_COLS] = { 1,1,1,1 };
	cl_double vkernel_data[K_ROWS] = { 1,1 };
	cl_double hkernel_data[K_COLS] = { 1,1 };

	/*cl_double km[K_ROWS][K_COLS] = {
		(1 / (float)16), (1 / (float)8), (1 / (float)16),
		(1 / (float)8) , (1 / (float)4), (1 / (float)8),
		(1 / (float)16), (1 / (float)8), (1 / (float)16)
	};*/

	for (size_t i = 0; i < K_ROWS; i++) {
		kernel_matrix[i] = new cl_double[K_COLS];
		for (size_t j = 0; j < K_COLS; j++) {
			kernel_matrix[i][j] = kernel_data[i][j];
		}
	}

	cl_double** EXPECTED_RESULT = convolute(src_matrix, kernel_matrix);

	/*// Vertical kernel instantiation
	cl_double vkernel[K_ROWS] = { 1,1 };
	// Horizontal kernel instantiation
	cl_double hkernel[K_COLS] = { 1,1 };*/

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
	cl_program vkprogram;                 // compute program
	cl_program hkprogram;                 // compute program

	cl_kernel vertical_kernel_kernel;                   // compute kernel
	cl_kernel horizontal_kernel_kernel;                   // compute kernel

	cl_mem matrix;                       // device memory used for the input array
	cl_mem vkernel;                       // device memory used for the input array
	cl_mem hkernel;
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
	vkprogram = clCreateProgramWithSource(context, 1, (const char **)&VerticalKernel, NULL, &error);
	hkprogram = clCreateProgramWithSource(context, 1, (const char **)&HorizontalKernel, NULL, &error2);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to create compute program1: " << getDescriptionOfError(error) << endl;
		return EXIT_FAILURE;
	}
	if (error2 != CL_SUCCESS) {
		cout << "Error: Failed to create compute program2: " << getDescriptionOfError(error) << endl;
		return EXIT_FAILURE;
	}

	// Build the program executable
	//
	error = clBuildProgram(vkprogram, 0, NULL, "-cl-opt-disable", NULL, NULL);
	error2 = clBuildProgram(hkprogram, 0, NULL, "-cl-opt-disable", NULL, NULL);
	if (error != CL_SUCCESS) {
		size_t len;
		char buffer[2048];

		cout << "Error: Failed to build 1program executable: " << getDescriptionOfError(error) << endl;
		clGetProgramBuildInfo(vkprogram, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		cout << buffer << endl;
		exit(1);
	}
	if (error2 != CL_SUCCESS) {
		size_t len;
		char buffer[2048];
		cout << "Error: Failed to build 2program executable: " << getDescriptionOfError(error2) << endl;
		clGetProgramBuildInfo(hkprogram, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		cout << buffer << endl;
		exit(1);
	}

	// Create the compute kernel in the program we wish to run
	//
	vertical_kernel_kernel = clCreateKernel(vkprogram, "convoluteVertical", &error);
	//horizontal_kernel_kernel = clCreateKernel(hkprogram, "convoluteHorizontal", &error2); GOOD
	horizontal_kernel_kernel = clCreateKernel(hkprogram, "convoluteHorizontal", &error2);
	if (!vertical_kernel_kernel || !horizontal_kernel_kernel || error != CL_SUCCESS || error2 != CL_SUCCESS) {
		cl_int local_err = error!=CL_SUCCESS ? error : error2;
		cout << "Error: Failed to create compute kernel: " << getDescriptionOfError(local_err) << endl;
		exit(1);
	}

	// Create the input and output arrays in device memory for our calculation
	//
	// The vectors to add.
	const unsigned int size_src = O_ROWS*O_COLS;
	const unsigned int size_vkernel = K_ROWS;
	const unsigned int size_hkernel = K_COLS;

	matrix = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_double) * size_src, NULL, NULL);
	vkernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_double) * size_vkernel, NULL, NULL);
	hkernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_double) * size_hkernel, NULL, NULL);

	// Place where to store the obtained results 
	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_double) * size_src, NULL, NULL);
	if (!matrix || !vkernel || !hkernel || !output) {
		cout << "Error: Failed to allocate device memory: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Write our data set into the input array in device memory
	//
	//cout << "test" << endl;
	printMatrix(src_matrix, O_ROWS, O_COLS);
	error = clEnqueueWriteBuffer(commands, matrix, CL_TRUE, 0, sizeof(cl_double) * size_src, SRC, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to write to source array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}


	//error = clEnqueueWriteBuffer(commands, vkernel, CL_TRUE, 0, sizeof(cl_double) * size_vkernel, kernel_data, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(commands, vkernel, CL_TRUE, 0, sizeof(cl_double) * size_vkernel, vkernel_data, 0, NULL, NULL);
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
	error = clSetKernelArg(vertical_kernel_kernel, 0, sizeof(cl_mem), &matrix);    // matrix1
	error |= clSetKernelArg(vertical_kernel_kernel, 1, sizeof(cl_mem), &vkernel);   // matrix 2
	error |= clSetKernelArg(vertical_kernel_kernel, 2, sizeof(cl_mem), &output);    // output
	error |= clSetKernelArg(vertical_kernel_kernel, 3, sizeof(unsigned int), &R_M1); // ROWS_SRC
	error |= clSetKernelArg(vertical_kernel_kernel, 4, sizeof(unsigned int), &C_M1); // COLS_SRC
	error |= clSetKernelArg(vertical_kernel_kernel, 5, sizeof(unsigned int), &R_M2); // ROWS_KERNEL
	error |= clSetKernelArg(vertical_kernel_kernel, 6, sizeof(unsigned int), &C_M2); // COLS_KERNEL

	if (error != CL_SUCCESS) {
		cout << "Error: Failed to set kernel arguments: " << getDescriptionOfError(error) << error << endl;
		exit(1);
	}


	// Get the maximum work group size for executing the kernel on the device
	//
	error = clGetKernelWorkGroupInfo(vertical_kernel_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to retrieve kernel work group info!" << endl;
		exit(1);
	}

	// Execute the kernel over the entire range of our 1d input data set
	// using the maximum number of work group items for this device
	//
	global = count;
	// 1024 is the size of a workgroup

	cl_uint dim = 2;
	cl_uint ROWS = O_ROWS;
	cl_uint COLS = O_COLS;

	size_t global_work_size[2] = { ROWS, COLS };
	size_t local_work_size[2] = { ROWS, COLS };

	error = clEnqueueNDRangeKernel(commands, vertical_kernel_kernel, dim, NULL, global_work_size, local_work_size, 0, NULL, NULL);

	if (error) {
		cout << "Error: Failed to execute kernel: " << getDescriptionOfError(error) << endl;
		system("pause");
		return EXIT_FAILURE;
	}

	// Wait for the command commands to get serviced before reading back results
	//
	clFinish(commands);
	// Read back the results from the device to get the vertically convoluted matrix.
	//
	error = clEnqueueReadBuffer(commands, output, CL_TRUE, 0, sizeof(cl_double) * size_src, result, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to read output array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}


	cout << "DEBUG: After VERTICAL convolution." << endl;
	//printMatrix((cl_double**)result, O_ROWS, O_COLS);
	for (size_t i = 0; i < O_ROWS; i++) {
		for (size_t j = 0; j < O_COLS; j++) {
			cout << "[" << result[i][j] << "] ";
		}
		cout << endl;
	}
	system("pause");


	///begin horizontal_kernel


	// NOW matrix should be equal to the results obtained on the vertically convolved matrix.
	error = clEnqueueWriteBuffer(commands, matrix, CL_TRUE, 0, sizeof(cl_double) * size_src, result, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to write to source array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}


	error = clEnqueueWriteBuffer(commands, hkernel, CL_TRUE, 0, sizeof(cl_double) * size_hkernel, hkernel_data, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to write to source array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}


	// same kernel arguments to our compute kernel
	//
	error = 0;
	error = clSetKernelArg(horizontal_kernel_kernel, 0, sizeof(cl_mem), &matrix);    // matrix1
	error |= clSetKernelArg(horizontal_kernel_kernel, 1, sizeof(cl_mem), &hkernel);   // matrix 2
	error |= clSetKernelArg(horizontal_kernel_kernel, 2, sizeof(cl_mem), &output);    // output
	error |= clSetKernelArg(horizontal_kernel_kernel, 3, sizeof(unsigned int), &R_M1); // ROWS_SRC
	error |= clSetKernelArg(horizontal_kernel_kernel, 4, sizeof(unsigned int), &C_M1); // COLS_SRC
	error |= clSetKernelArg(horizontal_kernel_kernel, 5, sizeof(unsigned int), &R_M2); // ROWS_KERNEL
	error |= clSetKernelArg(horizontal_kernel_kernel, 6, sizeof(unsigned int), &C_M2); // COLS_KERNEL

	if (error != CL_SUCCESS) {
		cout << "Error: Failed to set second kernel arguments: " << getDescriptionOfError(error) << error << endl;
		exit(1);
	}

	// same dimension
	error = clEnqueueNDRangeKernel(commands, horizontal_kernel_kernel, dim, NULL, global_work_size, local_work_size, 0, NULL, NULL);
	if (error) {
		cout << "Error: Failed to execute kernel2: " << getDescriptionOfError(error) << endl;
		system("pause");
		return EXIT_FAILURE;
	}

	// Wait for the command commands to get serviced before reading back results
	//
	clFinish(commands);
	// Read back the results from the device to get the vertically convoluted matrix.
	//
	error = clEnqueueReadBuffer(commands, output, CL_TRUE, 0, sizeof(cl_double) * size_src, result, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to read output array2: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	///END horizontal_kernel
	cout << endl << endl << "DEBUG: After HORIZONTAL convolution." << endl;
	//printMatrix((cl_double**)result, O_ROWS, O_COLS);
	for (size_t i = 0; i < O_ROWS; i++) {
		for (size_t j = 0; j < O_COLS; j++) {
			cout << "[" << result[i][j] << "] ";
		}
		cout << endl;
	}
	cout << endl << endl;
	
	//printMatrix(result, O_ROWS, O_COLS);
	cout << "BUT EXPECTED: " << endl;
	printMatrix(EXPECTED_RESULT, O_ROWS, O_COLS);
	cout << endl << endl;

	// Validate our results
	//
	correct = 0;
	for (int i = 0; i < O_ROWS; i++) {
		for (int j = 0; j < O_COLS; j++) {
			if (fabsf(result[i][j] - EXPECTED_RESULT[i][j]) == 0) correct++;
		}
	}

	

	
	// Print a brief summary detailing the results
	//
	cout << "Computed '" << correct << "/" << count << "' correct values!" << endl;

	// Shutdown and cleanup
	//
	clReleaseMemObject(matrix);
	clReleaseMemObject(vkernel);
	clReleaseMemObject(hkernel);
	clReleaseMemObject(output);
	clReleaseProgram(vkprogram);
	clReleaseProgram(hkprogram);
	clReleaseKernel(vertical_kernel_kernel);
	clReleaseKernel(horizontal_kernel_kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);
	system("pause");
	return 0;
}

/*void exit(int code) {
	system("pause");
}*/
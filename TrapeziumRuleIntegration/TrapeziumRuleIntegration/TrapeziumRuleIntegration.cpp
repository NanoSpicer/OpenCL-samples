// TrapeziumRuleIntegration.cpp : Solution 1.
//

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <math.h>

#include <CL/cl.h>

#define ROWS_1 3
#define COLS_1 2
#define ROWS_2 2
#define COLS_2 4

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


const char *KernelSource = "\n" \
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n" \
"                                                                          \n" \
"double ex(double x){                                                                          \n" \
"    return pow(2.71828, x);                                                                      \n" \
"}                                                                          \n" \
"                                                                          \n" \
"__kernel void integrateEx(                                                  \n" \
"   __global double* intervals,                                             \n" \
"   __global double* output) {                                             \n" \
"                                                                          \n" \
"   int id = get_global_id(0);                                              \n" \
"   // heuristic. on e^x fA always will be lower than fB                                                                       \n" \
"   double h = intervals[id+1]-intervals[id]; // If it doesn't work, pass it by parameter.                              \n"\
"   double fA = ex(intervals[id]);                                         \n"\
"   double fB = ex(intervals[id+1]);                                         \n"\
"                                                                          \n" \
"   double trapArea = 0.0;                                                 \n" \
"   double sqArea = fA*h;                                                 \n" \
"   double triArea = (fB -fA)*h/2;                                                 \n" \
"                                                                          \n" \
"   trapArea = sqArea+triArea;                                                                       \n" \
"                                                                          \n" \
"   output[id] = trapArea;                                                                       \n" \
"}                                                                         \n" \
"\n";


bool useGPU = true;
//cl_uint usePlatform = 1;
cl_uint usePlatform = 0;
cl_uint useDevice = 1;

/*
returns a matrix of floats of rows1 * cols2 dimension.
*/
cl_double** matrixProduct(cl_double matrix1[ROWS_1][COLS_1], cl_double matrix2[ROWS_2][COLS_2]) {

	// Error control!
	if (COLS_1 != ROWS_2) {
		cout << "Invalid matrix size. COLS_1 and ROWS_2 don't match!";
		return NULL;
	}

	//cl_double result[ROWS_1][COLS_2];
	cl_double** result = new cl_double*[ROWS_1];
	for (int i = 0; i < ROWS_1; i++) {
		result[i] = new cl_double[COLS_2];
		for (int j = 0; j < COLS_2; j++) {
			result[i][j] = 0;
		}
	}

	// Foreach row in matrix1
	for (int i = 0; i < ROWS_1; i++) {
		// ... and foreach value in that row...
		for (int j = 0; j < COLS_1; j++) { // We could use indistinctly COLS_1 or ROWS_2
										   // and foreach row in the second matrix multiply the values adding to the according position
			for (int k = 0; k < COLS_2; k++) {
				result[i][k] += matrix1[i][j] * matrix2[j][k];
			}
		}
	}

	return result;
}

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

int readNumber(const string message) {
	bool right = false;
	string input;
	int number;
	cout << message;
	while (!right) {
		getline(cin, input);
		try {
			number = stoi(input);
			right = true;
		} catch (invalid_argument exception) {
			cout << "Please enter a valid number." << endl;
			right = false;
		}
	}

	cout << "The input was: " << number << endl;

	return number;
}
int main(void) {

	cl_int error;

	int a = readNumber("Enter the number \"a\" from the interval [a,b]: ");
	cout << endl << endl;

	int b = 0;
	
	do {
		b = readNumber("Enter the number \"b\" [MUST BE HIGHER THAN \"a\"] from the interval [a,b]: ");
	} while (b <= a);
	cout << endl << endl;

	int n_intervals = readNumber("Enter the amount of intervals for the rule: ");
	cout << endl << endl << endl;
	system("cls");
	cout << "The integration interval is [" << a << ", " << b << "]. in function e^x" << endl;
	cout << "With " << n_intervals << " intervals for the  rule." << endl << endl;
	system("pause");

	double interval_range = ((b - a) +0.0)/ n_intervals; // each interval has a length of "interval_range".

	// Each and every point that f(a) and f(b) will have to be applied.
	cl_double* intervals = (cl_double*)malloc(sizeof(cl_double)*(n_intervals + 1));
	// Init each interval. Every work-item will be reading 2 of these according to their ID.
	double counter = (double)a;
	for (int i = 0; i < n_intervals + 1; i++) {
		intervals[i] = counter;
		counter += interval_range;
	}

	cout << endl << endl;

	if (n_intervals <= 10) {
		cout << "Each interval is: " << endl;
		for (int i = 0; i < n_intervals + 1; i++) {
			cout << intervals[i] << endl;
		}
	}
	


	// The result array.
	cl_double* result = (cl_double*) malloc(sizeof(cl_double)*n_intervals);
	// Init it with 0s
	for (size_t i = 0; i < n_intervals; i++) {
		result[i] = 0.0;
	}

	unsigned int correct;

	size_t global;
	size_t local;

	cl_platform_id platform_id;
	cl_device_id device_id;             // compute device id
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_kernel kernel;                   // compute kernel

	cl_mem clmem_intervals;                       // device memory used for the input array
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
	kernel = clCreateKernel(program, "integrateEx", &error);
	if (!kernel || error != CL_SUCCESS) {
		cout << "Error: Failed to create compute kernel: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Create the input and output arrays in device memory for our calculation
	//
	// The vectors to add.
	const unsigned int size_intervals = n_intervals+1;
	const unsigned int size_results = n_intervals;

	// a,b values that are going to be read from.
	clmem_intervals = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_double) * size_intervals, NULL, NULL);

	// Place where to store the obtained results 
	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_double) * size_results, NULL, NULL);
	if (!clmem_intervals || !output) {
		cout << "Error: Failed to allocate device memory: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Write our data set into the input array in device memory
	//
	error = clEnqueueWriteBuffer(commands, clmem_intervals, CL_TRUE, 0, sizeof(cl_double) * size_intervals, intervals, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to write to source array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Set the arguments to our compute kernel
	//
	error = 0;
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &clmem_intervals);    // matrix1
	error |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);    // output
	

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
	//;

	size_t global_work_size[1] = { n_intervals };
	size_t local_work_size[1] = { n_intervals };
	error = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, global_work_size, local_work_size, 0, NULL, NULL);

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
	error = clEnqueueReadBuffer(commands, output, CL_TRUE, 0, sizeof(cl_double) * n_intervals, result, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		cout << "Error: Failed to read output array: " << getDescriptionOfError(error) << endl;
		exit(1);
	}

	// Validate our results
	//
	double totalArea = 0.0;
	for (size_t i = 0; i < n_intervals; i++) {
		totalArea += result[i];
	}
	// Integrative result from wolfram alpha gives 145,69, this gives e^1 - e^5 with 100 steps: 145.714
	cout << "Integrative result: " << totalArea << endl;
	// Shutdown and cleanup
	//
	clReleaseMemObject(clmem_intervals);
	clReleaseMemObject(output);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);
	system("pause");
	return 0;
}
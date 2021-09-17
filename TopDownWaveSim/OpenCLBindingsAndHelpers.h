#pragma once

#include <cstdint>                                                                                  // Used for fixed-width types.

// These almost definitely only work in Windows, so if you ever want to port this to another system, these definitely need to change.
#define CL_API_CALL _stdcall                                                                        // Calling covention for the OpenCL API calls.
#define CL_CALLBACK _stdcall																		// Calling convention for the OpenCL callback functions.

#define CL_SUCCESS 0																				// Success error code for OpenCL functions.

/* cl_platoform_info */
#define CL_PLATFORM_PROFILE                         0x0900											// Different types of platform info to query using clGetPlatformInfo.
#define CL_PLATFORM_VERSION                         0x0901
#define CL_PLATFORM_NAME                            0x0902
#define CL_PLATFORM_VENDOR                          0x0903
#define CL_PLATFORM_EXTENSIONS                      0x0904

/* cl_device_type - bitfield */
#define CL_DEVICE_TYPE_DEFAULT                      (1 << 0)										// Type of device to use in clGetDeviceIDs.
#define CL_DEVICE_TYPE_CPU                          (1 << 1)
#define CL_DEVICE_TYPE_GPU                          (1 << 2)
#define CL_DEVICE_TYPE_ACCELERATOR                  (1 << 3)

/* cl_program_build_info */
#define CL_PROGRAM_BUILD_STATUS                     0x1181											// Type of build info to get in clGetProgramBuildInfo
#define CL_PROGRAM_BUILD_OPTIONS                    0x1182
#define CL_PROGRAM_BUILD_LOG                        0x1183

/* cl_mem_flags and cl_svm_mem_flags - bitfield */
#define CL_MEM_READ_WRITE                           (1 << 0)										// Memory flags (access modifiers and such) when doing (for example) clCreateImage2D.
#define CL_MEM_WRITE_ONLY                           (1 << 1)
#define CL_MEM_READ_ONLY                            (1 << 2)
#define CL_MEM_USE_HOST_PTR                         (1 << 3)
#define CL_MEM_ALLOC_HOST_PTR                       (1 << 4)
#define CL_MEM_COPY_HOST_PTR                        (1 << 5)

/* cl_channel_order */
#define CL_R                                        0x10B0											// Order of the channels when creating an image.
#define CL_A                                        0x10B1
#define CL_RG                                       0x10B2
#define CL_RA                                       0x10B3
#define CL_RGB                                      0x10B4
#define CL_RGBA                                     0x10B5
#define CL_BGRA                                     0x10B6
#define CL_ARGB                                     0x10B7
#define CL_INTENSITY                                0x10B8
#define CL_LUMINANCE                                0x10B9

/* cl_channel_type */
#define CL_SNORM_INT8                               0x10D0											// The type of channels to use when creating and image.
#define CL_SNORM_INT16                              0x10D1
#define CL_UNORM_INT8                               0x10D2
#define CL_UNORM_INT16                              0x10D3
#define CL_UNORM_SHORT_565                          0x10D4
#define CL_UNORM_SHORT_555                          0x10D5
#define CL_UNORM_INT_101010                         0x10D6
#define CL_SIGNED_INT8                              0x10D7
#define CL_SIGNED_INT16                             0x10D8
#define CL_SIGNED_INT32                             0x10D9
#define CL_UNSIGNED_INT8                            0x10DA
#define CL_UNSIGNED_INT16                           0x10DB
#define CL_UNSIGNED_INT32                           0x10DC
#define CL_HALF_FLOAT                               0x10DD
#define CL_FLOAT                                    0x10DE

/* cl_kernel_work_group_info */
#define CL_KERNEL_WORK_GROUP_SIZE                   0x11B0											// Possible queries for clGetKernelWorkGroupInfo.
#define CL_KERNEL_COMPILE_WORK_GROUP_SIZE           0x11B1
#define CL_KERNEL_LOCAL_MEM_SIZE                    0x11B2
#define CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE 0x11B3
#define CL_KERNEL_PRIVATE_MEM_SIZE                  0x11B4

// Making fixed-width floats is harder than making fixed-width ints because the data type is more complicated, which is why that isn't in the standard library yet.
// That means you can't use sizeof(x) to get the correct size of a float. To make getting the size of OpenCL floats (which are always 32-bit) easy, this is here.
#define CL_FLOAT_SIZE 4

// Simple type definitions for basic fixed-width, OpenCL compatible types.
typedef int32_t cl_int;
typedef uint32_t cl_uint;
typedef uint64_t cl_ulong;
typedef cl_uint cl_bool;																			// For some reason, this is not guaranteed to be the same size as bools in the kernel.
typedef cl_ulong cl_bitfield;

// Platforms
typedef struct _cl_platform_id* cl_platform_id;
typedef cl_uint cl_platform_info;

// Devices
typedef cl_bitfield cl_device_type;
typedef struct _cl_device_id* cl_device_id;

// Contexts
typedef struct _cl_context* cl_context;
typedef intptr_t cl_context_properties;

// Command queues.
typedef struct _cl_command_queue* cl_command_queue;
typedef cl_bitfield cl_command_queue_properties;

// Programs
typedef struct _cl_program* cl_program;
typedef cl_uint cl_program_build_info;

// Kernels
typedef struct _cl_kernel* cl_kernel;
typedef cl_uint cl_kernel_work_group_info;

// Memory
typedef struct _cl_mem* cl_mem;
typedef cl_bitfield cl_mem_flags;

// Image format
typedef cl_uint             cl_channel_order;
typedef cl_uint             cl_channel_type;

// Events
typedef struct _cl_event* cl_event;

// Image format struct
struct cl_image_format {
	cl_channel_order image_channel_order;
	cl_channel_type image_channel_data_type;
};

typedef cl_int (CL_API_CALL* clGetPlatformIDs_func)(cl_uint num_entries, 
													cl_platform_id* platforms, 
													cl_uint* num_platforms);
// Gets all of the availables platform IDs on the system.
inline clGetPlatformIDs_func clGetPlatformIDs;

typedef cl_int (CL_API_CALL* clGetPlatformInfo_func)(cl_platform_id platform, 
													 cl_platform_info param_name, 
													 size_t param_value_size, 
													 void* param_value, 
													 size_t* param_value_size_ret);
// Gets platform info for a specific platform.
inline clGetPlatformInfo_func clGetPlatformInfo;

typedef cl_int (CL_API_CALL* clGetDeviceIDs_func)(cl_platform_id platform, 
												  cl_device_type device_type, 
												  cl_uint num_entries, 
												  cl_device_id* devices, 
												  cl_uint* num_devices);
// Gets all the available device IDs on a specific platform.
inline clGetDeviceIDs_func clGetDeviceIDs;

typedef cl_context (CL_API_CALL* clCreateContext_func)(const cl_context_properties* properties, 
													   cl_uint num_devices, 
													   const cl_device_id* devices, 
													   void (CL_CALLBACK* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), 
													   void* user_data, 
													   cl_int* errcode_ret);
// Creates an OpenCL context. I assume this holds data which is useful for later functions. It would make sense if they work on the context like a state machine.
inline clCreateContext_func clCreateContext;

typedef cl_command_queue (CL_API_CALL* clCreateCommandQueue_func)(cl_context context, 
																  cl_device_id device, 
																  cl_command_queue_properties properties, 
																  cl_int* errcode_ret);
// Creates a command queue. This queue holds commands that will be executed sequentially. This is useful because these commands can run parallel to main code.
inline clCreateCommandQueue_func clCreateCommandQueue;

typedef cl_program (CL_API_CALL* clCreateProgramWithSource_func)(cl_context context, 
																 cl_uint count, 
																 const char** strings, 
																 const size_t* lengths, 
																 cl_int* errcode_ret);
// Creates an OpenCL program with the specified source.
inline clCreateProgramWithSource_func clCreateProgramWithSource;

typedef cl_int (CL_API_CALL* clBuildProgram_func)(cl_program program, 
												  cl_uint num_devices, 
												  const cl_device_id* device_list, 
												  const char* options, 
												  void (CL_CALLBACK* pfn_notify)(cl_program program, void* user_data), 
												  void* user_data);
// Builds an OpenCL program which was created with clCreateProgramWithSource.
inline clBuildProgram_func clBuildProgram;

typedef cl_int (CL_API_CALL* clGetProgramBuildInfo_func)(cl_program program, 
														 cl_device_id device, 
														 cl_program_build_info param_name, 
														 size_t param_value_size, 
														 void* param_value, 
														 size_t* param_value_size_ret);
// Gets build info about a built program. Useful for getting build logs of builds that didn't complete because of some error. This is the main tool when debugging kernels.
inline clGetProgramBuildInfo_func clGetProgramBuildInfo;

typedef cl_kernel (CL_API_CALL* clCreateKernel_func)(cl_program program, 
													 const char* kernel_name, 
													 cl_int* errcode_ret);
// Creates an OpenCL kernel using the successfully built program. A program can contain multiple kernels if I'm not mistaken, which is why this is necessary.
inline clCreateKernel_func clCreateKernel;

typedef cl_mem (CL_API_CALL* clCreateImage2D_func)(cl_context context, 
												   cl_mem_flags flags, 
												   const cl_image_format* image_format, 
												   size_t image_width, 
												   size_t image_height, 
												   size_t image_row_pitch, 
												   void* host_ptr, 
												   cl_int* errcode_ret);
// Creates a 2D image. This is essentially the same thing as a normal buffer, except you can access it in 2D and it contains various image channels (RGBA and such).
inline clCreateImage2D_func clCreateImage2D;

typedef cl_int (CL_API_CALL* clSetKernelArg_func)(cl_kernel kernel, 
												  cl_uint arg_index, 
												  size_t arg_size, 
												  const void* arg_value);
// Sets kernel arguments.
inline clSetKernelArg_func clSetKernelArg;

typedef cl_int (CL_API_CALL* clGetKernelWorkGroupInfo_func)(cl_kernel kernel, 
															cl_device_id device, 
															cl_kernel_work_group_info param_name, 
															size_t param_value_size, 
															void* param_value, 
															size_t* param_value_size_ret);
// Gets kernel work group info. Things like the optimal work group multiple and maximum work group size (based on the kernel memory usage and such) can be gotten.
inline clGetKernelWorkGroupInfo_func clGetKernelWorkGroupInfo;

typedef cl_int (CL_API_CALL* clEnqueueNDRangeKernel_func)(cl_command_queue command_queue, 
														  cl_kernel kernel, 
														  cl_uint work_dim, 
														  const size_t* global_work_offset, 
														  const size_t* global_work_size, 
														  const size_t* local_work_size, 
														  cl_uint num_events_in_wait_list, 
														  const cl_event* event_wait_list, 
														  cl_event* event);
// Enqueues a kernel on the command queue. There is no way to make this synchronous, you just have to use clFinish afterwards if you want to wait until it finishes.
inline clEnqueueNDRangeKernel_func clEnqueueNDRangeKernel;

typedef cl_int (CL_API_CALL* clFinish_func)(cl_command_queue command_queue);
// Waits for every entry in the command queue to finish. You can use this to synchronize OpenCL tasks with your own tasks.
inline clFinish_func clFinish;

typedef cl_int (CL_API_CALL* clEnqueueWriteBuffer_func)(cl_command_queue command_queue, 
														cl_mem buffer, 
														cl_bool blocking_write, 
														size_t offset, 
														size_t size, 
														const void* ptr, 
														cl_uint num_events_in_wait_list, 
														const cl_event* event_wait_list, 
														cl_event* event);
// Enqueues a buffer write on the command queue. You can set the blocking_write flag in order to wait for this function to finish without using clFinish.
inline clEnqueueWriteBuffer_func clEnqueueWriteBuffer;

typedef cl_int (CL_API_CALL* clEnqueueWriteImage_func)(cl_command_queue command_queue, 
													   cl_mem image, 
													   cl_bool blocking_write, 
													   const size_t* origin, 
													   const size_t* region, 
													   size_t input_row_pitch, 
													   size_t input_slice_pitch, 
													   const void* ptr, 
													   cl_uint num_events_in_wait_list, 
													   const cl_event* event_wait_list, 
													   cl_event* event);
// Enqueues an image write on the command queue. You can set the blocking_write flag in order to wait for this function to finish without using clFinish.
inline clEnqueueWriteImage_func clEnqueueWriteImage;

typedef cl_int (CL_API_CALL* clEnqueueReadImage_func)(cl_command_queue command_queue, 
													  cl_mem image, 
													  cl_bool blocking_read, 
													  const size_t* origin, 
													  const size_t* region, 
													  size_t row_pitch, 
													  size_t slice_pitch, 
													  void* ptr, 
													  cl_uint num_events_in_wait_list, 
													  const cl_event* event_wait_list, 
													  cl_event* event);
// Enqueues an image read on the command queue. You can set the blocking_read flag in order to wait for this function to finish without using clFinish.
inline clEnqueueReadImage_func clEnqueueReadImage;

// Simple helper function which initializes the dynamic linkage to the OpenCL DLL and initializes the bindings to all of the various functions.
bool initOpenCLBindings();
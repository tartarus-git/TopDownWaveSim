#pragma once

#include <cstdint>

#define CL_API_CALL _stdcall
#define CL_CALLBACK _stdcall

#define CL_SUCCESS 0

#define CL_DEVICE_TYPE_DEFAULT                      (1 << 0)
#define CL_DEVICE_TYPE_CPU                          (1 << 1)
#define CL_DEVICE_TYPE_GPU                          (1 << 2)
#define CL_DEVICE_TYPE_ACCELERATOR                  (1 << 3)

typedef int32_t cl_int;
typedef uint32_t cl_uint;
typedef uint64_t cl_ulong;

typedef cl_ulong cl_bitfield;

typedef struct _cl_platform_id* cl_platform_id;
typedef cl_bitfield cl_device_type;
typedef struct _cl_device_id* cl_device_id;
typedef struct _cl_context* cl_context;
typedef intptr_t cl_context_properties;
typedef struct _cl_command_queue* cl_command_queue;
typedef cl_bitfield cl_command_queue_properties;

typedef struct _cl_program* cl_program;

typedef cl_int (CL_API_CALL* clGetPlatformIDs_func)(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms);
extern const clGetPlatformIDs_func clGetPlatformIDs;

typedef cl_int (CL_API_CALL* clGetDeviceIDs_func)(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices);
extern const clGetDeviceIDs_func clGetDeviceIDs;

typedef cl_context (CL_API_CALL* clCreateContext_func)(const cl_context_properties* properties,
    cl_uint num_devices,
    const cl_device_id* devices,
    void (CL_CALLBACK* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data),
    void* user_data,
    cl_int* errcode_ret);
extern const clCreateContext_func clCreateContext;

typedef cl_command_queue (CL_API_CALL* clCreateCommandQueue_func)(cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_int* errcode_ret);
extern const clCreateCommandQueue_func clCreateCommandQueue;

typedef cl_program (CL_API_CALL* clCreateProgramWithSource_func)(cl_context context,
    cl_uint count,
    const char** strings,
    const size_t* lengths,
    cl_int* errcode_ret);
extern const clCreateProgramWithSource_func clCreateProgramWithSource;

typedef cl_int (CL_API_CALL* clBuildProgram_func)(cl_program program,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* options,
    void (CL_CALLBACK* pfn_notify)(cl_program program, void* user_data),
    void* user_data);
extern const clBuildProgram_func clBuildProgram;
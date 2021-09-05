#pragma once

#include <cstdint>

#define CL_API_CALL _stdcall
#define CL_CALLBACK _stdcall

#define CL_SUCCESS 0

#define CL_PLATFORM_PROFILE                         0x0900
#define CL_PLATFORM_VERSION                         0x0901
#define CL_PLATFORM_NAME                            0x0902
#define CL_PLATFORM_VENDOR                          0x0903
#define CL_PLATFORM_EXTENSIONS                      0x0904

#define CL_DEVICE_TYPE_DEFAULT                      (1 << 0)
#define CL_DEVICE_TYPE_CPU                          (1 << 1)
#define CL_DEVICE_TYPE_GPU                          (1 << 2)
#define CL_DEVICE_TYPE_ACCELERATOR                  (1 << 3)

#define CL_PROGRAM_BUILD_STATUS                     0x1181
#define CL_PROGRAM_BUILD_OPTIONS                    0x1182
#define CL_PROGRAM_BUILD_LOG                        0x1183

#define CL_MEM_READ_WRITE                           (1 << 0)
#define CL_MEM_WRITE_ONLY                           (1 << 1)
#define CL_MEM_READ_ONLY                            (1 << 2)
#define CL_MEM_USE_HOST_PTR                         (1 << 3)
#define CL_MEM_ALLOC_HOST_PTR                       (1 << 4)
#define CL_MEM_COPY_HOST_PTR                        (1 << 5)

#define CL_R                                        0x10B0
#define CL_A                                        0x10B1
#define CL_RG                                       0x10B2
#define CL_RA                                       0x10B3
#define CL_RGB                                      0x10B4
#define CL_RGBA                                     0x10B5
#define CL_BGRA                                     0x10B6
#define CL_ARGB                                     0x10B7
#define CL_INTENSITY                                0x10B8
#define CL_LUMINANCE                                0x10B9

#define CL_SNORM_INT8                               0x10D0
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

#define CL_FLOAT_SIZE 4

typedef int32_t cl_int;
typedef uint32_t cl_uint;
typedef uint64_t cl_ulong;

typedef cl_ulong cl_bitfield;

typedef struct _cl_platform_id* cl_platform_id;
typedef cl_uint cl_platform_info;

typedef cl_bitfield cl_device_type;
typedef struct _cl_device_id* cl_device_id;
typedef struct _cl_context* cl_context;
typedef intptr_t cl_context_properties;
typedef struct _cl_command_queue* cl_command_queue;
typedef cl_bitfield cl_command_queue_properties;

typedef struct _cl_program* cl_program;
typedef cl_uint cl_program_build_info;

typedef struct _cl_kernel* cl_kernel;

typedef struct _cl_mem* cl_mem;
typedef cl_bitfield cl_mem_flags;

typedef cl_uint             cl_channel_order;
typedef cl_uint             cl_channel_type;

struct cl_image_format {
    cl_channel_order image_channel_order;
    cl_channel_type image_channel_data_type;
};

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

typedef cl_int (CL_API_CALL* clGetProgramBuildInfo_func)(cl_program program,
    cl_device_id device,
    cl_program_build_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);
extern const clGetProgramBuildInfo_func clGetProgramBuildInfo;

typedef cl_kernel (CL_API_CALL* clCreateKernel_func)(cl_program program,
    const char* kernel_name,
    cl_int* errcode_ret);
extern const clCreateKernel_func clCreateKernel;

typedef cl_mem (CL_API_CALL* clCreateImage2D_func)(cl_context context,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    size_t image_width,
    size_t image_height,
    size_t image_row_pitch,
    void* host_ptr,
    cl_int* errcode_ret);
extern const clCreateImage2D_func clCreateImage2D;

typedef cl_int (CL_API_CALL* clSetKernelArg_func)(cl_kernel kernel,
    cl_uint arg_index,
    size_t arg_size,
    const void* arg_value);
extern const clSetKernelArg_func clSetKernelArg;

typedef cl_int (CL_API_CALL* clGetPlatformInfo_func)(cl_platform_id platform,
    cl_platform_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);
inline clGetPlatformInfo_func clGetPlatformInfo;

void initOpenCLBindings();
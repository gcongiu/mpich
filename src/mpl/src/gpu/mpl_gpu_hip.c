/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_HAVE_HIP

#define HIP_ERR_CHECK(ret) if (unlikely((ret) != hipSuccess)) goto fn_fail

int MPL_gpu_query_pointer_type(const void *ptr, MPL_pointer_type_t * attr)
{
    hipError_t ret;
    hipPointerAttributes_t ptr_attr = 0;
    ret = hipPointerGetAttributes(&ptr_attr, ptr);
    HIP_ERR_CHECK(ret);
    switch (ptr_attr.type) {
        case hipMemoryTypeUnregistered:
        case hipMemoryTypeHost:
            *attr = MPL_GPU_POINTER_HOST;
            break;
        case hipMemoryTypeDevice:
            *attr = MPL_GPU_POINTER_DEV;
            break;
        case hipMemoryTypeManaged:
            *attr = MPL_GPU_POINTER_MANAGED;
            break;
    }

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}

int MPL_gpu_ipc_get_mem_handle(MPL_gpu_ipc_mem_handle_t * h_mem, void *ptr)
{
    MPL_Abort();

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}

int MPL_gpu_ipc_open_mem_handle(void **ptr, MPL_gpu_ipc_mem_handle_t h_mem)
{
    MPL_Abort();

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}

int MPL_gpu_ipc_close_mem_handle(void *ptr)
{
    MPL_Abort();

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}

int MPL_gpu_malloc_host(size_t size)
{
    MPL_Abort();

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}

int MPL_gpu_free_host(void *ptr)
{
    MPL_Abort();

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}

int MPL_gpu_host_register(void *ptr, size_t size, unsigned int flags)
{
    MPL_Abort();

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}

int MPL_gpu_host_unregister(void *ptr)
{
    MPL_Abort();

  fn_exit:
    return MPL_GPU_SUCCESS;
  fn_fail:
    return MPL_GPU_FAILURE;
}
#endif /* MPL_HAVE_HIP */

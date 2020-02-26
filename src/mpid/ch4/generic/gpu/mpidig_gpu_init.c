/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_GPU_SMALL_BUF_POOL_SZ
      category    : CH4
      type        : int
      default     : 64
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The size of the buffer in small buffer pool for gpu memory staging.

    - name        : MPIR_CVAR_CH4_GPU_MEDIUM_BUF_POOL_SZ
      category    : CH4
      type        : int
      default     : 32768
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The size of the buffer in medium buffer pool for gpu memory staging.

    - name        : MPIR_CVAR_CH4_GPU_LARGE_BUF_POOL_SZ
      category    : CH4
      type        : int
      default     : 4194304
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The size of the buffer in large buffer pool for gpu memory staging.

    - name        : MPIR_CVAR_CH4_GPU_SMALL_BUF_POOL_NUM
      category    : CH4
      type        : int
      default     : 64
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The number of the buffers in small buffer pool for gpu memory staging.

    - name        : MPIR_CVAR_CH4_GPU_MEDIUM_BUF_POOL_NUM
      category    : CH4
      type        : int
      default     : 32
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The number of the buffers in medium buffer pool for gpu memory staging.

    - name        : MPIR_CVAR_CH4_GPU_LARGE_BUF_POOL_NUM
      category    : CH4
      type        : int
      default     : 4
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The number of the buffers in large buffer pool for gpu memory staging.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/
#include "mpidimpl.h"
#include "ch4r_buf.h"
#include "mpidig_gpu.h"
#include "mpidig_gpu_types.h"

static void create_buf_pool(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_CREATE_BUF_POOL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_CREATE_BUF_POOL);
    int num[MPIDIG_GPU_BUF_POOL_NUM] =
        { MPIR_CVAR_CH4_GPU_SMALL_BUF_POOL_NUM, MPIR_CVAR_CH4_GPU_MEDIUM_BUF_POOL_NUM,
        MPIR_CVAR_CH4_GPU_LARGE_BUF_POOL_NUM
    };
    size_t size[MPIDIG_GPU_BUF_POOL_NUM] =
        { MPIR_CVAR_CH4_GPU_SMALL_BUF_POOL_SZ, MPIR_CVAR_CH4_GPU_MEDIUM_BUF_POOL_SZ,
        MPIR_CVAR_CH4_GPU_LARGE_BUF_POOL_SZ
    };

    MPIDIU_buf_pool_t **pools = MPIDIG_gpu_global.buf_pool;
    for (int i = 0; i < MPIDIG_GPU_BUF_POOL_NUM; i++) {
        pools[i] = MPIDIU_create_buf_pool_internal(num[i], size[i], MPIDIU_BUF_POOL_PINNED, NULL);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_CREATE_BUF_POOL);
}

static void destroy_buf_pool(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_DESTROY_BUF_POOL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_DESTROY_BUF_POOL);
    MPIDIU_buf_pool_t **pools = MPIDIG_gpu_global.buf_pool;
    for (int i = 0; i < MPIDIG_GPU_BUF_POOL_NUM; i++) {
        MPIDIU_destroy_buf_pool(pools[i]);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_DESTROY_BUF_POOL);
}

int MPIDIG_gpu_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIU_map_create((void **) &MPIDIG_gpu_global.buf_map, MPL_MEM_OTHER);
    create_buf_pool();
    return mpi_errno;
}

void MPIDIG_gpu_finalize(void)
{
    MPIDIU_map_destroy(MPIDIG_gpu_global.buf_map);
    destroy_buf_pool();
}

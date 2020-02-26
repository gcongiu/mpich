/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDIG_GPU_TYPES_H_INCLUDED
#define MPIDIG_GPU_TYPES_H_INCLUDED

#define MPIDIG_GPU_BUF_POOL_NUM (3)

typedef struct MPIDIG_gpu_global_t {
    MPIDIU_buf_pool_t *buf_pool[MPIDIG_GPU_BUF_POOL_NUM];       /* Pools of registered buffers used by GPU
                                                                 * fallback code to stage device memory */
    MPIDIU_map_t *buf_map;      /* Track buffers currently being used */
} MPIDIG_gpu_global_t;
extern MPIDIG_gpu_global_t MPIDIG_gpu_global;

#endif /* MPIDIG_GPU_TYPES_H_INCLUDED */

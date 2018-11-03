/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <limits.h>
#include "dtpools_internal.h"

#define DTPI_MAX_FACTOR (64)

/* get list of MPI datatypes at compilation time */
MPI_Datatype DTP_Basic_type[] = { DTP_MPI_DATATYPE };

/* pointer to create functions for datatypes */
static DTPI_Creator creators[DTPI_OBJ_TYPE__NUM];
static DTPI_Destructor destructors[DTPI_OBJ_TYPE__NUM];
static DTPI_Checker checkers[DTPI_OBJ_TYPE__NUM];
static int DTPI_Initialized = 0;

/* return factor of count */
static int DTPI_Get_max_fact(int count)
{
    int i = 2;

    if ((count / 2) < DTPI_MAX_FACTOR && (count / 2) > 0) {
        i = (count / 2) + 1;
    } else if ((count / 2) >= DTPI_MAX_FACTOR) {
        i = DTPI_MAX_FACTOR + 1;
    }

    while ((count % --i) != 0 && i > 1);

    return i;
}

/*
 * DTP_pool_create - create a new datatype pool with specified signature
 *
 * basic_type:        signature's basic datatype
 * basic_type_count:  signature's basic datatype count
 * dtp:               returned datatype pool object
 */
int DTP_pool_create(MPI_Datatype basic_type, MPI_Aint basic_type_count, DTP_t * dtp)
{
    int i, err = DTP_SUCCESS;
    int num_objs;
    int env_num_objs = INT_MAX;
    char *env_num_objs_str = NULL;
    struct DTP_obj_array_s *obj_array = NULL;

    DTPI_OBJ_ALLOC_OR_FAIL(*dtp, sizeof(**dtp));

    /* get number of objects from environment */
    if ((env_num_objs_str = getenv("DTP_NUM_OBJS"))) {
        env_num_objs = atoi(env_num_objs_str);
    }

    if (basic_type_count == 1) {
        num_objs =
            (env_num_objs < DTPI_OBJ_LAYOUT_SIMPLE__NUM &&
             env_num_objs > 0) ? env_num_objs : DTPI_OBJ_LAYOUT_SIMPLE__NUM;
    } else {
        num_objs =
            (env_num_objs < DTPI_OBJ_LAYOUT_LARGE__NUM &&
             env_num_objs > 0) ? env_num_objs : DTPI_OBJ_LAYOUT_LARGE__NUM;
    }

    DTPI_OBJ_ALLOC_OR_FAIL(obj_array, sizeof(*obj_array) * num_objs);

    for (i = 0; i < num_objs; i++) {
        obj_array[i].DTP_obj_type = MPI_DATATYPE_NULL;
        obj_array[i].DTP_obj_count = 0;
        obj_array[i].DTP_obj_buf = NULL;
        obj_array[i].private_info = NULL;
    }

    (*dtp)->DTP_num_objs = num_objs;
    (*dtp)->DTP_pool_type = DTP_POOL_TYPE__BASIC;
    (*dtp)->DTP_type_signature.DTP_pool_basic.DTP_basic_type = basic_type;
    (*dtp)->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count = basic_type_count;
    (*dtp)->DTP_obj_array = (struct DTP_obj_array_s *) obj_array;

    if (!DTPI_Initialized) {
        DTPI_Init_creators(creators);
        DTPI_Init_destructors(destructors);
        DTPI_Init_checkers(checkers);
        DTPI_Initialized = 1;
    }

  fn_exit:
    return err;

  fn_fail:
    if (*dtp) {
        DTPI_OBJ_FREE(*dtp);
    }
    if (obj_array) {
        DTPI_OBJ_FREE(obj_array);
    }
    goto fn_exit;
}

/*
 * DTP_pool_create_struct - create a new struct pool with specified signature
 *
 * num_types:          number of basic datatypes in struct
 * basic_types:        array of basic datatypes in struct
 * basic_type_counts:  array of counts for each of the
 *                     basic datatypes in struct
 * dtp:                returned datatype pool object
 */
int DTP_pool_create_struct(int num_types, MPI_Datatype * basic_types, int *basic_type_counts,
                           DTP_t * dtp)
{
    int i, err = DTP_SUCCESS;
    int num_objs;
    int env_num_objs = INT_MAX;
    char *env_num_objs_str = NULL;
    struct DTP_obj_array_s *obj_array = NULL;

    DTPI_OBJ_ALLOC_OR_FAIL(*dtp, sizeof(**dtp));

    /* get number of objects from environment */
    if ((env_num_objs_str = getenv("DTP_NUM_OBJS"))) {
        env_num_objs = atoi(env_num_objs_str);
    }

    num_objs = (env_num_objs < DTPI_OBJ_LAYOUT__STRUCT_NUM && env_num_objs > 0) ?
        env_num_objs : DTPI_OBJ_LAYOUT__STRUCT_NUM;

    DTPI_OBJ_ALLOC_OR_FAIL(obj_array, sizeof(*obj_array) * num_objs);

    for (i = 0; i < num_objs; i++) {
        obj_array[i].DTP_obj_type = MPI_DATATYPE_NULL;
        obj_array[i].DTP_obj_count = 0;
        obj_array[i].DTP_obj_buf = NULL;
        obj_array[i].private_info = NULL;
    }

    (*dtp)->DTP_num_objs = num_objs;
    (*dtp)->DTP_pool_type = DTP_POOL_TYPE__STRUCT;
    (*dtp)->DTP_type_signature.DTP_pool_struct.DTP_num_types = num_types;
    (*dtp)->DTP_type_signature.DTP_pool_struct.DTP_basic_type = basic_types;
    (*dtp)->DTP_type_signature.DTP_pool_struct.DTP_basic_type_count = basic_type_counts;
    (*dtp)->DTP_obj_array = (struct DTP_obj_array_s *) obj_array;

  fn_exit:
    return err;

  fn_fail:
    if (*dtp) {
        DTPI_OBJ_FREE(*dtp);
    }
    if (obj_array) {
        DTPI_OBJ_FREE(obj_array);
    }
    goto fn_exit;
}

/*
 * DTP_pool_free - free previously created pool
 */
int DTP_pool_free(DTP_t dtp)
{
    int err = DTP_SUCCESS;

    if (!dtp) {
        return DTP_ERR_OTHER;
    }
    DTPI_OBJ_FREE(dtp->DTP_obj_array);
    DTPI_OBJ_FREE(dtp);

    return err;
}

/*
 * DTP_obj_create - create obj_idx datatype inside the pool
 *
 * dtp:         datatype pool handle
 * obj_idx:     index of the created datatype inside the pool
 * val_start:   start value for initialization of buffer
 * val_stride:  increment between one buffer element and the following
 * val_count:   number of elements to initialize in buffer
 *
 * NOTE1: for pools created with `DTP_pool_create_struct` `val_count` is
 *        disregarded, `basic_type_counts` passed to the pool create
 *        function is used instead for buffer initialization.
 * NOTE2: for receive datatypes 'val_count' can be set to 0. Every buffer
 *        element in any case is initialized to 0 using 'memset'.
 *        'val_count' should be set to > 0 only for send datatypes.
 *
 */
int DTP_obj_create(DTP_t dtp, int obj_idx, int val_start, int val_stride, MPI_Aint val_count)
{
    int err = DTP_SUCCESS;
    int basic_type_count;
    int factor;
    int obj_type;
    struct DTPI_Par par;

    /* init user defined params */
    par.user.val_start = val_start;
    par.user.val_stride = val_stride;
    par.user.val_count = val_count;
    par.user.obj_idx = obj_idx;

    if (dtp->DTP_pool_type == DTP_POOL_TYPE__BASIC) {
        /* get type signature for pool */
        basic_type_count = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count;

        /* get biggest factor for count */
        factor = DTPI_Get_max_fact(basic_type_count);

        switch (obj_idx) {
            case DTPI_OBJ_LAYOUT_SIMPLE__BASIC:
            case DTPI_OBJ_LAYOUT_SIMPLE__CONTIG:
                par.core.type_count = basic_type_count;
                par.core.type_blklen = 1;
                par.core.type_stride = 1;
                break;
            case DTPI_OBJ_LAYOUT_SIMPLE__VECTOR:
            case DTPI_OBJ_LAYOUT_SIMPLE__HVECTOR:
            case DTPI_OBJ_LAYOUT_SIMPLE__INDEXED:
            case DTPI_OBJ_LAYOUT_SIMPLE__HINDEXED:
            case DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_HINDEXED:
                par.core.type_count = basic_type_count;
                par.core.type_blklen = 1;
                par.core.type_stride = 2;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_BLK__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                } else {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                }
                par.core.type_stride = par.core.type_blklen + 1;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_CNT__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                } else {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                }
                par.core.type_stride = par.core.type_blklen + 1;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                } else {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                }
                par.core.type_stride = par.core.type_blklen * 4;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                } else {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                }
                par.core.type_stride = par.core.type_blklen * 4;
                break;
            default:
                err = DTP_ERR_OTHER;
                fprintf(stdout, "Type layout %d is not defined\n", obj_idx);
                fflush(stdout);
                goto fn_exit;
        }

        /* get type of object for creator array */
        DTPI_GET_BASIC_OBJ_TYPE_FROM_IDX(obj_idx, obj_type);

        err = creators[obj_type] (&par, dtp);
    } else {
        switch (obj_idx) {
            case DTPI_OBJ_LAYOUT_SIMPLE__STRUCT:
                /* TODO: add other struct layouts here */
                break;
            default:
                err = DTP_ERR_OTHER;
                fprintf(stdout, "Type layout %d is not defined\n", obj_idx);
                fflush(stdout);
                goto fn_exit;
        }
        err = DTPI_Struct_create(&par, dtp);
    }

  fn_exit:
    return err;
}

/*
 * DTP_obj_free - free previously created datatype obj_idx
 */
int DTP_obj_free(DTP_t dtp, int obj_idx)
{
    int err = DTP_SUCCESS;
    int obj_type;
    DTPI_t *dtpi;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    obj_type = dtpi->obj_type;

    if (dtp->DTP_pool_type == DTP_POOL_TYPE__BASIC) {
        err = destructors[obj_type] (dtp, obj_idx);
    } else {
        err = DTPI_Struct_free(dtp, obj_idx);
    }

    return err;
}

/*
 * DTP_obj_buf_check - check receive buffer to very correctness
 *
 * dtp:         datatype pool handle
 * obj_idx:     index of the datatype buffer to be checked
 * val_start:   start value for checking of buffer
 * val_stride:  increment between one buffer element and the following
 * val_count:   number of elements to check in buffer
 *
 * NOTE: for pools created with `DTP_pool_create_struct` `val_count` is
 *       disregarded, `basic_type_counts` passed to the pool create
 *       function is used instead to check buffer.
 */
int DTP_obj_buf_check(DTP_t dtp, int obj_idx, int val_start, int val_stride, MPI_Aint val_count)
{
    int err = DTP_SUCCESS;
    struct DTPI_Par par;
    DTPI_t *dtpi;
    DTP_pool_type pool_type;
    DTPI_obj_type_e obj_type;

    par.user.val_start = val_start;
    par.user.val_count = val_count;
    par.user.val_stride = val_stride;
    par.user.obj_idx = obj_idx;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    obj_type = dtpi->obj_type;

    if (dtp->DTP_pool_type == DTP_POOL_TYPE__BASIC) {
        err = checkers[obj_type] (&par, dtp);
    } else {
        err = DTPI_Struct_check_buf(&par, dtp);
    }

    return err;
}

#ifndef HAVE_CUDA
int DTP_obj_create_cuda(DTP_t dtp, int obj_idx, int val_start, int val_stride, MPI_Aint val_count)
{
    return 0;
}

int DTP_obj_free_cuda(DTP_t dtp, int obj_idx)
{
    return 0;
}

int DTP_obj_buf_check_cuda(DTP_t dtp, int obj_idx, int val_start, int val_stride,
                           MPI_Aint val_count)
{
    return 0;
}
#else
int DTP_obj_create_cuda(DTP_t dtp, int obj_idx, int val_start, int val_stride, MPI_Aint val_count)
{
    int err = DTP_SUCCESS;
    DTPI_t *dtpi;
    MPI_Aint lb, extent;
    DTPI_obj_type_e obj_type;
    size_t size;
    void *cuda_buf;

    /* create dtp obj using host memory first */
    err = DTP_obj_create(dtp, obj_idx, val_start, val_stride, val_count);

    /* get dtp obj info */
    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    obj_type = dtpi->obj_type;

    /* get buffer size */
    lb = dtpi->type_lb;
    extent = dtpi->type_extent;
    size =
        (obj_type ==
         DTPI_OBJ_TYPE__BASIC) ? extent * dtp->DTP_obj_array[obj_idx].DTP_obj_count : lb + extent;

    /* allocate device memory */
    DTPI_OBJ_ALLOC_OR_FAIL_IMPL(cuda_buf, size, DTPI_MEM_TYPE__CUDA);

    /* copy host memory to device memory */
    DTPI_OBJ_MEMCPY_IMPL(cuda_buf, dtp->DTP_obj_array[obj_idx].DTP_obj_buf, size,
                         DTPI_MEM_TYPE__CUDA, DTPI_MEMCPY_HOST_TO_DEVICE);

    /* free host memory */
    DTPI_OBJ_FREE(dtp->DTP_obj_array[obj_idx].DTP_obj_buf);

    /* update dtp obj array to point to device memory */
    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = cuda_buf;

  fn_exit:
    return err;

  fn_fail:
    DTP_obj_free(dtp, obj_idx);
    goto fn_exit;
}

int DTP_obj_free_cuda(DTP_t dtp, int obj_idx)
{
    DTPI_OBJ_FREE_IMPL(dtp->DTP_obj_array[obj_idx].DTP_obj_buf, DTPI_MEM_TYPE__CUDA);
    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = NULL;
    return DTP_obj_free(dtp, obj_idx);
}

int DTP_obj_buf_check_cuda(DTP_t dtp, int obj_idx, int val_start, int val_stride,
                           MPI_Aint val_count)
{
    int err = DTP_SUCCESS;
    DTPI_t *dtpi;
    MPI_Aint lb, extent;
    DTPI_obj_type_e obj_type;
    size_t size;
    void *host_buf, *cuda_buf;

    /* get dtp obj info */
    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    obj_type = dtpi->obj_type;

    /* get buffer size */
    lb = dtpi->type_lb;
    extent = dtpi->type_extent;
    size =
        (obj_type ==
         DTPI_OBJ_TYPE__BASIC) ? extent * dtp->DTP_obj_array[obj_idx].DTP_obj_count : lb + extent;

    /* allocate host memory */
    DTPI_OBJ_ALLOC_OR_FAIL(host_buf, size);

    /* copy device memory to host memory */
    DTPI_OBJ_MEMCPY_IMPL(host_buf, dtp->DTP_obj_array[obj_idx].DTP_obj_buf, size,
                         DTPI_MEM_TYPE__CUDA, DTPI_MEMCPY_DEVICE_TO_HOST);

    /* update dtp obj array to point to host memory */
    cuda_buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = host_buf;

    /* check buffer */
    err = DTP_obj_buf_check(dtp, obj_idx, val_start, val_stride, val_count);

    /* restore original buffer */
    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = cuda_buf;

    /* free host memory */
    DTPI_OBJ_FREE(host_buf);

  fn_exit:
    return err;

  fn_fail:
    DTP_obj_free(dtp, obj_idx);
    goto fn_exit;
}
#endif

/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDIG_GPU_UTIL_H_INCLUDED
#define MPIDIG_GPU_UTIL_H_INCLUDED

#include "ch4r_buf.h"
#include "mpidig_gpu_types.h"

#define MPIDIG_GPU_REQUEST_RELEASE(req)                            \
    do {                                                           \
        void *gpu_stage_buf_ = MPIDIG_GPU_REQUEST(req, stage_buf); \
        if (gpu_stage_buf_) {                                      \
            if (req->kind == MPIR_REQUEST_KIND__SEND)              \
                MPIDIG_gpu_unstage_send_buf(gpu_stage_buf_);       \
            else if (req->kind == MPIR_REQUEST_KIND__RECV)         \
                MPIDIG_gpu_unstage_recv_buf(gpu_stage_buf_);       \
            MPIDIG_GPU_REQUEST(req, stage_buf) = NULL;             \
        }                                                          \
    } while (0)

MPL_STATIC_INLINE_PREFIX void *MPIDIG_gpu_get_buf(size_t size)
{
    void *buf = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_GET_BUF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_GET_BUF);
    MPIDIU_buf_pool_t **pools = MPIDIG_gpu_global.buf_pool;
    for (int i = 0; i < MPIDIG_GPU_BUF_POOL_NUM; i++) {
        if (size > pools[i]->size)
            continue;
        buf = MPIDIU_get_buf(pools[i]);
        goto fn_exit;
    }
    /* TODO: for now we simply allocate another buffer and return it
     * after registering the memory. Ideally, when the buffer is
     * released it should be added to a huge pool so that it can be
     * reused later on */
    MPIDIU_buf_t *tmp_buf;
    MPL_gpu_malloc_host((void **) &tmp_buf, size + sizeof(MPIDIU_buf_t));
    tmp_buf->next = NULL;
    tmp_buf->pool = NULL;
    buf = tmp_buf->data;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_GET_BUF);
    return buf;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_release_buf(void *buf)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_RELEASE_BUF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_RELEASE_BUF);
    MPIDIU_buf_t *cont = MPL_container_of(buf, MPIDIU_buf_t, data);
    if (cont->pool == NULL) {
        /* Free huge buffer right away */
        MPL_gpu_free_host(cont);
        goto fn_exit;
    }
    MPIDIU_release_buf(buf);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_RELEASE_BUF);
}

typedef struct MPIDIG_gpu_stage_ctx_t {
    bool is_send;
    union {
        struct {
            MPI_Aint true_lb;
        } send;

        struct {
            void *dst_buf;
            MPI_Aint count;
            MPI_Datatype datatype;
        } recv;
    } u;
} MPIDIG_gpu_stage_ctx_t;

/* MPIDIG_gpu_stage_send_buf - allocate stage buffer, copy source data into it, and return pointer */
MPL_STATIC_INLINE_PREFIX void *MPIDIG_gpu_stage_send_buf(const void *buf, MPI_Aint count,
                                                         MPI_Datatype datatype)
{
    char *tmp_buf = (char *) buf;
    int rc;
    int dt_contig;
    size_t size;
    MPIR_Datatype *dt_ptr;
    MPI_Aint true_lb;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, size, dt_ptr, true_lb);
    if (!dt_contig)
        size = (dt_ptr->extent * count) + (dt_ptr->true_ub - true_lb) - dt_ptr->extent;

    tmp_buf = MPIDIG_gpu_get_buf(size);
    yaksa_request_t request;
    uintptr_t actual_pack_bytes;
    rc = yaksa_ipack((const void *) buf, (uintptr_t) size, YAKSA_TYPE__BYTE, (uintptr_t) true_lb,
                     tmp_buf, (uintptr_t) size, &actual_pack_bytes, &request);
    MPIR_Assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    MPIR_Assert(rc == YAKSA_SUCCESS);

    tmp_buf -= true_lb;

    MPIDIG_gpu_stage_ctx_t *ctx = MPL_malloc(sizeof(*ctx), MPL_MEM_OTHER);
    MPIR_Assert(ctx != NULL);
    ctx->is_send = true;
    ctx->u.send.true_lb = true_lb;
    MPIDIU_map_set(MPIDIG_gpu_global.buf_map, (uint64_t) tmp_buf, ctx, MPL_MEM_OTHER);
    return tmp_buf;
}

/* MPIDIG_gpu_unstage_send_buf - free stage buffer */
MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_unstage_send_buf(const void *buf)
{
    MPIDIG_gpu_stage_ctx_t *ctx =
        (MPIDIG_gpu_stage_ctx_t *) MPIDIU_map_lookup(MPIDIG_gpu_global.buf_map, (uint64_t) buf);
    MPIDIG_gpu_release_buf((char *) buf + ctx->u.send.true_lb);
    MPL_free(ctx);
    MPIDIU_map_erase(MPIDIG_gpu_global.buf_map, (uint64_t) buf);
}

/* MPIDIG_gpu_stage_recv_buf - allocate stage buffer and return pointer */
MPL_STATIC_INLINE_PREFIX void *MPIDIG_gpu_stage_recv_buf(void *buf, MPI_Aint count,
                                                         MPI_Datatype datatype)
{
    char *tmp_buf = (char *) buf;
    int dt_contig;
    size_t size;
    MPIR_Datatype *dt_ptr;
    MPI_Aint true_lb;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, size, dt_ptr, true_lb);
    if (!dt_contig)
        size = (dt_ptr->extent * count) + (dt_ptr->true_ub - true_lb) - dt_ptr->extent;

    tmp_buf = MPIDIG_gpu_get_buf(size);
    tmp_buf -= true_lb;

    MPIDIG_gpu_stage_ctx_t *ctx = MPL_malloc(sizeof(*ctx), MPL_MEM_OTHER);
    MPIR_Assert(ctx != NULL);
    ctx->is_send = false;
    ctx->u.recv.dst_buf = buf;
    ctx->u.recv.count = count;
    ctx->u.recv.datatype = datatype;
    MPIDIU_map_set(MPIDIG_gpu_global.buf_map, (uint64_t) tmp_buf, ctx, MPL_MEM_OTHER);
    return tmp_buf;
}

/* MPIDIG_gpu_unstage_recv_buf - copy data from stage buffer to source buffer and free it */
MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_unstage_recv_buf(void *buf)
{
    int rc;
    MPIDIG_gpu_stage_ctx_t *ctx =
        (MPIDIG_gpu_stage_ctx_t *) MPIDIU_map_lookup(MPIDIG_gpu_global.buf_map, (uint64_t) buf);

    int dt_contig;
    size_t size;
    MPIR_Datatype *dt_ptr;
    MPI_Aint true_lb;

    MPIDI_Datatype_get_info(ctx->u.recv.count, ctx->u.recv.datatype, dt_contig, size, dt_ptr,
                            true_lb);
    if (!dt_contig)
        size = (dt_ptr->extent * ctx->u.recv.count) + (dt_ptr->true_ub - true_lb) - dt_ptr->extent;

    void *stage_buf = (char *) buf + true_lb;
    void *dst_buf = (char *) ctx->u.recv.dst_buf;

    yaksa_request_t request;
    rc = yaksa_iunpack((const void *) stage_buf, (uintptr_t) size, dst_buf,
                       (uintptr_t) size, YAKSA_TYPE__BYTE, (uintptr_t) true_lb, &request);
    MPIR_Assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    MPIR_Assert(rc == YAKSA_SUCCESS);

    MPIDIG_gpu_release_buf(stage_buf);
    MPL_free(ctx);
    MPIDIU_map_erase(MPIDIG_gpu_global.buf_map, (uint64_t) buf);
}

/* MPIDIG_gpu_cancel_stage_buf - release stage buffer in case of errors */
MPL_STATIC_INLINE_PREFIX void MPIDIG_gpu_cancel_stage_buf(void *buf)
{
    MPIDIG_gpu_stage_ctx_t *ctx =
        (MPIDIG_gpu_stage_ctx_t *) MPIDIU_map_lookup(MPIDIG_gpu_global.buf_map, (uint64_t) buf);
    if (ctx->is_send)
        MPIDIG_gpu_release_buf((char *) buf + ctx->u.send.true_lb);
    else {
        MPI_Aint true_lb, true_extent;
        MPIR_Type_get_true_extent_impl(ctx->u.recv.datatype, &true_lb, &true_extent);
        MPIDIG_gpu_release_buf((char *) buf + true_lb);
    }
    MPL_free(ctx);
    MPIDIU_map_erase(MPIDIG_gpu_global.buf_map, (uint64_t) buf);
}

#endif /* MPIDIG_GPU_UTIL_H_INCLUDED */

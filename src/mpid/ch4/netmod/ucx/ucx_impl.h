/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_IMPL_H_INCLUDED
#define UCX_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "ucx_types.h"
#include "mpidch4r.h"
#include "ch4_impl.h"

#include <ucs/type/status.h>

#define MPIDI_UCX_COMM(comm)     ((comm)->dev.ch4.netmod.ucx)
#define MPIDI_UCX_REQ(req)       ((req)->dev.ch4.netmod.ucx)
#define COMM_TO_INDEX(comm,rank) MPIDIU_comm_rank_to_pid(comm, rank, NULL, NULL)
#define MPIDI_UCX_COMM_TO_EP(comm,rank,vni_src,vni_dst) \
    MPIDI_UCX_AV(MPIDIU_comm_rank_to_av(comm, rank)).dest[vni_src][vni_dst]
#define MPIDI_UCX_AV_TO_EP(av,vni_src,vni_dst) MPIDI_UCX_AV((av)).dest[vni_src][vni_dst]

#define MPIDI_UCX_WIN(win) ((win)->dev.netmod.ucx)
#define MPIDI_UCX_WIN_INFO(win, rank) MPIDI_UCX_WIN(win).info_table[rank]

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_UCX_init_tag(MPIR_Context_id_t contextid, int source,
                                                     uint64_t tag)
{
    uint64_t ucp_tag = 0;
    ucp_tag = contextid;
    ucp_tag = (ucp_tag << MPIDI_UCX_SOURCE_SHIFT);
    ucp_tag |= source;
    ucp_tag = (ucp_tag << MPIDI_UCX_TAG_SHIFT);
    ucp_tag |= (MPIDI_UCX_TAG_MASK & tag);
    return ucp_tag;
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_UCX_tag_mask(int mpi_tag, int src)
{
    uint64_t tag_mask = 0xffffffffffffffff;
    MPIR_TAG_CLEAR_ERROR_BITS(tag_mask);
    if (mpi_tag == MPI_ANY_TAG)
        tag_mask &= ~MPIR_TAG_USABLE_BITS;

    if (src == MPI_ANY_SOURCE)
        tag_mask &= ~(MPIDI_UCX_SOURCE_MASK);

    return tag_mask;
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_UCX_recv_tag(int mpi_tag, int src,
                                                     MPIR_Context_id_t contextid)
{
    uint64_t ucp_tag = contextid;

    ucp_tag = (ucp_tag << MPIDI_UCX_SOURCE_SHIFT);
    if (src != MPI_ANY_SOURCE)
        ucp_tag |= (src & UCS_MASK(MPIDI_UCX_CONTEXT_RANK_BITS));
    ucp_tag = ucp_tag << MPIDI_UCX_TAG_SHIFT;
    if (mpi_tag != MPI_ANY_TAG)
        ucp_tag |= (MPIDI_UCX_TAG_MASK & mpi_tag);
    return ucp_tag;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_get_tag(uint64_t match_bits)
{
    return ((int) (match_bits & MPIDI_UCX_TAG_MASK));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_get_source(uint64_t match_bits)
{
    return ((int) ((match_bits & MPIDI_UCX_SOURCE_MASK) >> MPIDI_UCX_TAG_SHIFT));
}

#define MPIDI_UCX_CHK_STATUS(STATUS)                                    \
    do {                                                                \
        MPIR_ERR_CHKANDJUMP4((STATUS!=UCS_OK && STATUS!=UCS_INPROGRESS), \
                             mpi_errno,                                 \
                             MPI_ERR_OTHER,                             \
                             "**ucx_nm_status",                         \
                             "**ucx_nm_status %s %d %s %s",             \
                             __SHORT_FILE__,                            \
                             __LINE__,                                  \
                             __func__,                                    \
                             ucs_status_string(STATUS));                \
    } while (0)

#define MPIDI_UCX_CHK_REQUEST(_req)                                     \
    do {                                                                \
        MPIR_ERR_CHKANDJUMP4(UCS_PTR_IS_ERR(_req),                      \
                             mpi_errno,                                 \
                             MPI_ERR_OTHER,                             \
                             "**ucx_nm_rq_error",                       \
                             "**ucx_nm_rq_error %s %d %s %s",           \
                             __SHORT_FILE__,                            \
                             __LINE__,                                  \
                             __func__,                                    \
                             ucs_status_string(UCS_PTR_STATUS(_req)));  \
    } while (0)

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_is_reachable_win(MPIR_Win * win)
{
    /* UCX only enables HW RMA for create and allocate windows. */
    return win->create_flavor == MPI_WIN_FLAVOR_CREATE ||
        win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_is_reachable_target(int rank, MPIR_Win * win)
{
    /* zero win target does not have rkey. */
    return MPIDI_UCX_is_reachable_win(win) && MPIDI_UCX_WIN_INFO(win, rank).rkey != NULL;
}

/* This function implements netmod vci to vni(context) mapping.
 * It returns -1 if the vci does not have a mapping.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_vci_to_vni(int vci)
{
    return vci < MPIDI_UCX_global.num_vnis ? vci : -1;
}

/* vni mapping */
/* NOTE: concerned by the modulo? If we restrict num_vnis to power of 2,
 * we may get away with bit mask */
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_get_vni_src(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return MPIDI_vci_get_src(comm_ptr, rank, tag) % MPIDI_UCX_global.num_vnis;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_get_vni_dst(MPIR_Comm * comm_ptr, int rank, int tag)
{
    return MPIDI_vci_get_dst(comm_ptr, rank, tag) % MPIDI_UCX_global.num_vnis;
}

#endif /* UCX_IMPL_H_INCLUDED */

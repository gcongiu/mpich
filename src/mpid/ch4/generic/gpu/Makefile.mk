##
## Copyright (C) by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/generic/gpu

noinst_HEADERS   += src/mpid/ch4/generic/gpu/mpidig_gpu.h       \
                    src/mpid/ch4/generic/gpu/mpidig_gpu_utils.h \
                    src/mpid/ch4/generic/gpu/mpidig_gpu_types.h

mpi_core_sources += src/mpid/ch4/generic/gpu/mpidig_gpu_init.c    \
                    src/mpid/ch4/generic/gpu/mpidig_gpu_globals.c

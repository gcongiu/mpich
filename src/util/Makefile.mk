## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/util

mpi_core_sources +=   \
    src/util/mpir_assert.c     \
    src/util/mpir_cvars.c      \
    src/util/mpir_pmi.c        \
    src/util/mpir_thread.c     \
    src/util/mpir_handlemem.c  \
    src/util/mpir_strerror.c   \
    src/util/mpir_localproc.c  \
    src/util/mpir_netloc.c     \
    src/util/mpir_hwtopo.c

noinst_HEADERS +=   \
    src/util/mpir_nodemap.h

# include $(top_srcdir)/src/util/logging/Makefile.mk

if MAINTAINER_MODE
# normally built by autogen.sh, but this rebuild rule is here
$(top_srcdir)/src/util/cvar/mpir_cvars.c: $(top_srcdir)/maint/extractcvars
	( cd $(top_srcdir) && $(top_srcdir)/maint/extractcvars --dirs="`cat $(top_srcdir)/maint/cvardirs`")
endif MAINTAINER_MODE

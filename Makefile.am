# allow to use Host cuda functions in C/C++
DEF_INCLUDES = @CUDA_INCLUDES@ -I$(top_srcdir)/src -I$(top_srcdir)/src/sph

if WANT_JANSSON
JANSSON_INCLUDES= -I$(top_srcdir)/src/compat/jansson
else
JANSSON_INCLUDES=
endif

bin_PROGRAMS = ccminerGPU ccminerCPU

EXTRA_DIST = autogen.sh README.md README.txt LICENSE.txt \
            src/compat/gettimeofday.c src/compat/getopt/getopt_long.c src/miner.h

SUBDIRS = src/compat

ccminerCPU_SOURCES = src/api.cpp src/elist.h src/miner.h src/compat.h \
                     src/compat/inttypes.h src/compat/stdbool.h src/compat/unistd.h \
                     src/compat/sys/time.h src/compat/getopt/getopt.h \
                     src/crc32.c src/hefty1.c \
                     src/ccminer.cpp src/pools.cpp src/util.cpp src/bench.cpp src/bignum.cpp \
                     src/sph/bmw.c src/sph/blake.c src/sph/groestl.c src/sph/jh.c src/sph/keccak.c src/sph/skein.c \
                     src/sph/cubehash.c src/sph/echo.c src/sph/luffa.c src/sph/sha2.c src/sph/shavite.c src/sph/simd.c \
                     src/sph/hamsi.c src/sph/hamsi_helper.c src/sph/streebog.c \
                     src/sph/shabal.c src/sph/whirlpool.c src/sph/sha2big.c src/sph/haval.c \
                     src/sph/ripemd.c src/sph/sph_sha2.c src/nvsettings.cpp \
                     src/hashlog.cpp src/nvml.cpp src/stats.cpp src/sysinfos.cpp src/cuda.cpp \
                     src/equi/equi-stratum.cpp src/equi/equi.cpp src/equi/blake2/blake2bx.cpp \
                     src/verus/verusscan.cpp \
                     src/verus/verus_clhash.cpp src/verus/verus_clhash_portable.cpp src/verus/verus_hash.cpp \
                     src/verus/haraka.c src/verus/haraka_portable.c

if HAVE_NVML
nvml_defs = -DUSE_WRAPNVML
nvml_libs = -ldl
endif

ccminerGPU_SOURCES = $(ccminerCPU_SOURCES) src/cuda_checkhash.cu src/verus/verus.cu

ccminerGPU_LDFLAGS	= $(PTHREAD_FLAGS) @CUDA_LDFLAGS@
ccminerCPU_LDFLAGS	= $(ccminerGPU_LDFLAGS)

ccminerGPU_LDADD	= @LIBCURL@ @JANSSON_LIBS@ @PTHREAD_LIBS@ @WS2_LIBS@ @CUDA_LIBS@ @OPENMP_CFLAGS@ @LIBS@ $(nvml_libs)
ccminerCPU_LDADD	= $(ccminerGPU_LDADD)

ccminerGPU_CPPFLAGS = @LIBCURL_CPPFLAGS@ @OPENMP_CFLAGS@ $(CPPFLAGS) $(PTHREAD_FLAGS) -I$(top_srcdir)/src -mpclmul -msse4 -msse4.1 -msse4.2 -mssse3 -maes
ccminerGPU_CPPFLAGS += -static -fno-strict-aliasing $(JANSSON_INCLUDES) $(DEF_INCLUDES) $(nvml_defs)
ccminerCPU_CPPFLAGS = $(ccminerGPU_CPPFLAGS)

ccminerGPU_LDADD += -lcuda

nvcc_ARCH = -gencode=arch=compute_61,code=sm_61
nvcc_FLAGS = $(nvcc_ARCH) @CUDA_INCLUDES@ -I. @CUDA_CFLAGS@
nvcc_FLAGS += $(JANSSON_INCLUDES) --ptxas-options="-v"

.cu.o:
	$(NVCC) $(nvcc_FLAGS) --maxrregcount=96 -o $@ -c $<

gitclean:
	make distclean
	git clean -fxd
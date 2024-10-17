# allow to use Host cuda functions in C/C++
DEF_INCLUDES = @CUDA_INCLUDES@

if WANT_JANSSON
JANSSON_INCLUDES= -I$(top_srcdir)/compat/jansson
else
JANSSON_INCLUDES=
endif

EXTRA_DIST		= autogen.sh README.md README.txt LICENSE.txt \
			  compat/gettimeofday.c compat/getopt/getopt_long.c

SUBDIRS = compat

bin_PROGRAMS = ccminerGPU ccminerCPU

ccminerCPU_SOURCES	= api.cpp elist.h miner.h compat.h \
			  compat/inttypes.h compat/stdbool.h compat/unistd.h \
			  compat/sys/time.h compat/getopt/getopt.h \
			  crc32.c hefty1.c \
			  ccminer.cpp pools.cpp util.cpp bench.cpp bignum.cpp \
			  sph/bmw.c sph/blake.c sph/groestl.c sph/jh.c sph/keccak.c sph/skein.c \
			  sph/cubehash.c sph/echo.c sph/luffa.c sph/sha2.c sph/shavite.c sph/simd.c \
			  sph/hamsi.c sph/hamsi_helper.c sph/streebog.c \
			  sph/shabal.c sph/whirlpool.c sph/sha2big.c sph/haval.c \
			  sph/ripemd.c sph/sph_sha2.c nvsettings.cpp \
			  hashlog.cpp nvml.cpp stats.cpp sysinfos.cpp cuda.cpp \
			  equi/equi-stratum.cpp equi/equi.cpp equi/blake2/blake2bx.cpp \
			  verus/verusscan.cpp \
			  verus/verus_clhash.cpp verus/verus_clhash_portable.cpp verus/verus_hash.cpp \
			  verus/haraka.c verus/haraka_portable.c

if HAVE_NVML
nvml_defs = -DUSE_WRAPNVML
nvml_libs = -ldl
endif

ccminerGPU_SOURCES = $(ccminerCPU_SOURCES) cuda_checkhash.cu verus/verus.cu

ccminerGPU_LDFLAGS  = $(PTHREAD_FLAGS) @CUDA_LDFLAGS@
ccminerCPU_LDFLAGS  = $(ccminerGPU_LDFLAGS)

ccminerGPU_LDADD    = @LIBCURL@ @JANSSON_LIBS@ @PTHREAD_LIBS@ @WS2_LIBS@ @CUDA_LIBS@ @OPENMP_CFLAGS@ @LIBS@ $(nvml_libs)
ccminerCPU_LDADD    = $(ccminerGPU_LDADD)

ccminerGPU_CPPFLAGS = @LIBCURL_CPPFLAGS@ @OPENMP_CFLAGS@ $(CPPFLAGS) $(PTHREAD_FLAGS) -mpclmul -msse4 -msse4.1 -msse4.2 -mssse3 -maes
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

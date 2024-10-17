#!/bin/bash

# Simple script to create the Makefile and build

make distclean || echo clean

rm -f Makefile.in config.status

#aclocal && autoheader && automake --add-missing --gnu --copy && autoconf || echo done
#vs
autoreconf -i
#🤦‍♂️️
CUDA_CFLAGS="-O3 -Xcompiler -Wall -D_FORCE_INLINES" \
	./configure --with-nvml=libnvidia-ml.so

make ccminerGPU -j20

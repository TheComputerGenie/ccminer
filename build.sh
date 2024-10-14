#!/bin/bash

# Simple script to create the Makefile and build

# export PATH="$PATH:/usr/local/cuda/bin/"

make distclean || echo clean

rm -f Makefile.in
rm -f config.status

aclocal && autoheader && automake --add-missing --gnu --copy && autoconf || echo done

# CFLAGS="-O2" ./configure
# To change the cuda arch, edit Makefile.am and run ./build.sh

extracflags="-march=native -D_REENTRANT -falign-functions=16 -falign-jumps=16 -falign-labels=16"

CUDA_CFLAGS="-O3 -lineno -Xcompiler -Wall -D_FORCE_INLINES" \
	./configure CXXFLAGS="-O3 $extracflags -std=c++17" --with-cuda=/usr/local/cuda --with-nvml=libnvidia-ml.so


make -j 20

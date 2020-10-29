#!/bin/bash

# number of cpu cores
cores="$(grep -c ^processor /proc/cpuinfo )"

# exports
export SH_PREFIX=/opt/toolchains/dc/sh-elf
export ARM_PREFIX=/opt/toolchains/dc/arm-eabi
export KOS_ROOT=/usr/local/dc/kos
export KOS_BASE=$KOS_ROOT/kos
export KOS_PORTS=$KOS_ROOT/kos-ports

# install deps and setup directories
sudo dnf install subversion gcc gcc-c++ make automake autoconf m4 \
	bison elfutils-libelf-devel flex libtool texinfo gawk \
	latex2html git sed wget libpng-devel lyx libjpeg-turbo-devel \
	mpfr-devel gmp-devel isl-devel intltool zlib-devel diffutils patch

sudo mkdir -p $KOS_ROOT
sudo mkdir -p $SH_PREFIX/sh-elf/include
sudo mkdir -p $ARM_PREFIX/share
sudo chown -R $USER:$USER $ARM_PREFIX
sudo chown -R $USER:$USER $SH_PREFIX
sudo chown -R $USER:$USER $KOS_ROOT

# clone kos
git clone git://git.code.sf.net/p/cadcdev/kallistios kos
git clone --recursive git://git.code.sf.net/p/cadcdev/kos-ports kos-ports
cp -r kos $KOS_ROOT
rm -rf kos
cp -r kos-ports $KOS_ROOT
rm -rf kos-ports

# prepare
./download.sh
./unpack.sh
cd gcc-9.3.0
./contrib/download_prerequisites
cd .. && make patch

# build
make -j${cores} build-sh4-binutils
make -j${cores} build-sh4-gcc-pass1
make -j${cores} build-sh4-newlib-only
make -j${coreS} fixup-sh4-newlib
make -j${cores} build-sh4-gcc-pass2
make -j${coreS} build-arm-binutils
make -j${cores} build-arm-gcc

# env script
cp $KOS_BASE/doc/environ.sh.sample $KOS_BASE/environ.sh
sed -i 's/\/opt\/toolchains\/dc\/kos/\/usr\/local\/dc\/kos\/kos/g' $KOS_BASE/environ.sh
source $KOS_BASE/environ.sh

# build kos and kos-ports
pushd `pwd`
cd $KOS_BASE
make -j${cores}
cd $KOS_PORTS
./utils/build-all.sh
popd

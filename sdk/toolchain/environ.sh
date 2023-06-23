# KallistiOS environment variable settings
#
# This is a sample script. Configure to suit your setup. Some possible
# alternatives for the values below are included as an example.
#
# This script should be sourced in your current shell environment (probably
# by bashrc or something similar).
#

# Build architecture. Set the major architecture you'll be building for.
# The only option here is "dreamcast" as of KOS 2.0.0.
export KOS_ARCH="dreamcast"

# Build sub-architecture. If you need a particular sub-architecture, then set
# that here; otherwise use "pristine".
# Possible subarch options include:
#  "pristine" - a normal Dreamcast console or HKT-0120 devkit
#  "naomi" - a NAOMI or NAOMI 2 arcade board
export KOS_SUBARCH="pristine"

# KOS main base path
export KOS_BASE="/usr/local/dc/kos/kos"
export KOS_PORTS="${KOS_BASE}/../kos-ports"

# Make utility
export KOS_MAKE="make"
#export KOS_MAKE="gmake"

# CMake toolchain
export KOS_CMAKE_TOOLCHAIN="${KOS_BASE}/utils/cmake/dreamcast.toolchain.cmake"

# Load utility
export KOS_LOADER="dc-tool -x"				# dcload, preconfigured
# export KOS_LOADER="dc-tool-ser -t /dev/ttyS0 -x"	# dcload-serial

# Genromfs utility
export KOS_GENROMFS="${KOS_BASE}/utils/genromfs/genromfs"
#export KOS_GENROMFS="genromfs"

# Compiler prefixes
#export KOS_CC_BASE="/usr/local/dc/dc-elf"
#export KOS_CC_PREFIX="dc"
export KOS_CC_BASE="/opt/toolchains/dc/sh-elf"		# DC
export KOS_CC_PREFIX="sh-elf"

# If you are compiling for DC and have an ARM compiler, use these too.
# If you're using a newer compiler (GCC 4.7.0 and newer), you should probably be
# using arm-eabi as the target, rather than arm-elf. dc-chain now defaults to
# arm-eabi, so that's the default here.
#export DC_ARM_BASE="/usr/local/dc/arm-elf"
#export DC_ARM_PREFIX="arm-elf"
export DC_ARM_BASE="/opt/toolchains/dc/arm-eabi"
export DC_ARM_PREFIX="arm-eabi"

# Expand PATH if not already set (comment out if you don't want this done here)
if [[ ":$PATH:" != *":${KOS_CC_BASE}/bin:/opt/toolchains/dc/bin:"* ]]; then
  export PATH="${PATH}:${KOS_CC_BASE}/bin:/opt/toolchains/dc/bin"
fi

# reset some options because there's no reason for them to persist across
# multiple sourcing of this
export KOS_INC_PATHS=""
export KOS_CFLAGS=""
export KOS_CPPFLAGS=""
export KOS_LDFLAGS=""
export KOS_AFLAGS=""
export DC_ARM_LDFLAGS=""

# Setup some default CFLAGS for compilation. The things that will go here
# are user specifyable, like optimization level and whether you want stack
# traces enabled. Some platforms may have optimization restrictions,
# please check README.
# GCC seems to have made -fomit-frame-pointer the default on many targets, so
# hence you may need -fno-omit-frame-pointer to actually have GCC spit out frame
# pointers. It won't hurt to have it in there either way.
# Link-time optimizations can be enabled by adding -flto. It however requires a
# recent toolchain (GCC 10+), and has not been thoroughly tested.
export KOS_CFLAGS="-O2 -fomit-frame-pointer"
# export KOS_CFLAGS="-O2 -DFRAME_POINTERS -fno-omit-frame-pointer"

# Comment out this line to enable GCC to use its own builtin implementations of 
# certain standard library functions. Under certain conditions, this can allow
# compiler-optimized implementations to replace standard function invocations.
# The downside of this is that it COULD interfere with Newlib or KOS implementations
# of these functions, and it has not been tested thoroughly to ensure compatibility. 
export KOS_CFLAGS="${KOS_CFLAGS} -fno-builtin"

# Uncomment this line to enable the optimized fast-math instructions (FSSRA,
# FSCA, and FSQRT) for calculating sin/cos, inverse square root, and square roots.
# These can result in substantial performance gains for these kinds of operations;
# however, they do so at the price of accuracy and are not IEEE compliant.
# NOTE: This also requires -fno-builtin be removed from KOS_CFLAGS to take effect!
# export KOS_CFLAGS="${KOS_CFLAGS} -ffast-math -ffp-contract=fast -mfsrra -mfsca"

# Everything else is pretty much shared. If you want to configure compiler
# options or other such things, look at this file.
. ${KOS_BASE}/environ_base.sh

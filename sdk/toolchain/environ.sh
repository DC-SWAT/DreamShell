# KallistiOS Environment Settings
#
# This is a sample script for configuring and customizing your
# KOS build environment. Modify it to suit your setup. Several
# settings may be enabled optionally or are provided with
# alternative values.
#
# This script is typically sourced in your current shell environment
# (probably by .bashrc, .bash_profile, or something similar), so that
# the KOS environment is set up automatically for each shell session.
#

# Build Architecture
#
# Set the major architecture you'll be building for.
# The only option here is "dreamcast" as of KOS 2.0.0.
#
export KOS_ARCH="dreamcast"

# Build Sub-Architecture
#
# Defines the sub architecture your configuration
# is targeting or uses an existing value that
# can be set externally via your IDE.
#
# Valid values:
#   "pristine" - Dreamcast console or HKT-0120 devkit (default)
#   "naomi"    - NAOMI or NAOMI 2 arcade board
#
if [ -z "${KOS_SUBARCH}" ] ; then
    export KOS_SUBARCH="pristine"
else
    export KOS_SUBARCH
fi

# KOS Root Path
#
# Specifies the path to the KOS root directory
#
export KOS_BASE="/usr/local/dc/kos/kos"

# KOS-Ports Path
#
# Specifies the path to the KOS-ports directory
#
export KOS_PORTS="${KOS_BASE}/../kos-ports"

# SH Compiler Prefixes
#
# Specifies the path to and prefix for the main SH
# GCC toolchain used to target the Dreamcast's SH4 CPU.
#
export KOS_CC_BASE="/opt/toolchains/dc/sh-elf"
export KOS_CC_PREFIX="sh-elf"

# ARM Compiler Prefixes
#
# Specifies the path to and prefix for the additional ARM
# GCC toolchain used to target the Dreamcast's AICA SPU.
#
export DC_ARM_BASE="/opt/toolchains/dc/arm-eabi"
export DC_ARM_PREFIX="arm-eabi"

# External Dreamcast Tools Path
#
# Specifies the path where Dreamcast tools that are not part of KOS are to be
# installed. This includes, for example, dc-tool-ip, dc-tool-serial, and the
# mrbc bytecode compiler. This directory, along with SH and ARM compiler
# toolchains, will be added to your PATH environment variable.
#
export DC_TOOLS_BASE="/opt/toolchains/dc/bin"

# CMake Toolchain Path
#
# Specifies the path to the toolchain file used to target
# KOS with the "cmake" build utility.
#
export KOS_CMAKE_TOOLCHAIN="${KOS_BASE}/utils/cmake/dreamcast.toolchain.cmake"

# Genromfs Utility Path
#
# Specifies the path to the utility which is used by KOS
# to create romdisk filesystem images.
#
export KOS_GENROMFS="${KOS_BASE}/utils/genromfs/genromfs"

# Make Utility
#
# Configures the tool to be used as the main "make" utility
# for building GNU Makefiles. On a platform such as BSD,
# the default can be changed to "gmake," for the GNU
# implementation.
#
export KOS_MAKE="make"
#export KOS_MAKE="gmake"

# Loader Utility
#
# Specifies the loader to be used with the "make run" targets
# in the KOS examples. Defaults to using a preconfigured version
# of dc-tool. Use one of the other options for a manual dc-tool-ip
# or dc-tool-serial configuration, remembering to change the values
# for the Dreamcast's IP address or the serial port interface.
#
export KOS_LOADER="${KOS_BASE}/ds/sdk/bin/dc-tool-ip -t 192.168.1.252 -x"
#export KOS_LOADER="dc-tool -x"
#export KOS_LOADER="dc-tool-ip -t 192.168.1.100 -x"
#export KOS_LOADER="dc-tool-ser -t /dev/ttyS0 -x"

# Default Compiler Flags
#
# Resets build flags. You can also initialize them to some preset
# default values here if you wish.
#
export KOS_INC_PATHS=""
export KOS_CFLAGS=""
export KOS_CPPFLAGS=""
export KOS_LDFLAGS=""
export KOS_AFLAGS=""
export DC_ARM_LDFLAGS=""

# Debug Builds
#
# Controls whether to disable additional debugging checks and assertions,
# such as for parameter validation or internal errors. Uncomment this if
# you do not wish to compile with this additional logic enabled.
#
#export KOS_CFLAGS="${KOS_CFLAGS} -DNDEBUG"

# Optimization Level
#
# Controls the baseline optimization level to use when building.
# Typically this is -Og (debugging), -O0, -01, -02, or -03.
# NOTE: For our target, -O4 is a valid optimization level that has
# been seen to have some performance impact as well.
#
export KOS_CFLAGS="${KOS_CFLAGS} -O2"

# Additional Optimizations
#
# Uncomment this to enable what has been found empirically to be
# the optimal set of additional flags for release build performance
# on the current stable toolchain. NOTE: Certain KOS-ports and examples
# do not work properly with "-flto=auto"!
#
#export KOS_CFLAGS="${KOS_CFLAGS} -freorder-blocks-algorithm=simple -flto=auto"

# Frame Pointers
#
# Controls whether frame pointers are emitted or not. Disabled by
# default. Enable them if you plan to use GDB for debugging.
#
export KOS_CFLAGS="${KOS_CFLAGS} -fomit-frame-pointer"
#export KOS_CFLAGS="${KOS_CFLAGS} -fno-omit-frame-pointer -DFRAME_POINTERS"

# Stack Protector
#
# Controls whether GCC emits extra code to check for buffer overflows or stack
# smashing, which can be very useful for debugging. -fstack-protector only
# covers vulnerable objects, while -fstack-protector-strong provides medium
# coverage, and -fstack-protector-all provides full coverage. You may also
# override the default stack excepton handler by providing your own
# implementation of "void __stack_chk_fail(void)."
#
#export KOS_CFLAGS="${KOS_CFLAGS} -fstack-protector-all"

# GCC Builtin Functions
#
# Comment out this line to enable GCC to use its own builtin implementations of
# certain standard library functions. Under certain conditions, this can allow
# compiler-optimized implementations to replace standard function invocations.
# The downside of this is that it COULD interfere with Newlib or KOS implementations
# of these functions, and it has not been tested thoroughly to ensure compatibility.
#
export KOS_CFLAGS="${KOS_CFLAGS} -fno-builtin"

# Fast Math Instructions
#
# Uncomment this line to enable the optimized fast-math instructions (FSSRA,
# FSCA, and FSQRT) for calculating sin/cos, inverse square root, and square roots.
# These can result in substantial performance gains for these kinds of operations;
# however, they do so at the price of accuracy and are not IEEE compliant.
# NOTE: Enabling this option will also override -fno-builtin!
#
#export KOS_CFLAGS="${KOS_CFLAGS} -fbuiltin -ffast-math -ffp-contract=fast -mfsrra -mfsca"

# SH4 Floating Point Arithmetic Precision
#
# KallistiOS only officially supports the single-precision-only floating-point
# arithmetic mode (-m4-single-only), but double precision, single default
# (-m4-single) or double precision default (-m4) modes can be enabled here by
# adjusting KOS_SH4_PRECISION.
# WARNING: Adjusting this setting has a high likelihood of breaking KallistiOS,
#          kos-ports, and existing codebases which assume -m4-single-only.
#          Do not touch this setting unless you know what you are doing!
# NOTE: Altering this setting also requires your toolchain to have been built
#       with support for these modes, which is not the case by default!
#
export KOS_SH4_PRECISION="-m4-single-only"

# Use LRA (Local Register Allocator) Pass
#
# Uncomment this line to use the modern Local Register Allocator pass during
# code generation instead of the default older reload pass. This option is
# known to be unstable or less performant for SH at this time, but will likely
# become mandatory in future versions of GCC, so feel free to help us test.
# Only enable this setting if you understand what you are doing!
#
#export KOS_CFLAGS="${KOS_CFLAGS} -mlra"

# Shared Compiler Configuration
#
# Include sub architecture-independent configuration file for shared
# environment settings. If you want to configure additional compiler
# options or see where other build flags are set, look at this file.
#
. ${KOS_BASE}/environ_base.sh

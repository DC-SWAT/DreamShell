#!/bin/sh

# These version numbers are all that should ever have to be changed.
export GCC_VER=5.2.0
export BINUTILS_VER=2.25
export NEWLIB_VER=2.2.0
export GMP_VER=6.1.0
export MPFR_VER=3.1.4
export MPC_VER=1.0.3

while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    case $PARAM in
        --no-gmp)
            unset GMP_VER
            ;;
        --no-mpfr)
            unset MPFR_VER
            ;;
        --no-mpc)
            unset MPC_VER
            ;;
        --no-deps)
            unset GMP_VER
            unset MPFR_VER
            unset MPC_VER
            ;;
        *)
            echo "ERROR: unknown parameter \"$PARAM\""
            exit 1
            ;;
    esac
    shift
done

# Download everything.
if command -v wget >/dev/null 2>&1; then
    echo "Downloading binutils-$BINUTILS_VER..."
    wget -c ftp://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.bz2 || exit 1
    echo "Downloading GCC $GCC_VER..."
    wget -c ftp://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.bz2 || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    wget -c ftp://sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.gz || exit 1

    if [ -n "$GMP_VER" ]; then
        echo "Downloading GMP $GMP_VER..."
        wget -c ftp://gcc.gnu.org/pub/gcc/infrastructure/gmp-$GMP_VER.tar.bz2 || exit 1
    fi

    if [ -n "$MPFR_VER" ]; then
        echo "Downloading MPFR $MPFR_VER..."
        wget -c ftp://gcc.gnu.org/pub/gcc/infrastructure/mpfr-$MPFR_VER.tar.bz2 || exit 1
    fi

    if [ -n "$MPC_VER" ]; then
        echo "Downloading MPC $MPC_VER..."
        wget -c ftp://gcc.gnu.org/pub/gcc/infrastructure/mpc-$MPC_VER.tar.gz || exit 1
    fi
elif command -v curl >/dev/null 2>&1; then
    echo "Downloading Binutils $BINUTILS_VER..."
    curl -C - -O ftp://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.bz2 || exit 1
    echo "Downloading GCC $GCC_VER..."
    curl -C - -O ftp://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.bz2 || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    curl -C - -O ftp://sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.gz || exit 1

    if [ -n "$GMP_VER" ]; then
        echo "Downloading GMP $GMP_VER..."
        curl -C - -O ftp://gcc.gnu.org/pub/gcc/infrastructure/gmp-$GMP_VER.tar.bz2 || exit 1
    fi

    if [ -n "$MPFR_VER" ]; then
        echo "Downloading MPFR $MPFR_VER..."
        curl -C - -O ftp://gcc.gnu.org/pub/gcc/infrastructure/mpfr-$MPFR_VER.tar.bz2 || exit 1
    fi

    if [ -n "$MPC_VER" ]; then
        echo "Downloading MPC $MPC_VER..."
        curl -C - -O ftp://gcc.gnu.org/pub/gcc/infrastructure/mpc-$MPC_VER.tar.gz || exit 1
    fi
else
    echo >&2 "You must have either wget or cURL installed to use this script!"
    exit 1
fi

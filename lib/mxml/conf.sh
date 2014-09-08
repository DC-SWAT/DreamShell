#!/bin/sh

./configure --prefix="${KOS_BASE}/ds/lib" --docdir="${KOS_BASE}/ds//sdk/doc/mxml" --with-docdir="${KOS_BASE}/ds/sdk/doc/mxml" --disable-shared --host=sh-elf CC="${KOS_CC}" AR="${KOS_AR}" RANLIB="${KOS_RUNLIB}" ARFLAGS="" CFLAGS="${KOS_CFLAGS}" LDFLAGS="${KOS_LDFLAGS}" LIBS="${KOS_LIBS}"



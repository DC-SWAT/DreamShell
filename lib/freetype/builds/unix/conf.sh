
./configure --host=sh-elf --libdir=${KOS_BASE}/ds/lib --includedir=${KOS_BASE}/ds/include --disable-shared \
			CFLAGS="${KOS_CFLAGS}" LDFLAGS="${KOS_LDFLAGS}" LIBS="${KOS_LIBS}" CC=${KOS_CC} AR=${KOS_AR} RUNLIB=${KOS_RUNLIB}

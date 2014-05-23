
./configure --host=sh-elf CFLAGS="${KOS_CFLAGS} -I${KOS_BASE}/ds/include" LDFLAGS="${KOS_LDFLAGS} -L${KOS_BASE}/ds/lib" LIBS="${KOS_LIBS} -lm" CC="${KOS_CC}" --disable-pthread --disable-shared #--disable-assembly

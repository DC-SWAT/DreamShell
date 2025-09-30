#
# ffmpeg library config for DreamShell
# Copyright (C)2011-2025 SWAT
# http://www.dc-swat.ru
#
cd ./ffmpeg-0.6.3
# cd ./FFmpeg
./configure \
	--malloc-prefix=ds_ \
	--disable-debug --disable-network --disable-swscale --enable-small \
	--disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-encoders --disable-avdevice \
	--disable-protocols --disable-devices --disable-filters --disable-muxers --disable-bsfs --disable-hwaccels \
  	--disable-amd3dnow --disable-amd3dnowext --disable-mmx --disable-mmx2 --disable-sse --disable-ssse3 \
	--disable-armv5te --disable-armv6 --disable-armv6t2 --disable-armvfp --disable-iwmmxt --disable-mmi \
	--disable-neon --disable-vis --disable-yasm --disable-altivec --disable-golomb --disable-lpc \
  	--disable-vaapi --disable-vdpau --disable-dxva2 --disable-decoders --disable-demuxers --disable-parsers \
	\
	--enable-gpl --prefix=../ --enable-cross-compile --cross-prefix=sh-elf- --arch=sh4 --target-os=gnu \
	--extra-cflags="${KOS_CFLAGS} -Wno-redundant-decls -Wno-attributes -Wno-discarded-qualifiers -Wno-strict-aliasing -Wno-maybe-uninitialized -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-incompatible-pointer-types -Wno-format -I${KOS_BASE}/ds/include -I${KOS_BASE}/ds/include/zlib" \
	--extra-ldflags="${KOS_LDFLAGS} -L${KOS_BASE}/ds/sdk/lib ${KOS_LIBS}" --enable-zlib \
	\
	--enable-decoder=mpeg4 --enable-decoder=roq \
	--enable-decoder=mjpeg --enable-decoder=mjpegb \
	--enable-decoder=ac3 --enable-decoder=aac --enable-decoder=adpcm_ms \
	\
	--enable-demuxer=avi \
	--enable-demuxer=mjpeg --enable-demuxer=roq \
	--enable-demuxer=ac3 --enable-demuxer=aac \
	--enable-demuxer=wav

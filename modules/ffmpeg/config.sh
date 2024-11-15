#
# ffmpeg library config for DreamShell
# Copyright (C)2011-2020, 2024 SWAT
# http://www.dc-swat.ru
#
cd ./ffmpeg-0.6.3
./configure \
	--disable-debug --disable-network --disable-swscale --enable-small \
	--disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-encoders --disable-avdevice \
	--disable-protocols --disable-devices --disable-filters --disable-muxers --disable-bsfs --disable-hwaccels \
  	--disable-amd3dnow --disable-amd3dnowext --disable-mmx --disable-mmx2 --disable-sse --disable-ssse3 \
	--disable-armv5te --disable-armv6 --disable-armv6t2 --disable-armvfp --disable-iwmmxt --disable-mmi \
	--disable-neon --disable-vis --disable-yasm --disable-altivec --disable-golomb --disable-lpc \
  	--disable-vaapi --disable-vdpau --disable-dxva2 --disable-decoders --disable-demuxers \
	\
	--enable-gpl --prefix=../ --enable-cross-compile --cross-prefix=sh-elf- --arch=sh4 --target-os=gnu \
	--extra-cflags="${KOS_CFLAGS} -Wno-incompatible-pointer-types -I${KOS_BASE}/ds/include -I${KOS_BASE}/ds/include/zlib -I${KOS_BASE}/ds/sdk/include/freetype -I${KOS_BASE}/ds/modules/ogg/liboggvorbis/liboggvorbis/libvorbis/include -I${KOS_BASE}/ds/modules/ogg/liboggvorbis/liboggvorbis/libogg/include" \
	--extra-ldflags="${KOS_LDFLAGS} -L${KOS_BASE}/ds/sdk/lib ${KOS_LIBS}" --enable-zlib --enable-bzlib --enable-libvorbis \
	\
	--enable-decoder=h264 --enable-decoder=mpeg2video --enable-decoder=mpeg4 --enable-decoder=msmpeg4v1 --enable-decoder=ape \
	--enable-decoder=mpeg1video --enable-decoder=ac3 --enable-decoder=eac3 --enable-decoder=vp6 --enable-decoder=vp5 --enable-decoder=vp3 \
	--enable-decoder=aac --enable-decoder=eac3 --enable-decoder=flac --enable-decoder=flic --enable-decoder=mjpeg --enable-decoder=mjpegb \
	--enable-decoder=mpegvideo --enable-decoder=theora --enable-decoder=dca --enable-decoder=h261  --enable-decoder=flv \
	--enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=h263 --enable-decoder=wmv1 --enable-decoder=wmv2 --enable-decoder=wmv3 \
	--enable-decoder=pcm_s16be --enable-decoder=pcm_s16le --enable-decoder=pcm_u16be --enable-decoder=pcm_u16le \
	--enable-decoder=adpcm_yamaha --enable-decoder=adpcm_ms \
	\
	--enable-demuxer=avi --enable-demuxer=ac3 --enable-demuxer=mp3 --enable-demuxer=mpegts --enable-demuxer=mpegvideo --enable-demuxer=matroska \
	--enable-demuxer=h264 --enable-demuxer=m4v --enable-demuxer=aac --enable-demuxer=mjpeg \
	--enable-demuxer=h261 --enable-demuxer=pcm_u16le --enable-demuxer=pcm_s16be --enable-demuxer=pcm_s16le --enable-demuxer=pcm_u16be \
	--enable-demuxer=ape --enable-demuxer=aiff --enable-demuxer=ogg \
	--enable-demuxer=eac3 --enable-demuxer=rm --enable-demuxer=avisynth \
	--enable-demuxer=flac --enable-demuxer=flic --enable-decoder=dts --enable-demuxer=h263 --enable-demuxer=flv \
	--enable-demuxer=dv --enable-demuxer=vc1
	
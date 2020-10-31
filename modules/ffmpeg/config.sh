#
# ffmpeg library config for DreamShell
# Copyright (C)2011-2020 SWAT
# http://www.dc-swat.ru
#
cd ./ffmpeg-0.6.3
./configure \
	--disable-debug --disable-network --disable-swscale \
	--disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-encoders --disable-avdevice \
	--disable-protocols --disable-devices --disable-filters --disable-muxers --disable-bsfs --disable-hwaccels \
  	--disable-amd3dnow --disable-amd3dnowext --disable-mmx --disable-mmx2 --disable-sse --disable-ssse3 \
	--disable-armv5te --disable-armv6 --disable-armv6t2 --disable-armvfp --disable-iwmmxt --disable-mmi \
	--disable-neon --disable-vis --disable-yasm --disable-altivec --disable-golomb --disable-lpc \
  	--disable-vaapi --disable-vdpau --disable-dxva2 --disable-decoders --disable-demuxers \
	\
	--enable-gpl --prefix=../ --enable-cross-compile --cross-prefix=sh-elf- --arch=sh4 --target-os=gnu \
	--extra-cflags="${KOS_CFLAGS} -I${KOS_BASE}/ds/include -I${KOS_BASE}/ds/include/zlib -I${KOS_BASE}/ds/sdk/include/freetype -I${KOS_BASE}/ds/modules/ogg/liboggvorbis/liboggvorbis/libvorbis/include -I${KOS_BASE}/ds/modules/ogg/liboggvorbis/liboggvorbis/libogg/include" \
	--extra-ldflags="${KOS_LDFLAGS} -L${KOS_BASE}/ds/sdk/lib ${KOS_LIBS}" --enable-zlib --enable-bzlib --enable-libvorbis --enable-libxvid \
	\
	--enable-decoder=h264 --enable-decoder=mpeg2video --enable-decoder=mpeg4 --enable-decoder=msmpeg4v1 --enable-decoder=ape \
	--enable-decoder=mpeg1video --enable-decoder=ac3 --enable-decoder=eac3 --enable-decoder=vp6 --enable-decoder=vp5 --enable-decoder=vp3 \
	--enable-decoder=aac --enable-decoder=eac3 --enable-decoder=flac --enable-decoder=flic --enable-decoder=mjpeg --enable-decoder=mjpegb \
	--enable-decoder=adpcm_4xm --enable-decoder=adpcm_adx --enable-decoder=mpegvideo --enable-decoder=theora --enable-decoder=dca \
	--enable-decoder=flac --enable-decoder=h261  --enable-decoder=flv --enable-decoder=cinepak --enable-decoder=bink --enable-decoder=truehd \
	--enable-decoder=msmpeg4v2 --enable-decoder=msmpeg4v3 --enable-decoder=h263 --enable-decoder=wmv1 --enable-decoder=wmv2 --enable-decoder=wmv3 \
	--enable-decoder=pcm_s16be --enable-decoder=pcm_s16le --enable-decoder=pcm_u16be --enable-decoder=pcm_u16le \
	\
	--enable-demuxer=avi --enable-demuxer=ac3 --enable-demuxer=mp3 --enable-demuxer=mpegts --enable-demuxer=mpegvideo --enable-demuxer=matroska \
	--enable-demuxer=h264 --enable-demuxer=asf --enable-demuxer=m4v --enable-demuxer=aac --enable-demuxer=mjpeg \
	--enable-demuxer=h261 --enable-demuxer=pcm_u16le --enable-demuxer=pcm_s16be --enable-demuxer=pcm_s16le --enable-demuxer=pcm_u16be \
	--enable-demuxer=eac3 --enable-demuxer=ape --enable-demuxer=aiff --enable-demuxer=asf --enable-demuxer=ogg \
	--enable-demuxer=voc --enable-demuxer=eac3 --enable-demuxer=rm --enable-demuxer=avisynth \
	--enable-demuxer=flac --enable-demuxer=flic --enable-decoder=dts --enable-demuxer=h263 --enable-demuxer=flv \
	--enable-demuxer=dv --enable-demuxer=truehd --enable-demuxer=vc1 --enable-demuxer=bink --enable-demuxer=nc
	
	#  --enable-small --ar=kos-ar --as=kos-as --cc=kos-cc --ld=kos-ld \
	#  --enable-decoder=adpcm_ms --enable-demuxer=pcm_u32le --enable-demuxer=pcm_f32le --enable-demuxer=pcm_s24be --enable-demuxer=pcm_s24le
	#  --enable-decoder=pcm_f32be --enable-decoder=pcm_f32le --enable-decoder=pcm_s24le --enable-decoder=pcm_s32be --enable-decoder=pcm_s32le 
	#  --enable-decoder=pcm_s24be --enable-decoder=pcm_u24be --enable-decoder=pcm_u24le --enable-decoder=pcm_u32be --enable-decoder=pcm_u32le
	#  --enable-demuxer=pcm_u24be --enable-demuxer=pcm_u24le --enable-demuxer=pcm_u32be --enable-demuxer=pcm_s32be --enable-demuxer=pcm_s32le 
	#  --enable-demuxer=pcm_f32be
	#  --disable-mdct --disable-rdft --disable-fft  --enable-decoder=mp3 --enable-decoder=mp2 --enable-decoder=mp1 --enable-decoder=vorbis 
	# 
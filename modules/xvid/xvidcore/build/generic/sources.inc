SRC_DIR = ../../src

SRC_GENERIC = \
 	decoder.c \
	encoder.c \
	xvid.c \
	bitstream/bitstream.c \
	bitstream/cbp.c \
	bitstream/mbcoding.c \
	dct/fdct.c \
	dct/idct.c \
	dct/simple_idct.c \
	image/colorspace.c \
	image/image.c \
	image/interpolate8x8.c \
	image/font.c \
	image/postprocessing.c \
	image/qpel.c \
	image/reduced.c \
	motion/estimation_bvop.c \
	motion/estimation_common.c \
	motion/estimation_gmc.c \
	motion/estimation_pvop.c \
	motion/estimation_rd_based.c \
	motion/estimation_rd_based_bvop.c \
	motion/gmc.c \
	motion/motion_comp.c \
	motion/vop_type_decision.c \
	motion/sad.c \
	prediction/mbprediction.c \
	plugins/plugin_single.c \
	plugins/plugin_2pass1.c \
	plugins/plugin_2pass2.c \
	plugins/plugin_lumimasking.c \
	plugins/plugin_dump.c \
	plugins/plugin_psnr.c \
	plugins/plugin_ssim.c \
	plugins/plugin_psnrhvsm.c \
	quant/quant_h263.c \
	quant/quant_matrix.c \
	quant/quant_mpeg.c \
	utils/emms.c \
	utils/mbtransquant.c \
	utils/mem_align.c \
	utils/mem_transfer.c \
	utils/timer.c

SRC_IA32 = \
	bitstream/x86_asm/cbp_mmx.asm \
	bitstream/x86_asm/cbp_sse2.asm \
	dct/x86_asm/fdct_mmx_ffmpeg.asm \
	dct/x86_asm/fdct_mmx_skal.asm \
	dct/x86_asm/fdct_sse2_skal.asm \
	dct/x86_asm/idct_3dne.asm \
	dct/x86_asm/idct_mmx.asm \
	dct/x86_asm/idct_sse2_dmitry.asm \
	image/x86_asm/colorspace_rgb_mmx.asm \
	image/x86_asm/colorspace_yuv_mmx.asm \
	image/x86_asm/colorspace_yuyv_mmx.asm \
	image/x86_asm/interpolate8x8_3dn.asm \
	image/x86_asm/interpolate8x8_3dne.asm \
	image/x86_asm/interpolate8x8_mmx.asm \
	image/x86_asm/interpolate8x8_xmm.asm \
	image/x86_asm/postprocessing_mmx.asm \
	image/x86_asm/postprocessing_sse2.asm \
	image/x86_asm/reduced_mmx.asm \
	image/x86_asm/qpel_mmx.asm \
	image/x86_asm/gmc_mmx.asm \
	image/x86_asm/deintl_sse.asm \
	motion/x86_asm/sad_xmm.asm \
	motion/x86_asm/sad_sse2.asm \
	motion/x86_asm/sad_mmx.asm \
	motion/x86_asm/sad_3dne.asm \
	motion/x86_asm/sad_3dn.asm \
	quant/x86_asm/quantize_h263_mmx.asm \
	quant/x86_asm/quantize_h263_3dne.asm \
	quant/x86_asm/quantize_mpeg_xmm.asm \
	quant/x86_asm/quantize_mpeg_mmx.asm \
	utils/x86_asm/mem_transfer_mmx.asm \
	utils/x86_asm/mem_transfer_3dne.asm \
	utils/x86_asm/interlacing_mmx.asm \
	utils/x86_asm/cpuid.asm \
	plugins/x86_asm/plugin_ssim-a.asm

SRC_X86_64 = $(SRC_IA32)

SRC_IA64 = \
	dct/ia64_asm/fdct_ia64.s \
	image/ia64_asm/interpolate8x8_ia64.s \
	motion/ia64_asm/sad_ia64.s \
	motion/ia64_asm/halfpel8_refine_ia64.s \
	quant/ia64_asm/quant_h263_ia64.s \
	utils/ia64_asm/mem_transfer_ia64.s

SRC_IA64_IDCT_GCC = \
	dct/ia64_asm/idct_ia64_gcc.s

SRC_IA64_IDCT_ECC = \
	dct/ia64_asm/idct_ia64_ecc.s

SRC_PPC_ALTIVEC = \
	utils/ppc_asm/altivec_trigger.c \
	utils/ppc_asm/mem_transfer_altivec.c \
	motion/ppc_asm/sad_altivec.c \
	dct/ppc_asm/idct_altivec.c \
	image/ppc_asm/interpolate8x8_altivec.c \
	image/ppc_asm/colorspace_altivec.c \
	image/ppc_asm/qpel_altivec.c \
	quant/ppc_asm/quant_h263_altivec.c \
	quant/ppc_asm/quant_mpeg_altivec.c

/* DreamShell ##version##

   cis_isp_regs.h - dreameye CIS and ISP regs
   Copyright (C)2023 SWAT

*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \brief Hyndai HV7131B CMOS IMAGE SENSOR registers */

/** \brief Row Start Address (Upper byte) */
#define CIS_MODE_A 0x00
/** \brief Row Start Address ( Lower byte ) */
#define CIS_MODE_B 0x01
/** \brief Column start Address ( Upper byte ) */
#define CIS_FRSU 0x10
/** \brief Column start Address ( Lower byte ) */
#define CIS_FRSL 0x11
/** \brief Column start Address ( Upper byte ) */
#define CIS_FCSU 0x12
/** \brief Column start Address ( Lower byte ) */
#define CIS_FCSL 0x13
/** \brief Window Height ( Upper byte ) */
#define CIS_FWHU 0x14
/** \brief Window Height ( Lower byte ) */
#define CIS_FWHL 0x15
/** \brief Window Width ( Upper byte ) */
#define CIS_FWWU 0x16
/** \brief Window Width ( Lower byte ) */
#define CIS_FWWL 0x17
/** \brief HSYNC Blanking Duration value ( Upper byte ) */
#define CIS_THBU 0x20
/** \brief HSYNC Blanking Duration value ( Lower byte ) */
#define CIS_THBL 0x21
/** \brief VSYNC Blanking Duration value ( Upper byte ) */
#define CIS_TVBU 0x22
/** \brief VSYNC Blanking Duration value ( Lower byte ) */
#define CIS_TVBL 0x23
/** \brief Integration Time value ( Upper byte ) */
#define CIS_TITU 0x25
/** \brief Integration Time value ( Middle byte ) */
#define CIS_TITM 0x26
/** \brief Integration Time value ( Lower byte ) */
#define CIS_TITL 0x27
/** \brief Master Clock Divider */
#define CIS_TMCD 0x28
/** \brief Reset Level Value */
#define CIS_ARLV 0x30
/** \brief Red Color Gain */
#define CIS_ARCG 0x31
/** \brief Green Color Gain */
#define CIS_AGCG 0x32
/** \brief Blue Color Gain */
#define CIS_ABCG 0x33
/** \brief Pixel Bias Voltage Control */
#define CIS_APBV 0x34
/** \brief R Offset Register */
#define CIS_OFSR 0x50
/** \brief G offset Register */
#define CIS_OFSG 0x51
/** \brief B offset Register */
#define CIS_OFSB 0x52
/** \brief Low Reset Level Counter [<3] (Upper byte) (Read only) */
#define CIS_LoREfNOH 0x57
/** \brief Low Reset Level Counter [<3] (Lower byte) (Read only) */
#define CIS_LoREfNOL 0x58
/** \brief High Reset Level Counter [>123] (Upper byte) (Read only) */
#define CIS_HiRefNOH 0x59
/** \brief High Reset Level Counter [>123] (Lower byte) (Read only) */
#define CIS_HiRefNOL 0x59


/** \brief Hyndai H1A424M167 Image Signal Processor registers */

/** \brief Operating Mode Register */
#define ISP_OP_MODE 0x80
/** \brief Base Function Enable Register */
#define ISP_BASE_ENB 0x81
/** \brief Scale Width Control Upper Register */
#define ISP_SCALE_UPPER 0x82
/** \brief Scale Width Control Lower Register */
#define ISP_SCALE_LOWER 0x83
/** \brief CMA11 Register */
#define ISP_CMA11 0x8A
/** \brief CMA12 Register */
#define ISP_CMA12 0x8B
/** \brief CMA13 Register */
#define ISP_CMA13 0x8C
/** \brief CMA21 Register */
#define ISP_CMA21 0x8D
/** \brief CMA22 Register */
#define ISP_CMA22 0x8E
/** \brief CMA23 Register */
#define ISP_CMA23 0x8F
/** \brief CMA31 Register */
#define ISP_CMA31 0x90
/** \brief CMA32 Register */
#define ISP_CMA32 0x91
/** \brief CMA33 Register */
#define ISP_CMA33 0x92
/** \brief OFSR Register */
#define ISP_OFSR 0x93
/** \brief OFSG Register */
#define ISP_OFSG 0x94
/** \brief OFSB Register */
#define ISP_OFSB 0x95
/** \brief Auto Function Enable Register */
#define ISP_AUTO_ENB 0xA0
/** \brief AWB/AE Window Horizontal Start Position Ha */
#define ISP_WIN_H_START 0xA1
/** \brief Horizontal Side Segment Width Hb */
#define ISP_WIN_H_SIDE 0xA2
/** \brief Horizontal Center Segment Width Hc */
#define ISP_WIN_H_CENTER	0xA3
/** \brief AWB/AE Window Vertical Start Position Va */
#define ISP_WIN_V_START 0xA4
/** \brief Vertical Side Segment Height Vb */
#define ISP_WIN_V_SIDE 0xA5
/** \brief Vertical Center Segment Height Vc */
#define ISP_WIN_V_CENTER	0xA6
/** \brief Analog Gain-Top Limit Register */
#define ISP_GAIN_TOP 0xA7
/** \brief Analog Gain-Bottom Limit Register */
#define ISP_GAIN_BOTTOM 0xA8
/** \brief AWB Function Control Register */
#define ISP_AWB_CONTROL 0xA9
/** \brief AWB Lock Control Register */
#define ISP_AWB_LOCK 0xAA
/** \brief AE Function Control Register */
#define ISP_AE_CONTROL 0xAB
/** \brief AE Lock Control Register */
#define ISP_AE_LOCK 0xAC
/** \brief Y-target Value Register */
#define ISP_Y_TARGET 0xAD
/** \brief Reset Level Control Register */
#define ISP_RESET_LEVEL 0xAE
/** \brief Exposure Time Limitation Value Upper Byte */
#define ISP_EXP_LMT_UPPER	0xB0
/** \brief Exposure Time Limitation Value Middle Byte */
#define ISP_EXP_LMT_MIDDLE	0xB1
/** \brief Exposure Time Limitation Value Lower Byte */
#define ISP_EXP_LMT_LOWER	0xB2
/** \brief AWB Cr-target Value Register */
#define ISP_AWB_CR_TARGET	0xB3
/** \brief AWB Cb-target Value Register */
#define ISP_AWB_CB_TARGET	0xB4
/** \brief Anti Flicker Unit Time Upper Byte */
#define ISP_AF_UT_UPPER 0xB5
/** \brief Anti Flicker Unit Time Middle Byte */
#define ISP_AF_UT_MIDDLE	0xB6
/** \brief Anti Flicker Unit Time Lower Byte */
#define ISP_AF_UT_LOWER 0xB7
/** \brief Lock Status Flags Register (Read Only) */
#define ISP_STATUS_FLAGS	0xB8
/** \brief Edge Control Register */
#define ISP_EDGE_CONTROL	0xC0
/** \brief Output Format Control Register */
#define ISP_OUT_FORM 0xC1
/** \brief HSYNC Counter Register */
#define ISP_HSYNC_COUNT 0xC2
/** \brief Manual Histogram Mode Control Register */
#define ISP_HISTO_MODE 0xC3
/** \brief Fixed Contrast Stretching Factor Register  */
#define ISP_FIXED_FACTOR	0xC4
/** \brief Gamma Start 0 Register */
#define ISP_GMA_START0 0xE0
/** \brief Gamma Start 1 Register */
#define ISP_GMA_START1 0xE1
/** \brief Gamma Start 2 Register */
#define ISP_GMA_START2 0xE2
/** \brief Gamma Start 3 Register */
#define ISP_GMA_START3 0xE3
/** \brief Gamma Start 4 Register */
#define ISP_GMA_START4 0xE4
/** \brief Gamma Start 5 Register */
#define ISP_GMA_START5 0xE5
/** \brief Gamma Start 6 Register */
#define ISP_GMA_START6 0xE6
/** \brief Gamma Start 7 Register */
#define ISP_GMA_START7 0xE7
/** \brief Gamma Start 8 Register */
#define ISP_GMA_START8 0xE8
/** \brief Gamma Slope 0 Register */
#define ISP_GMA_SLOPE0 0xE9
/** \brief Gamma Slope 1 Register */
#define ISP_GMA_SLOPE1 0xEA
/** \brief Gamma Slope 2 Register */
#define ISP_GMA_SLOPE2 0xEB
/** \brief Gamma Slope 3 Register */
#define ISP_GMA_SLOPE3 0xEC
/** \brief Gamma Slope 4 Register */
#define ISP_GMA_SLOPE4 0xED
/** \brief Gamma Slope 5 Register */
#define ISP_GMA_SLOPE5 0xEE
/** \brief Gamma Slope 6 Register */
#define ISP_GMA_SLOPE6 0xEF
/** \brief Gamma Slope 7 Register */
#define ISP_GMA_SLOPE7 0xF0
/** \brief Gamma Slope 8 Register */
#define ISP_GMA_SLOPE8 0xF1

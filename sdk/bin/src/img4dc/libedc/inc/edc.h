/* @(#)edc.h	1.2 06/05/14 Copyright 1998-2001 Heiko Eissfeldt */

/*
 * This file contains protected intellectual property.
 *
 * Compact disc reed-solomon routines.
 * Copyright (c) 1998-2001 by Heiko Eissfeldt, heiko@hexco.de
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifdef	EDC_DECODER_HACK

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define	DO2(a)	a\
		a

#define	DO4(a)	DO2(a)\
		DO2(a)

#define	DO8(a)	DO4(a)\
		DO4(a)

#if	defined EDC_LAYER1 || defined EDC_LAYER2
#define RS_L12_BITS 8
#endif

/*
 * known sector types
 */
#define EDC_MODE_0	0
#define EDC_MODE_1	1
#define EDC_MODE_2	2
#define EDC_MODE_2_FORM_1	3
#define EDC_MODE_2_FORM_2	4
#define EDC_AUDIO	5
#define EDC_UNKNOWN	6


typedef struct bcdmsf {
	unsigned char	bcdmsf_min;		/* the minute in bcd format */
	unsigned char	bcdmsf_sec;		/* the second in bcd format */
	unsigned char	bcdmsf_frame;	/* the frame in bcd format */
} bcdmsf_t;


#ifdef	EDC_LAYER2
/*
 * data sector definitions for RSPC
 * user data bytes per frame
 */
#define L2_RAW (1024*2)

#define L12_MODUL       255
#define L12_P_ERRORS    1
#define L12_Q_ERRORS    1
#define L12_P_LENGTH        43
#define L12_Q_LENGTH        26
#define L12_P_SKIPPED       (L12_MODUL-L12_Q_LENGTH)
#define L12_Q_SKIPPED       (L12_MODUL-L12_P_LENGTH-2*L12_Q_ERRORS)

/*
 * parity bytes for 16 bit units
 */
#define L2_Q   (L12_Q_LENGTH*2*L12_Q_ERRORS*2)
#define L2_P   (L12_P_LENGTH*2*L12_P_ERRORS*2)

/*
 * sector access macros
 *
 * col = column address of P layer
 * dia = diagonal address of Q layer
 * p   = position in column or diagonal
 *
 * SECxyz calculate the position in the sector from offset 12, where
 *    x   is B for byte access and W for word access
 *     y  is P for the P parity layer and Q for the Q parity layer
 *      z is L for the least significant and M for the most significant byte
 */
/* word macros */
#define SECWP(col,p)	(L12_P_LENGTH*(p)+(col))
/* byte macros */
#define SECBPL(col,p)	(SECWP(col,p) << 1)
#define SECBPM(col,p)	(SECBPL(col,p)+1)

#if	defined USE_ARRAY
/* word macros */
#define SECWQ(dia,p)	(qacc[dia][p] >> 1)
#define SECWQLO(dia,p)	(qacc[dia][p] >> 1)
#define SECWQHI(dia,p)	(qacc[dia][p] >> 1)
/* byte macros */
#define SECBQLLO(dia,p)	(qacc[dia][p])
#define SECBQLHI(dia,p)	(qacc[dia][p])
#define SECBQL(dia,p)	(qacc[dia][p])
#define SECBQMLO(dia,p)	(SECBQLLO(dia,p)+1)
#define SECBQMHI(dia,p)	(SECBQLHI(dia,p)+1)
#define SECBQM(dia,p)	(SECBQL(dia,p)+1)
#else
#define SECWQ(dia,p)	(SECWQC(dia,p))
#define SECWQLO(dia,p)	(SECWQLOC(dia,p))
#define SECWQHI(dia,p)	(SECWQHIC(dia,p))
#define SECBQLLO(dia,p)	(SECBQLLOC(dia,p))
#define SECBQLHI(dia,p)	(SECBQLHIC(dia,p))
#define SECBQL(dia,p)	(SECBQLC(dia,p))
#define SECBQMLO(dia,p)	(SECBQMLOC(dia,p))
#define SECBQMHI(dia,p)	(SECBQMHIC(dia,p))
#define SECBQM(dia,p)	(SECBQMC(dia,p))
#endif
/* word macros */
#define SECWQLOC(dia,p)	((unsigned short)(L12_P_LENGTH*((p)+(dia))+(p)) % (unsigned short) (L12_P_LENGTH*L12_Q_LENGTH))
#define SECWQHIC(dia,p)	(L12_Q_LENGTH*(p)+(dia))
#define SECWQC(dia,p)	((p) < L12_P_LENGTH ? SECWQLOC(dia,p) : SECWQHIC(dia,p))
/* byte macros */
#define SECBQLLOC(dia,p)	(SECWQLOC(dia,p) << 1)
#define SECBQLHIC(dia,p)	(SECWQHIC(dia,p) << 1)
#define SECBQLC(dia,p)	((p) < L12_P_LENGTH ? SECBQLLOC(dia,p) : SECBQLHIC(dia,p))
#define SECBQMLOC(dia,p)	(SECBQLLOC(dia,p)+1)
#define SECBQMHIC(dia,p)	(SECBQLHIC(dia,p)+1)
#define SECBQMC(dia,p)	(SECBQLC(dia,p)+1)


/*
 * conversion macros
 *
 * Calculate layer coordinates from a (12-byte based) sector offset in words
 *
 * POSP calculates the position in the column in the P parity layer
 * COL calculates the column from the offset
 *
 * POSQ calculates the position in the diagonal in the Q parity layer
 * DIA calculates the diagonal
 */
/* p parity macros */
#define POSP(off)	(unsigned)((unsigned short)(off) / (unsigned short)L12_P_LENGTH)
#define COL(off)	(unsigned)((unsigned short)(off) % (unsigned short)L12_P_LENGTH)

/* q parity macros */
#define POSQLO(off)	COL(off)
#define POSQHI(off)	(unsigned)((unsigned short)(off) / (unsigned short)L12_Q_LENGTH)
#define POSQ(off)	((unsigned short)(off) < (unsigned short)(L12_P_LENGTH*L12_Q_LENGTH) ? POSQLO(off) : POSQHI(off))
#define DIALO(off)	(unsigned)((unsigned short)(POSP(off)-COL(off)+2*L12_Q_LENGTH)%(unsigned short)L12_Q_LENGTH)
#define DIAHI(off)	(unsigned)((unsigned short)(off) % (unsigned short)L12_Q_LENGTH)
#define DIA(off)	((unsigned short)(off) < (unsigned short)(L12_P_LENGTH*L12_Q_LENGTH) ? DIALO(off) : DIAHI(off))

#ifdef EDC_ENCODER
/*
 * data sector layer 2 Reed-Solomon Product Code encoder
 *
 * encode the given data portion depending on sector type (see
 * get/set_sector_type() functions). Use the given address for the header.
 * The returned data is __unscrambled__ and not in F2-frame format (for that
 * see function scramble_L2()).
 *
 * Input parameter:
 * 	sectortype
 *   EDC_MODE_0: a 12-byte sync field, a header and 2336 zeros are returned.
 *   EDC_MODE_1: the user data portion (2048 bytes) has to be given
 *           at offset 16 in the inout array.
 *           Sync-, header-, edc-, spare-, p- and q- fields will be added.
 *   EDC_MODE_2: the user data portion (2336 bytes) has to be given
 *           at offset 16 in the inout array.
 *           Sync- and header- fields will be added.
 *   EDC_MODE_2_FORM_1: the user data portion (8 bytes subheader followed
 *                  by 2048 bytes data) has to be given at offset 16
 *                  in the inout array.
 *                  Sync-, header-, edc-, p- and q- fields will be added.
 *   EDC_MODE_2_FORM_2: the user data portion (8 bytes subheader followed
 *                  by 2324 bytes data) has to be given at offset 16
 *                  in the inout array.
 *                  Sync-, header- and edc- fields will be added.
 *
 * 	inout is an array of 2352 or more bytes.
 *	address is a pointer to a bcdmsf_t struct 
 *		holding bcd values for minute/second/frame address parts.
 *
 */
int            do_encode_L2 __PR((unsigned char *inout, int sectortype, bcdmsf_t *address));
#endif

#ifdef EDC_DECODER

#define	NO_ERRORS	1
#define FULLY_CORRECTED	0
#define	UNCORRECTABLE	-1
#define	WRONG_TYPE	-2
/*
 * data sector decoder for MODE 1 and MODE 2 FORM 1 sector types.
 * Input parameters:
 *	     inout:
 *           have_erasures: indicates error positions when > 0
 *           erasures: a value > 0 indicates an error at the corresponding
 *			 position in the inout array.
 *
 * On output: inout has error corrected data, if correctable.
 *	      return values:
 *			-1 uncorrectable
 *			 0 corrected
 *			 1 uncorrected (no correction needed)
 */
int		do_decode_L2 __PR((unsigned char inout[(L2_RAW + L2_Q + L2_P)], int sectortype, int have_erasures, unsigned char erasures[(L2_RAW + L2_Q + L2_P)]));

/*
 * calculate the crc checksum depending on the sector type.
 *
 * parameters:
 *   input:
 *		inout is the data sector as a byte array at offset 0
 *		type is the sector type (MODE_1, MODE_2_FORM_1 or MODE_2_FORM_2)
 *
 * return value:	1, if cyclic redundancy checksum is 0 (no errors)
 *			0, if errors are present.
 *
 */
int crc_check __PR((unsigned char inout[(L2_RAW + L2_Q + L2_P)], int type));



int     encode_L2_P __PR((unsigned char *inout));
int     encode_L2_Q __PR((unsigned char *inout));
#endif

extern const unsigned short qacc[26][64];

/*
 * calculates checksum for data sectors
 */
unsigned int    build_edc __PR((unsigned char inout[], unsigned from, unsigned upto));

/*
 * generates f2 frames from otherwise fully formatted sectors (generated by
 * do_encode_L2()).
 */
int             scramble_L2 __PR((unsigned char *inout));
#endif

#ifdef	EDC_SUBCHANNEL
/*
 * r-w sub channel definitions
 */
#define RS_SUB_RW_BITS 6

#define PACKETS_PER_SUBCHANNELFRAME 4
#define LSUB_RAW 18
#define LSUB_QRAW 2

/*
 * 6 bit entities
 */
#define LSUB_Q 2
#define LSUB_P 4

typedef struct del *edc_sub_delp;

/*
 * Create an initialized instance of the subchannel delay line.
 *
 * Return a pointer to the initialized delay line object.
 */
edc_sub_delp create_edc_sub_del __PR((void));

#ifdef EDC_ENCODER

/*
 * R-W subchannel encoder
 * input: inout has 96 bytes data, four frames with 24 bytes each.
 * output: four frames with 2 bytes user data, 2 bytes added Q parity,
 *                         16 bytes user data, 4 bytes added P parity.
 *
 * This routine fills the q and p parity fields.
 * No interleaving is done here.
 */
int             do_encode_sub __PR((
	unsigned char inout[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME]));


/*
 * R-W subchannel interleaver
 *
 * Apply after parity code generation.
 *
 * input: inout has 96 bytes user data, four frames with 24 bytes each.
 * output: as above but swapped and interleaved.
 *
 * Parameter:
 *   swap : if true, perform low level permutations (swaps)
 *   delay: if true, use low level delay line
 *   delp: pointer to a edc_sub_del struct
 * 		   (holding the state of the delay line)
 *		   this pointer is dereferenced only, if parameter delay is true.
 */
int
packed_to_raw __PR((unsigned char inout[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME],
	int swap, int delay, edc_sub_delp delp));

#endif

#ifdef EDC_DECODER
/*
 * R-W subchannel deinterleaver
 *
 * Apply before correction.
 *
 * input: inout has 96 bytes user data, four frames with 24 bytes each.
 * output: as above but swapped and interleaved (delayed).
 *
 * Parameter:
 *   swap : if true, perform permutations (swaps)
 *   delay: if true, use delay line
 *   delp: pointer to a edc_sub_delp struct
 * 		   (holding the state of the delay line)
 *		   this pointer is dereferenced only, if parameter delay is true.
 */
int
raw_to_packed __PR((unsigned char inout[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME],
	int swap, int delay, edc_sub_delp delp));

/*
 * R-W subchannel decoder
 * On input: inout: 96 bytes packed user data, four frames with each 24 bytes.
 *           have_erasures: indicates error positions when > 0
 *           erasures: a value > 0 indicates an error at the corresponding
 *			 position in the inout array.
 *
 * On output: inout has error corrected data, if correctable.
 *	      return values:
 *			-1 uncorrectable
 *			 0 corrected
 *			 1 uncorrected (not needed)
 */
int		do_decode_sub __PR((
unsigned char inout[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME],
                int have_erasures, 
unsigned char erasures[(LSUB_RAW + LSUB_Q+LSUB_P) *PACKETS_PER_SUBCHANNELFRAME],
int results[PACKETS_PER_SUBCHANNELFRAME]
				  ));

int check_sub __PR((unsigned char input[]));

#endif
#endif

#endif	/* EDC_DECODER_HACK */
/*
 * XXX Remove this if EDC_DECODER_HACK is removed
 */
int do_decode_L2	__PR((unsigned char inout[(L2_RAW + L2_Q + L2_P)], int sectortype, int have_erasures, unsigned char erasures[(L2_RAW + L2_Q + L2_P)]));
int crc_check		__PR((unsigned char inout[(L2_RAW + L2_Q + L2_P)], int type));

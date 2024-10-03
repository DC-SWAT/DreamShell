# KallistiOS ##version##
# Created by SWAT
# http://www.dc-swat.ru
# libcfg Makefile
#   
# $Id: Makefile,v 1.4 2003/02/27 04:25:40 bardtx Exp $


KOS_CFLAGS += -I./platon/str -I. \
	-DHAVE_LIMITS_H -DHAVE_FLOAT_H \
	-DHAVE_STRING_H -DHAVE_CTYPE_H \
	-DHAVE_MATH_H -DHAVE_STDLIB_H

TARGET = ../../libcfg.a
OBJS = cfg+.o parse.o props.o cmdline.o cfgfile.o shared.o \
	./platon/str/strplus.o \
	./platon/str/strdyn.o \
	./platon/str/strctype.o \
	./platon/str/dynfgets.o

include ../../../sdk/Makefile.library

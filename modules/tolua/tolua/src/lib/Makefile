# makefile for tolua library

TOLUA=../..

include $(TOLUA)/config

OBJS=	\
	tolua_event.o \
	tolua_is.o \
	tolua_map.o \
	tolua_push.o \
	tolua_to.o

T= ../../../../libtolua_5.1.4.a

all: $T

$T: $(OBJS)
	$(AR) $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -f $(OBJS) 
	
klean:
	rm -f $T


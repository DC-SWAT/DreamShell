# makefile for tolua executable

TOLUA=../..

include $(TOLUA)/config_pc

OBJS=	\
	tolua.o \
	toluabind.o

OBJS_D=  \
	tolua_lua.o

T= $(TOLUA)/bin/tolua
T_D = $(TOLUA)/bin/tolua_lua

all: $(T)

$(T): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIB) -ltolua -llua -lm

toluabind.c: $(T_D)
	$(T_D) -o toluabind.c tolua.pkg

$(T_D): $(OBJS_D)
	$(CC) -o $@ $(OBJS_D) $(LIB) -ltolua -llua -lm

tolua_lua.o: tolua.c
	$(CC) -c -DLUA_SOURCE $(INC) -o $@ $<

clean:
	rm -f tolua_lua.o $(OBJS) -$(OBJS_D) $(T) $(T_D)

klean:
	rm -f tolua_lua.o $(OBJS) -$(OBJS_D) $(T_D)


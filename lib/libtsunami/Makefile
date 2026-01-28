# Tsunami for KallistiOS ##version##
#
# Makefile
# Copyright (C) 2002 Megan Potter
# Copyright (C) 2024 Maniac Vera
# Copyright (C) 2026 SWAT

TARGET     		=  ../libtsunami_2.1.0.a

KOS_CPPFLAGS 	+= -fno-operator-names -fno-rtti -fno-exceptions -Iinclude -std=gnu++17

KOS_CFLAGS 		+= -I../../include/tsunami


# Main sources
OBJS_MAIN  := $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

# Drawables
OBJS_DRW   := $(patsubst %.cpp,%.o,$(wildcard src/drawables/*.cpp))

# Animations
OBJS_ANIMS := $(patsubst %.cpp,%.o,$(wildcard src/anims/*.cpp))

# Triggers
OBJS_TRIGS := $(patsubst %.cpp,%.o,$(wildcard src/triggers/*.cpp))

OBJS       := $(OBJS_MAIN) $(OBJS_DRW) $(OBJS_ANIMS) $(OBJS_TRIGS)

include ../../sdk/Makefile.library

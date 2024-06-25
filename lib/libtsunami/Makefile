# Tsunami for KallistiOS ##version##
#
# Makefile
# Copyright (C) 2002 Megan Potter
# Copyright (C) 2024 Maniac Vera

TARGET     		=  ../libtsunami_2.0.0.a

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

# # Grab the shared Makefile pieces
# #include $(KOS_BASE)/ds/lib/Makefile.prefab
include ../../sdk/Makefile.library
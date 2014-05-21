
SPUTARGET = $(TARGET_SPU)

ELFTARGET = aicaos.elf

CC := $(DC_ARM_CC)
CFLAGS := $(DC_ARM_CFLAGS) $(DC_ARM_INCS)

OBJS = aica_syscalls.o aica_arm.o main.o aica_common.o interrupt.o task.o task_asm.o
CRT0 = crt0.o

all: clean $(SPUTARGET)

$(SPUTARGET): $(ELFTARGET)
	$(DC_ARM_OBJCOPY) -O binary $(ELFTARGET) $(SPUTARGET)

$(ELFTARGET): $(OBJS) $(CRT0)
	$(DC_ARM_CC) -Wl,-Tscript.ld -nostartfiles -nostdlib -o $(ELFTARGET) $(OBJS) -lc -lgcc

aica_common.o: ../aica_common.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	-rm -f $(OBJS) $(CRT0) $(ELFTARGET) $(SPUTARGET) ../$(SPUTARGET)

install: $(SPUTARGET)
	cp $(SPUTARGET) ../$(SPUTARGET)


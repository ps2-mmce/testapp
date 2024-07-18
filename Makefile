
EE_BIN = mmceman_testapp.elf
EE_SRC_DIR ?= src/
TTY ?= UDP
EE_LIBS = -ldebug -lpatches -lpad -lkernel -liopreboot -lmc

EE_OBJS = $(addsuffix $(EE_SRC_DIR), common.o pad.o mmce_cmd_tests.o mmce_fs_tests.o pattern_256.o main.o ioprp.o)
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# Add embedded IRX files
EE_IRX_FILES =\
	mcserv.irx \
	mcman.irx \
	sio2man.irx\
	mmceman.irx\
	padman.irx \
	ioptrap.irx

ifeq ($(TTY),UDP)
  EE_LIBS += -lps2ip -lnetman
  EE_IRX_FILES += udptty.irx netman.irx smap.irx ps2dev9.irx ps2ip_nm.irx
  EE_CFLAGS += -DTTY_UDP
else ifeq ($(TTY),PPC)
  EE_IRX_FILES += ppctty.irx
  EE_CFLAGS += -DTTY_PPC
endif

EE_IRX_OBJS = $(addsuffix _irx.o, $(basename $(EE_IRX_FILES)))
EE_OBJS += $(EE_IRX_OBJS)

# Where to find the IRX files
vpath %.irx $(PS2SDK)/iop/irx/
vpath %.irx $(ROOT_DIR)/../mmceman/irx/

# Rule to generate them
%_irx.o: %.irx
	bin2c $< $*_irx.c $*_irx
	mips64r5900el-ps2-elf-gcc -c $*_irx.c -o $*_irx.o

$(EE_SRC_DIR)ioprp.c: iop/IOPRP.IMG
	bin2c $< $@ ioprp

all: $(EE_BIN)

clean:
	rm -f -r $(EE_OBJS) $(EE_BIN) *_irx.c

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

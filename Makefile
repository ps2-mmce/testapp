
EE_BIN = mmceman_testapp.elf

EE_LIBS = -lps2ip  -lnetman -ldebug -lpatches -lpad -lkernel

EE_OBJS = src/common.o src/pad.o src/mmce_cmd_tests.o src/mmce_fs_tests.o src/pattern_256.o src/main.o
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# Add embedded IRX files
EE_IRX_FILES =\
	mcman.irx \
	sio2man.irx\
	mmceman.irx\
	padman.irx \
	udptty.irx \
	netman.irx \
	smap.irx \
	ps2dev9.irx \
	ps2ip_nm.irx

EE_IRX_OBJS = $(addsuffix _irx.o, $(basename $(EE_IRX_FILES)))
EE_OBJS += $(EE_IRX_OBJS)

# Where to find the IRX files
vpath %.irx $(PS2SDK)/iop/irx/
vpath %.irx $(ROOT_DIR)/../mmceman/irx/

# Rule to generate them
%_irx.o: %.irx
	bin2c $< $*_irx.c $*_irx
	mips64r5900el-ps2-elf-gcc -c $*_irx.c -o $*_irx.o

all: $(EE_BIN)

clean:
	rm -f -r $(EE_OBJS) $(EE_BIN) *_irx.c

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
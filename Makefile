# Please see the README for setting up a valid build environment.

# The top-level binary that you wish to produce.
all: xargon.bin

# All of the source files (.c and .s) that you want to compile.
# You can use relative directories here as well. Note that if
# one of these files is not found, make will complain about a
# missing missing `build/naomi.bin' target, so make sure all of
# these files exist.
SRCS = $(wildcard source/*.c)

# Find the include files that we care about.
FLAGS += -I./

# Compile "normal linux" as per the forked repo.
FLAGS += -DNAOMI

# We need GNU extensions.
CSTD = gnu99

# We are using the add-on sprite library for scaled screen draw.
LIBS += -lnaomisprite -lnaomisramfs -lz -llfs

# We want a different serial to make this unique.
SERIAL = BXR0

# Pick up base makefile rules common to all examples.
include ${NAOMI_BASE}/tools/Makefile.base

# Provide a rule for converting our provided graphics.
build/%.o: %.png ${IMG2C_FILE}
	@mkdir -p $(dir $@)
	${IMG2C} build/$<.c --mode RGBA1555 $<
	${CC} -c build/$<.c -o $@

# Provide a rule to build our ROM FS.
build/romfs.bin: romfs/ ${ROMFSGEN_FILE}
	mkdir -p romfs/
	${ROMFSGEN} $@ $<

# Provide the top-level ROM creation target for this binary.
# See scripts/makerom.py for details about what is customizable.
xargon.bin: ${MAKEROM_FILE} ${NAOMI_BIN_FILE} build/romfs.bin
	${MAKEROM} $@ \
		--title "Xargon on Naomi" \
		--publisher "DragonMinded" \
		--serial "${SERIAL}" \
		--section ${NAOMI_BIN_FILE},${START_ADDR} \
		--entrypoint ${MAIN_ADDR} \
		--main-binary-includes-test-binary \
		--test-entrypoint ${TEST_ADDR} \
		--align-before-data 4 \
		--filedata build/romfs.bin

# Include a simple clean target which wipes the build directory
# and kills any binary built.
.PHONY: clean
clean:
	rm -rf build
	rm -rf xargon.bin

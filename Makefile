# Linux-hosted Makefile for Arculator

######################################################################

SERVE_IP   ?= localhost
SERVE_PORT ?= 3020

# Enable if you like, will fix this so dependencies are more automatic
#BUILD_PODULES := ultimatecdrom common_sound common_cdrom common_eeprom common_scsi

# Enable if you want to build a "full fat" version that includes ROMs, config and CMOS
# FULL_FAT := 1

######################################################################

SHELL     := bash
BUILD_TAG := $(shell echo `git rev-parse --short HEAD`-`[[ -n $$(git status -s) ]] && echo 'dirty' || echo 'clean'` on `date --rfc-3339=seconds`)

CC             ?= gcc
W64CC          := x86_64-w64-mingw32-gcc
CFLAGS         := -D_REENTRANT -DARCWEB -Wall -Werror -DBUILD_TAG="${BUILD_TAG}" -Isrc -Ibuild/generated-src
CFLAGS_WASM    := -sUSE_ZLIB=1 -sUSE_SDL=2 -Ibuild/generated-src
LINKFLAGS      := -lz -lSDL2 -lm -lGL -lGLU
LINKFLAGS_W64  := -Wl,-Bstatic -lz -Wl,-Bdynamic -lSDL2 -lm -lopengl32 -lglu32
LINKFLAGS_WASM := -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sTOTAL_MEMORY=32768000 -sFORCE_FILESYSTEM -sUSE_WEBGL2=1 -sEXPORTED_RUNTIME_METHODS=[\"ccall\"] -lidbfs.js
DATA           := ddnoise
ifdef DEBUG
  CFLAGS += -D_DEBUG -DDEBUG_LOG -O0 -g3
  LINKFLAGS_WASM += -gsource-map
  LINKFLAGS_W64 += -mconsole
  BUILD_TAG +=  (DEBUG)
  $(info ❗BUILD_TAG="${BUILD_TAG}")
  FULL_FAT = 1
else
  CFLAGS += -O3 -flto
  LINKFLAGS += -flto
  $(info ❗BUILD_TAG="${BUILD_TAG}")
  $(info ❗Re-run make with DEBUG=1 if you want a debug build)
endif

ifdef FULL_FAT
  DATA += roms/riscos311/ros311 roms/arcrom_ext cmos arc.cfg
endif

######################################################################

OBJS := 82c711 82c711_fdc \
	arm bmu cmos colourcard config cp15 \
	debugger debugger_swis ddnoise \
	disc disc_adf disc_apd disc_fdi disc_mfm_common \
        disc_hfe disc_jfd disc_scp ds2401 eterna fdi2raw \
        fpa g16 g332 \
	hostfs ide ide_a3in ide_config ide_idea \
	ide_riscdev ide_zidefs ide_zidefs_a3k \
	input_sdl2 ioc ioeb joystick keyboard \
	lc main mem memc podules printer \
	riscdev_hdfc romload sound sound_sdl2 \
	st506 st506_akd52 timer vidc video_sdl2gl wd1770 \
	wx-sdl2-joystick \
    emscripten_main emscripten-console emscripten_podule_config podules-static

PODULE_COMMON_INCLUDES = $(addprefix -I, $(sort $(dir $(wildcard podules/common/*/*.h))))
PODULE_DEFINES = $(addprefix -DPODULE_, $(filter-out common_%, $(BUILD_PODULES)))

######################################################################

OBJS_DOT_O := $(addsuffix .o,${OBJS})

OBJS_WASM   := $(addprefix build/wasm/,${OBJS_DOT_O}) build/wasm/hostfs-unix.o build/wasm/hostfs_emscripten.o
OBJS_NATIVE := $(addprefix build/native/,${OBJS_DOT_O}) build/native/hostfs-unix.o build/native/gl.o
OBJS_WIN64  := $(addprefix build/win64/,${OBJS_DOT_O}) build/win64/hostfs-win.o build/win64/gl.o

OBJS_WASM   += $(addsuffix .a, $(addprefix build/wasm/podules/,${BUILD_PODULES}))
OBJS_NATIVE += $(addsuffix .a, $(addprefix build/native/podules/,${BUILD_PODULES}))
OBJS_WIN64  += $(addsuffix .a, $(addprefix build/win64/podules/,${BUILD_PODULES}))

######################################################################
all:	native wasm win64

clean:
	rm -rf build

serve: wasm web/serve.js
	node web/serve.js ${SERVE_IP} ${SERVE_PORT}

######################################################################

build/native/video_sdl2gl.o: build/generated-src/video.vert.c build/generated-src/video.frag.c
build/wasm/video_sdl2gl.o: build/generated-src/video.vert.c build/generated-src/video.frag.c
build/win64/video_sdl2gl.o: build/generated-src/video.vert.c build/generated-src/video.frag.c

build/generated-src/%.c: src/%.glsl
	@mkdir -p $(@D)
	xxd -i $< $@

######################################################################

native:	build/native/arculator

build/native/arculator: ${OBJS_NATIVE}
	${CC} ${OBJS_NATIVE} -o $@ ${LINKFLAGS}

build/native/%.o: src/%.c
	@mkdir -p $(@D)
	${CC} -c ${CFLAGS} ${PODULE_DEFINES} $< -o $@

#### Rules for podules ###############################################

build/native/podules/common_cdrom.a: build/native/podules/common/cdrom/cdrom-linux-ioctl.o
	ar -rs $@ $^

build/native/podules/common_sound.a: $(addsuffix .o, $(addprefix build/native/podules/common/sound/, sound_out_sdl2))
	ar -rs $@ $^
build/native/podules/common_eeprom.a: build/native/podules/common/eeprom/93c06.o
	ar -rs $@ $^
build/native/podules/common_scsi.a: $(addsuffix .o, $(addprefix build/native/podules/common/scsi/, hdd_file scsi scsi_cd scsi_config scsi_hd))
	ar -rs $@ $^

build/native/podules/common/%.o: podules/common/%.c
	@mkdir -p $(@D)
	${CC} -o $@ -c ${CFLAGS} ${PODULE_COMMON_INCLUDES} $^

build/native/podules/ultimatecdrom.a: $(addsuffix .o, $(addprefix build/native/podules/ultimatecdrom/, mitsumi ultimatecdrom))
	ar -rs $@ $^

build/native/podules/ultimatecdrom/%.o: podules/ultimatecdrom/src/%.c
	@mkdir -p $(@D)
	${CC} -o $@ -c ${CFLAGS} ${PODULE_COMMON_INCLUDES} \
	  -Dpodule_probe=$(*F)_podule_probe \
	  -Dpodule_path=$(*F)_podule_path \
	  $<

######################################################################

win64:	build/win64/arculator.exe build/win64/SDL2.dll

build/win64/arculator.exe: ${OBJS_WIN64}
	${W64CC} ${OBJS_WIN64} -o $@ ${LINKFLAGS_W64} `./SDL2/x86_64-w64-mingw32/bin/sdl2-config --libs`

build/win64/%.o: src/%.c SDL2
	@mkdir -p $(@D)
	${W64CC} `./SDL2/x86_64-w64-mingw32/bin/sdl2-config --cflags` -c ${CFLAGS} ${PODULE_DEFINES} $< -o $@

#### Get SDL externally ##############################################

SDL_VERSION=2.28.5
SDL2:
	@mkdir -p $(@D)
	curl -L https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-devel-${SDL_VERSION}-mingw.tar.gz | tar xz
	ln -fs . SDL2-${SDL_VERSION}/x86_64-w64-mingw32/include/SDL2/SDL2
	cp SDL2/x86_64-w64-mingw32/bin/SDL2.dll build/win64/SDL2.dll

build/win64/SDL2.dll:	SDL2
	@mkdir -p $(@D)
	cp SDL2/x86_64-w64-mingw32/bin/SDL2.dll build/win64/SDL2.dll

#### Rules for podules ###############################################

build/win64/podules/common_cdrom.a: build/win64/podules/common/cdrom/cdrom-linux-ioctl.o
	ar -rs $@ $^

build/win64/podules/common_sound.a: $(addsuffix .o, $(addprefix build/win64/podules/common/sound/, sound_out_sdl2))
	ar -rs $@ $^
build/win64/podules/common_eeprom.a: build/win64/podules/common/eeprom/93c06.o
	ar -rs $@ $^
build/win64/podules/common_scsi.a: $(addsuffix .o, $(addprefix build/win64/podules/common/scsi/, hdd_file scsi scsi_cd scsi_config scsi_hd))
	ar -rs $@ $^

build/win64/podules/common/%.o: podules/common/%.c
	@mkdir -p $(@D)
	${W64CC} -o $@ -c ${CFLAGS} ${PODULE_COMMON_INCLUDES} $^

build/win64/podules/ultimatecdrom.a: $(addsuffix .o, $(addprefix build/native/podules/ultimatecdrom/, mitsumi ultimatecdrom))
	ar -rs $@ $^

build/win64/podules/ultimatecdrom/%.o: podules/ultimatecdrom/src/%.c
	@mkdir -p $(@D)
	${W64CC} -o $@ -c ${CFLAGS} ${PODULE_COMMON_INCLUDES} \
	  -Dpodule_probe=$(*F)_podule_probe \
	  -Dpodule_path=$(*F)_podule_path \
	  $<

######################################################################
wasm:	$(addprefix build/wasm/arculator.,html js wasm data data.js)

build/wasm/arculator.wasm build/wasm/arculator.js: build/wasm/arculator.html
build/wasm/arculator.html: ${OBJS_WASM} web/shell.html
	emcc ${LINKFLAGS_WASM} ${OBJS_WASM} --shell-file web/shell.html -o $@

build/wasm/arculator.data.js: build/wasm/arculator.data
build/wasm/arculator.data: ${DATA}
	${EMSDK}/upstream/emscripten/tools/file_packager $@ --js-output=$@.js --preload $^

build/wasm/%.o: src/%.c
	@mkdir -p $(@D)
	emcc -c ${CFLAGS} ${PODULE_DEFINES} ${CFLAGS_WASM} $< -o $@

build/wasm/podules/common_cdrom.a: build/wasm/podules/common/cdrom/cdrom-emscripten-ioctl.o
	emar -rs $@ $^
build/wasm/podules/common_sound.a: build/wasm/podules/common/sound/sound_out_sdl2.o
	emar -rs $@ $^
build/wasm/podules/common_eeprom.a: build/wasm/podules/common/eeprom/93c06.o
	emar -rs $@ $^
build/wasm/podules/common_scsi.a: $(addsuffix .o, $(addprefix build/wasm/podules/common/scsi/, hdd_file scsi scsi_cd scsi_config scsi_hd))
	emar -rs $@ $^

build/wasm/podules/common/%.o: podules/common/%.c
	@mkdir -p $(@D)
	emcc -o $@ -c ${CFLAGS} ${PODULE_COMMON_INCLUDES} $^

build/wasm/podules/ultimatecdrom.a: $(addsuffix .o, $(addprefix build/wasm/podules/ultimatecdrom/, mitsumi ultimatecdrom))
	emar -rs $@ $^

build/wasm/podules/ultimatecdrom/%.o: podules/ultimatecdrom/src/%.c
	@mkdir -p $(@D)
	emcc -o $@ -c ${CFLAGS} ${PODULE_COMMON_INCLUDES} \
	  -Dpodule_probe=$(*F)_podule_probe \
	  -Dpodule_path=$(*F)_podule_path \
	  $<

# Not sure why we need this, but for now
arc.cfg:
	touch arc.cfg

######################################################################
roms/arcrom_ext: roms/riscos311/ros311
roms/riscos311/ros311:
	curl -Ls https://b-em.bbcmicro.com/arculator/Arculator_V2.2_Linux.tar.gz | tar xz roms

# for extraction of ROMs from Arculator release...
# find . -iname "*rom*" -size +4 -type f \! -name "*.c" \! -name "*.tar" -exec tar cf roms.tar {} +

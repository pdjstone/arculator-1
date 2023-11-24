######################################################################

SERVE_IP   ?= localhost
SERVE_PORT ?= 3010

# Only "LINUX" tested for now
INCLUDE_LINUX := 1
#INCLUDE_MACOS := 1
#INCLUDE_WIN32 := 1

# Enable if you like, will fix this so dependencies are more automatic
#BUILD_PODULES := ultimatecdrom common_sound common_cdrom common_eeprom common_scsi

######################################################################

SHELL     := bash
BUILD_TAG := $(shell echo `git rev-parse --short HEAD`-`[[ -n $$(git status -s) ]] && echo 'dirty' || echo 'clean'` on `date --rfc-3339=seconds`)

CC             ?= gcc
CFLAGS         := -D_REENTRANT -DARCWEB -Wall -Werror -DBUILD_TAG="${BUILD_TAG}" -Isrc
CFLAGS_WASM    := -sUSE_ZLIB=1 -sUSE_SDL=2
LINKFLAGS      := -lz -lSDL2 -lm
LINKFLAGS_WASM := -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM -sEXPORTED_RUNTIME_METHODS=[\"ccall\"] -lidbfs.js
DATA           := ddnoise 
ifdef DEBUG
  CFLAGS += -D_DEBUG -DDEBUG_LOG -O0 -g3
  LINKFLAGS_WASM += -gsource-map
  BUILD_TAG +=  (DEBUG)
  $(info ❗BUILD_TAG="${BUILD_TAG}")
  DATA += roms/riscos311/ros311 roms/arcrom_ext cmos arc.cfg
else
  CFLAGS += -O3 -flto
  LINKFLAGS += -flto
  $(info ❗BUILD_TAG="${BUILD_TAG}")
  $(info ❗Re-run make with DEBUG=1 if you want a debug build)
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
	st506 st506_akd52 timer vidc video_sdl2 wd1770 \
	wx-sdl2-joystick \
    emscripten_main emscripten-console emscripten_podule_config podules-static

ifdef INCLUDE_LINUX
  OBJS += hostfs-unix
else ifdef INCLUDE_MACOS
  OBJS += hostfs-unix
else ifdef INCLUDE_WIN32
  OBJS += hostfs-win32
endif

PODULE_COMMON_INCLUDES = $(addprefix -I, $(sort $(dir $(wildcard podules/common/*/*.h))))
PODULE_DEFINES = $(addprefix -DPODULE_, $(filter-out common_%, $(BUILD_PODULES)))

######################################################################

OBJS_DOT_O := $(addsuffix .o,${OBJS})

OBJS_WASM   := $(addprefix build/wasm/,${OBJS_DOT_O}) build/wasm/hostfs_emscripten.o
OBJS_NATIVE := $(addprefix build/native/,${OBJS_DOT_O})

OBJS_WASM   += $(addsuffix .a, $(addprefix build/wasm/podules/,${BUILD_PODULES}))
OBJS_NATIVE += $(addsuffix .a, $(addprefix build/native/podules/,${BUILD_PODULES}))

######################################################################
all:	native wasm

clean:
	rm -rf build

serve: build/wasm/arculator.html build/wasm/arculator.data.js build/native/nweb
	@echo ">>>>>>> http://${SERVE_IP}:${SERVE_PORT}/build/wasm/arculator.html <<<<<<<<"
	@./build/native/nweb ${SERVE_PORT} .

build/native/nweb: src/nweb.c
	@mkdir -p $(@D)
	${CC} -Wall -Werror -o $@ $^

######################################################################

native:	build/native/arculator

build/native/arculator: ${OBJS_NATIVE}
	${CC} ${OBJS_NATIVE} -o $@ ${LINKFLAGS}

build/native/%.o: src/%.c
	@mkdir -p $(@D)
	${CC} -c ${CFLAGS} ${PODULE_DEFINES} $< -o $@

#### Rules for podules ###############################################

ifdef INCLUDE_LINUX
  CDROM_BACKEND := linux
  SOUND_BACKEND := sound_out_sdl2 
  #sound_alsain
else ifdef INCLUDE_MACOS
  CDROM_BACKEND := macos
  SOUND_BACKEND := sound_out_sdl2 sound_null
else ifdef INCLUDE_WIN32
  CDROM_BACKEND := win32
  SOUND_BACKEND := sound_out_sdl2 sound_wavein
endif

build/native/podules/common_cdrom.a: build/native/podules/common/cdrom/cdrom-${CDROM_BACKEND}-ioctl.o
	ar -rs $@ $^

build/native/podules/common_sound.a: $(addsuffix .o, $(addprefix build/native/podules/common/sound/, ${SOUND_BACKEND}))
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
wasm:	$(addprefix build/wasm/arculator.,html js wasm data data.js)

build/wasm/arculator.wasm build/wasm/arculator.js: build/wasm/arculator.html
build/wasm/arculator.html: ${OBJS_WASM}
	emcc ${LINKFLAGS_WASM} ${OBJS_WASM} -o $@
	sed -e "s/<script async/<script async type=\"text\/javascript\" src=\"arculator.data.js\"><\/script>&/" build/wasm/arculator.html >build/wasm/arculator_fix.html && mv build/wasm/arculator_fix.html build/wasm/arculator.html

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

######################################################################
roms/arcrom_ext: roms/riscos311/ros311
roms/riscos311/ros311:
	curl -s http://b-em.bbcmicro.com/arculator/Arculator_V2.1_Linux.tar.gz | tar xz roms


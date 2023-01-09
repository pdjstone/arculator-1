# Replaces autotools for emscripten build.

SHELL := bash
BUILD_TAG := $(shell echo `git rev-parse --short HEAD`-`[[ -n $$(git status -s) ]] && echo 'dirty' || echo 'clean'` on `date --rfc-3339=seconds`)

CC             := gcc
FIX_WARNINGS   := $(addprefix -Wno-,deprecated-non-prototype unused-but-set-variable constant-conversion integer-overflow unused-variable uninitialized int-to-pointer-cast)
CFLAGS         := -D_REENTRANT -DARCWEB -Wall ${FIX_WARNINGS} -DBUILD_TAG="${BUILD_TAG}"
CFLAGS_WASM    := -sUSE_ZLIB=1 -sUSE_SDL=2
LINKFLAGS      := -lz -lSDL2 -lopenal -lm -ldl
LINKFLAGS_WASM := -sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM -sEXPORTED_RUNTIME_METHODS=[\"ccall\"]

ifdef DEBUG
  CFLAGS += -D_DEBUG -DDEBUG_LOG -g3
  LINKFLAGS_WASM += -gsource-map --source-map-base http://localhost:8000/build/
  BUILD_TAG +=  (DEBUG)
  $(info ❗BUILD_TAG="${BUILD_TAG}")
else
  CFLAGS += -O3
  $(info ❗BUILD_TAG="${BUILD_TAG}")
  $(info ❗Re-run make with DEBUG=1 if you want a debug build)
endif

DATA := roms/riscos311/ros311 roms/arcrom_ext cmos ddnoise arc.cfg

OBJS := 82c711 82c711_fdc \
	arm bmu cmos colourcard config cp15 \
	debugger debugger_swis ddnoise \
	disc disc_adf disc_apd disc_fdi disc_hfe \
	disc_jfd disc_mfm_common disc_scp \
	ds2401 eterna fdi2raw fpa g16 g332 \
	hostfs ide ide_a3in ide_config ide_idea \
	ide_riscdev ide_zidefs ide_zidefs_a3k \
	input_sdl2 ioc ioeb joystick keyboard \
	lc main mem memc podules printer \
	riscdev_hdfc romload sound soundopenal \
	st506 st506_akd52 timer vidc video_sdl2 wd1770 \
	wx-sdl2-joystick hostfs-unix podules-linux \
	emscripten_main emscripten-console
OBJS_DOT_O  := $(addsuffix .o,${OBJS})
OBJS_NATIVE := $(addprefix build/native/,${OBJS_DOT_O})
OBJS_WASM   := $(addprefix build/wasm/,${OBJS_DOT_O})

######################################################################
all:	native wasm

clean:
	rm -rf build

serve: build/wasm/arculator.html
	@echo "Now open >> http://${SERVE_IP}:${SERVE_PORT}/build/wasm/arculator.html << in your browser"
	@python3 -mhttp.server -b ${SERVE_IP} ${SERVE_PORT}

######################################################################
native:	build/native/arculator

build/native/arculator: ${OBJS_NATIVE}
	${CC} ${OBJS_NATIVE} -o $@ ${LINKFLAGS}

build/native/%.o: src/%.c
	@mkdir -p $(@D)
	${CC} -c ${CFLAGS} $< -o $@

######################################################################
wasm:	$(addprefix build/wasm/arculator.,html js wasm data data.js)

build/wasm/arculator.wasm build/wasm/arculator.js: build/wasm/arculator.html
build/wasm/arculator.html: ${OBJS_WASM}
	emcc ${LINKFLAGS} ${LINKFLAGS_WASM} ${OBJS_WASM} -o $@
	sed -e "s/<script async/<script async type=\"text\/javascript\" src=\"arculator.data.js\"><\/script>&/" build/wasm/arculator.html >build/wasm/arculator_fix.html && mv build/wasm/arculator_fix.html build/wasm/arculator.html

build/wasm/arculator.data.js: build/wasm/arculator.data
build/wasm/arculator.data: ${DATA}
	${EMSDK}/upstream/emscripten/tools/file_packager $@ --js-output=$@.js --preload $^

build/wasm/%.o: src/%.c
	@mkdir -p $(@D)
	emcc -c ${CFLAGS} ${CFLAGS_WASM} $< -o $@

######################################################################
roms/arcrom_ext: roms/riscos311/ros311
roms/riscos311/ros311:
	curl -s http://b-em.bbcmicro.com/arculator/Arculator_V2.1_Linux.tar.gz | tar xz roms


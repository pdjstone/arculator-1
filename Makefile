# Replaces autotools for emscripten build.

CC             := gcc
FIX_WARNINGS   := $(addprefix -Wno-,deprecated-non-prototype unused-but-set-variable constant-conversion integer-overflow unused-variable uninitialized int-to-pointer-cast)
CFLAGS         := -g3 -D_REENTRANT -DARCWEB -D_DEBUG -DDEBUG_LOG -Wall ${FIX_WARNINGS}
CFLAGS_WASM    := -sUSE_ZLIB=1 -sUSE_SDL=2
LINKFLAGS      := -lz -lSDL2 -lopenal -lm -ldl
LINKFLAGS_WASM :=-gsource-map --source-map-base http://localhost:8000/build/ -sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM

#-sEXPORTED_RUNTIME_METHODS=[\"ccall\"]

DATA := roms/riscos311/ros311 roms/arcrom_ext cmos ddnoise arc.cfg

OBJS := 82c711.o 82c711_fdc.o arm.o bmu.o cmos.o colourcard.o config.o cp15.o ddnoise.o disc.o disc_adf.o disc_apd.o disc_fdi.o disc_hfe.o disc_jfd.o disc_mfm_common.o ds2401.o eterna.o fdi2raw.o fpa.o g16.o g332.o hostfs.o ide.o ide_a3in.o ide_config.o ide_idea.o ide_riscdev.o ide_zidefs.o ide_zidefs_a3k.o input_sdl2.o ioc.o ioeb.o joystick.o keyboard.o lc.o main.o mem.o memc.o podules.o printer.o riscdev_hdfc.o romload.o sound.o soundopenal.o st506.o st506_akd52.o timer.o vidc.o video_sdl2.o wd1770.o wx-sdl2-joystick.o hostfs-unix.o podules-linux.o emscripten_main.o
OBJS_NATIVE := $(addprefix build/native/,${OBJS})
OBJS_WASM   := $(addprefix build/wasm/,${OBJS})

#all: build $(addprefix build/wasm/arculator.,wasm js html data data.js)

all:	native wasm
native:	build/native/arculator
wasm:	$(addprefix build/wasm/arculator.,html js wasm data data.js)

build/native/arculator: ${OBJS_NATIVE}
	${CC} ${OBJS_NATIVE} -o $@ ${LINKFLAGS}

build/native/%.o: src/%.c
	@mkdir -p $(@D)
	${CC} -c ${CFLAGS} $< -o $@

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

roms/arcrom_ext: roms/riscos311/ros311
roms/riscos311/ros311:
	curl -s http://b-em.bbcmicro.com/arculator/Arculator_V2.1_Linux.tar.gz | tar xz roms

clean:
	rm -rf build

serve: build/wasm/arculator.html
	@echo "Now open >> http://localhost:8000/build/wasm/arculator.html << in your browser"
	@python3 -mhttp.server -d.

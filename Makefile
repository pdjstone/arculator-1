# Replaces autotools for emscripten build.

FIX_WARNINGS  := $(addprefix -Wno-,deprecated-non-prototype unused-but-set-variable constant-conversion integer-overflow unused-variable uninitialized)
CFLAGS        := -g3 -D_REENTRANT -DARCWEB -sUSE_ZLIB=1 -sUSE_SDL=2 -D_DEBUG -DDEBUG_LOG -Wall ${FIX_WARNINGS}
LINKFLAGS     :=  -gsource-map --source-map-base http://localhost:8000/build/ -sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM -lz -lSDL2
#-sEXPORTED_RUNTIME_METHODS=[\"ccall\"]

DATA := roms/riscos311/ros311 cmos ddnoise arc.cfg

OBJS := $(addprefix build/,82c711.o 82c711_fdc.o arm.o bmu.o cmos.o colourcard.o config.o cp15.o ddnoise.o disc.o disc_adf.o disc_apd.o disc_fdi.o disc_hfe.o disc_jfd.o disc_mfm_common.o ds2401.o eterna.o fdi2raw.o fpa.o g16.o g332.o hostfs.o ide.o ide_a3in.o ide_config.o ide_idea.o ide_riscdev.o ide_zidefs.o ide_zidefs_a3k.o input_sdl2.o ioc.o ioeb.o joystick.o keyboard.o lc.o main.o mem.o memc.o podules.o printer.o riscdev_hdfc.o romload.o sound.o soundopenal.o st506.o st506_akd52.o timer.o vidc.o video_sdl2.o wd1770.o wx-sdl2-joystick.o hostfs-unix.o podules-linux.o emscripten_main.o)

all: build $(addprefix build/arculator.,wasm js html data data.js)

build:
	@mkdir build

build/arculator.wasm build/arculator.js: build/arculator.html
build/arculator.html: ${OBJS}
	emcc ${LINKFLAGS} ${OBJS} -o $@
	sed -e "s/<script async/<script async type=\"text\/javascript\" src=\"arculator.data.js\"><\/script>&/" build/arculator.html >build/arculator_fix.html && mv build/arculator_fix.html build/arculator.html

build/arculator.data.js: build/arculator.data
build/arculator.data: ${DATA}
	${EMSDK}/upstream/emscripten/tools/file_packager $@ --js-output=$@.js --preload $^

build/%.o: src/%.c build
	emcc -c ${CFLAGS} $< -o $@

roms/riscos311/ros311:
	curl -s http://b-em.bbcmicro.com/arculator/Arculator_V2.1_Linux.tar.gz | tar xz roms

clean:
	rm -rf build

serve: build/arculator.html
	@echo "Now open >> http://localhost:8000/build/arculator.html << in your browser"
	@python3 -mhttp.server -d.

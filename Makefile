# Linux or Mac-hosted Makefile for Arculator

######################################################################

SERVE_IP   ?= localhost
SERVE_PORT ?= 3020

# Enable if you like, will fix this so dependencies are more automatic
#BUILD_PODULES := ultimatecdrom common_sound common_cdrom common_eeprom common_scsi

# Enable if you want to build a "full fat" version that includes ROMs, config and CMOS
# FULL_FAT := 1

######################################################################

UNAME     := $(shell uname)

SHELL     := bash
BUILD_TAG := $(shell echo `git rev-parse --short HEAD`-`[[ -n $$(git status -s) ]] && echo 'dirty' || echo 'clean'` on `date -u +"%Y-%m-%dT%H:%M:%SZ"`)

CC             ?= gcc
W64CC          := x86_64-w64-mingw32-gcc
CFLAGS         := -D_REENTRANT -DARCWEB -Wall -Werror -DBUILD_TAG="${BUILD_TAG}" -Isrc -Ibuild/generated-src -include embed.h
ifeq ($(UNAME),Darwin)
  CFLAGS += -Fbuild/SDL2-mac -DGL_SILENCE_DEPRECATION
endif
CFLAGS_WASM    := -sUSE_ZLIB=1 -sUSE_SDL=2 -Ibuild/generated-src
LINKFLAGS      := -lz -lm
ifeq ($(UNAME),Darwin)
  LINKFLAGS += -Fbuild/SDL2-mac -F/System/Library/Frameworks -framework SDL2 -framework OpenGL -rpath build/SDL2-mac -rpath @executable_path/../Resources/
else
  LINKFLAGS += -lSDL2 -lGL -lGLU
endif
LINKFLAGS_W64  := -Wl,-Bstatic -lz -Wl,-Bdynamic -lSDL2 -lm -lopengl32 -lglu32
LINKFLAGS_WASM := -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sTOTAL_MEMORY=32768000 -sFORCE_FILESYSTEM -sUSE_WEBGL2=1 -sEXPORTED_RUNTIME_METHODS=[\"ccall\"] -lidbfs.js
DATA           := ddnoise src/video.vert.glsl src/video.frag.glsl

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
  DATA += roms/riscos311.rom roms/arcrom_ext cmos arc.cfg
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
    emscripten_main emscripten-console emscripten_podule_config podules-static \
	embed c-embed

PODULE_COMMON_INCLUDES = $(addprefix -I, $(sort $(dir $(wildcard podules/common/*/*.h))))
PODULE_DEFINES = $(addprefix -DPODULE_, $(filter-out common_%, $(BUILD_PODULES)))

######################################################################

OBJS_DOT_O := $(addsuffix .o,${OBJS})

OBJS_WASM   := $(addprefix build/wasm/,${OBJS_DOT_O}) build/wasm/hostfs-unix.o build/wasm/hostfs_emscripten.o
OBJS_NATIVE := $(addprefix build/native/,${OBJS_DOT_O}) build/native/hostfs-unix.o build/native/gl.o
OBJS_WIN64  := $(addprefix build/win64/,${OBJS_DOT_O}) build/win64/hostfs-win.o build/win64/gl.o build/win64/icon.o

OBJS_WASM   += $(addsuffix .a, $(addprefix build/wasm/podules/,${BUILD_PODULES}))
OBJS_NATIVE += $(addsuffix .a, $(addprefix build/native/podules/,${BUILD_PODULES}))
OBJS_WIN64  += $(addsuffix .a, $(addprefix build/win64/podules/,${BUILD_PODULES}))

######################################################################

ifeq ($(UNAME),Darwin)
all:	native wasm macbundle
else
all:	native wasm win64
endif

macbundle: build/Arculator.app

clean:
	rm -rf build

serve: wasm web/serve.js
	node web/serve.js ${SERVE_IP} ${SERVE_PORT}

######################################################################

# if you're playing around here, you might need to do this:
#
#     /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f build/Arculator.app 
#
# to get the Finder to show an updated icon

APPC=build/Arculator.app/Contents

build/Arculator.app: native ${APPC}/Resources/Arculator.icns ${APPC}/Resources/Arculator.icns ${APPC}/Info.plist
	@mkdir -p ${APPC}/{MacOS,Resources}
	cp build/native/arculator ${APPC}/MacOS/arculator
	cp -a build/SDL2-mac/SDL2.framework ${APPC}/Resources/SDL2.framework

# Separated because it only works on the console at the moment, not sure
# what a sensible default should be.
sign-macbundle:
	codesign -s `security find-identity -v -p codesigning | head -n1 | cut -f4 -d' '` $@

${APPC}/Resources/Arculator.icns: build/Arculator.iconset
	mkdir -p $(@D)
	iconutil -c icns --output $@ $<

build/Arculator.iconset: build/Archimedes.svg.png
	mkdir -p $@
	sips -z 16 16     $< --out $@/icon_16x16.png
	sips -z 32 32     $< --out $@/icon_16x16@2x.png
	sips -z 32 32     $< --out $@/icon_32x32.png
	sips -z 64 64     $< --out $@/icon_32x32@2x.png
	sips -z 128 128   $< --out $@/icon_128x128.png
	sips -z 256 256   $< --out $@/icon_128x128@2x.png
	sips -z 256 256   $< --out $@/icon_256x256.png
	sips -z 512 512   $< --out $@/icon_256x256@2x.png
	sips -z 512 512   $< --out $@/icon_512x512.png
	sips -z 1024 1024 $< --out $@/icon_512x512@2x.png

build/Archimedes.svg.png: Archimedes.svg
	qlmanage -t -s 2000 -o build $<
	sips --cropOffset 1 1 -c 616 588 $@

${APPC}/Info.plist:
	@mkdir -p $(@D)
	./mac/generate-Info.plist "${BUILD_TAG}" >$@

######################################################################

build/generated-src/c-embed.c: build/native/c-embed-build ${DATA} ${ROMS}
	@mkdir -p $(@D)
	./build/native/c-embed-build ${DATA} > $@

######################################################################

native:	build/native/arculator

build/native/arculator: ${OBJS_NATIVE}
	${CC} ${OBJS_NATIVE} -o $@ ${LINKFLAGS}

build/native/c-embed-build: src/c-embed-build.c
	@mkdir -p $(@D)
	${CC} -Wall -o $@ $<

build/native/c-embed.o: build/generated-src/c-embed.c
	@mkdir -p $(@D)
	${CC} -c ${CFLAGS} $< -o $@

ifeq ($(UNAME),Darwin)
  SDL2_MAC = build/SDL2-mac
endif

build/native/%.o: src/%.c $(SDL2_MAC)
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

build/win64/c-embed.o: build/generated-src/c-embed.c
	@mkdir -p $(@D)
	${W64CC} -c ${CFLAGS} $< -o $@

build/win64/%.o: src/%.c SDL2
	@mkdir -p $(@D)
	${W64CC} `./SDL2/x86_64-w64-mingw32/bin/sdl2-config --cflags` -c ${CFLAGS} ${PODULE_DEFINES} $< -o $@

build/win64/icon.o: build/win64/icon.rc
	@mkdir -p $(@D)
	x86_64-w64-mingw32-windres $< -O coff -o $@

build/win64/icon.rc: build/win64/icon.ico
	@mkdir -p $(@D)
	echo 'id ICON $<' > $@

build/win64/icon.ico: Archimedes.svg
	@mkdir -p $(@D)
	convert $< -crop 220x256 -define icon:auto-resize="256,128,96,64,48,32,16" $@

#### Get SDL externally ##############################################

SDL_VERSION=2.28.5
SDL2:
	@mkdir -p $(@D)
	curl -L https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-devel-${SDL_VERSION}-mingw.tar.gz | tar xz
	ln -s   SDL2-${SDL_VERSION} SDL2
	ln -s . SDL2-${SDL_VERSION}/x86_64-w64-mingw32/include/SDL2/SDL2

build/SDL2.dmg:
	@mkdir -p $(@D)
	curl -L https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.dmg >$@

build/SDL2-mac: build/SDL2.dmg
	@mkdir -p $@ $@.mnt
	hdiutil attach -mountpoint $@.mnt $<
	tar c -C $@.mnt . | tar x -C $@
	hdiutil detach $@.mnt

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

build/wasm/c-embed.o: build/generated-src/c-embed.c
	@mkdir -p $(@D)
	emcc -c ${CFLAGS} $< -o $@

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

build/src-generated/c-embed.c: ${DATA}
	@mkdir -p $(@D)
	./build/native/c-embed-build ${DATA} > $@.tmp && mv $@.tmp $@

# Upstream Arculator releases allow the user to put unpredictably-named ROMs 
# inside predictably-named directories. We can cut a lot of code complexity
# by just renaming those randomly-named ROMs to match those of the directories
# (there's only ever one ROM per folder). So here's how we do that.
#
# There is also arcrom_ext and "A4 5th Column.rom" which can stay as-is.
#
roms/arcrom_ext roms/riscos311.rom:
	curl -Ls https://b-em.bbcmicro.com/arculator/Arculator_V2.2_Linux.tar.gz | tar xz roms
	find roms roms/podules -mindepth 2 -maxdepth 2 -size +4 -type f -printf "%p\0%h.rom\0" | xargs -L2 -0 mv -n -v

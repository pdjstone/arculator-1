# Makefile.am for Arculator

AUTOMAKE_OPTIONS = subdir-objects

noinst_SCRIPTS =
CLEANFILES =

if !EMSCRIPTEN
bin_PROGRAMS = arculator
else
bin_PROGRAMS = arculatorweb
endif

if !OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
if !EMSCRIPTEN
noinst_SCRIPTS += ../arculator
../arculator: arculator
	cp arculator ..
else
noinst_SCRIPTS += ../arculatorweb
../arculatorweb: arculatorweb
	if [ -d ../emscripten_assets ]; then rm -rf ../emscripten_assets; fi
	mkdir ../emscripten_assets
	mkdir -p ../emscripten_out/
	cp -r ../ddnoise ../emscripten_assets
	cp -r ../cmos ../emscripten_assets
	cp ../arc.cfg ../emscripten_assets
	mkdir -p ../emscripten_assets/roms/
	cp -r ../roms/riscos311 ../emscripten_assets/roms/
	emcc -sUSE_ZLIB=1 -sUSE_SDL=2  -sALLOW_MEMORY_GROWTH=1 *.o -o ../emscripten_out/arculator.html --preload-file ../emscripten_assets@/
endif
endif

amrefresh:	

# Special wxWidgets files
WINDRES = $(shell $(WX_CONFIG_PATH) --rescomp)
wx.res: wx.rc
	$(WINDRES) -i wx.rc --input-format=rc -o wx.res -O coff
 
wx-resources.cpp : arculator.xrc
	-wxrc -c arculator.xrc -o wx-resources.cc

# Arculator
arculator_SOURCES = 82c711.c 82c711_fdc.c arm.c bmu.c cmos.c colourcard.c config.c cp15.c ddnoise.c \
 debugger.c debugger_swis.c disc.c disc_adf.c disc_apd.c disc_fdi.c disc_hfe.c disc_jfd.c disc_mfm_common.c disc_scp.c ds2401.c \
 eterna.c fdi2raw.c fpa.c g16.c g332.c hostfs.c ide.c ide_a3in.c ide_config.c ide_idea.c ide_riscdev.c \
 ide_zidefs.c ide_zidefs_a3k.c input_sdl2.c ioc.c ioeb.c joystick.c keyboard.c lc.c main.c mem.c memc.c \
 podules.c printer.c riscdev_hdfc.c romload.c sound.c soundopenal.c st506.c st506_akd52.c timer.c vidc.c \
 video_sdl2.c wd1770.c wx-app.cc wx-config.cc wx-config_sel.cc wx-hd_conf.cc wx-console.cc wx-hd_new.cc \
 wx-joystick-config.cc wx-main.cc wx-podule-config.cc wx-resources.cc wx-sdl2.c wx-sdl2-joystick.c

arculator_CFLAGS = $(subst -fpermissive,,$(shell $(WX_CONFIG_PATH) --cxxflags) $(shell sdl2-config --cflags))
arculator_CXXFLAGS = $(shell $(WX_CONFIG_PATH) --cxxflags) $(shell sdl2-config --cflags)
arculator_LDADD = @LIBS@

if OS_LINUX
arculator_SOURCES += hostfs-unix.c podules-linux.c
arculator_LDADD += -ldl -lX11 
endif

if OS_MACOSX
arculator_SOURCES += hostfs-unix.c podules-macosx.c
endif

arculatorweb_SOURCES = 82c711.c 82c711_fdc.c arm.c bmu.c cmos.c colourcard.c config.c cp15.c ddnoise.c \
 disc.c disc_adf.c disc_apd.c disc_fdi.c disc_hfe.c disc_jfd.c disc_mfm_common.c ds2401.c eterna.c \
 fdi2raw.c fpa.c g16.c g332.c hostfs.c ide.c ide_a3in.c ide_config.c ide_idea.c ide_riscdev.c ide_zidefs.c \
 ide_zidefs_a3k.c input_sdl2.c ioc.c ioeb.c joystick.c keyboard.c lc.c main.c mem.c memc.c podules.c printer.c riscdev_hdfc.c \
 romload.c sound.c soundopenal.c st506.c st506_akd52.c timer.c vidc.c video_sdl2.c wd1770.c wx-sdl2-joystick.c

arculatorweb_SOURCES += hostfs-unix.c podules-linux.c emscripten_main.c 
arculatorweb_CFLAGS = $(shell sdl2-config --cflags)
arculatorweb_LDADD = @LIBS@ 
arculatorweb_LDADD += -ldl -lX11 -lm

if EMSCRIPTEN
arculatorweb_CFLAGS += -sUSE_ZLIB=1 -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -O2
endif
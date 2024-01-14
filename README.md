# Arculator WASM

[Arculator](https://github.com/sarah-walker-pcem/arculator) is an 
authentic [Acorn Archimedes](https://en.wikipedia.org/wiki/Acorn_Archimedes)
emulator by [Sarah Walker](https://github.com/sarah-walker-pcem/arculator).

This is a modernized port by [Paul Stone](https://github.com/pdjstone/) and
[Matthew Bloch](https://github.com/matthewbloch/) which is built to run smoothly 
on the web via [Emscripten](https://emscripten.org/).

## Current state

We're rebuilding the front-end for Arculator; the old wxWidgets code is 
redundant in this fork. The best front-end is through the
[Archimedes Live!](https://github.com/pdjstone/archimedes-live) project 
which is online at [https://archi.medes.live/](https://archi.medes.live/).

Podules are in a very basic state; only the Ultimate CD-ROM works at the 
moment.  HostFS and floppy disk images work, but all the files need to be 
preloaded at build time.

The main _improvements_ to the core are:

* the display is rendered through OpenGL, and scales well;
* the pointer supports absolute positioning.

## Building

This fork also has a new build system that outputs Linux, Windows and Web 
versions from a single Makefile.

The fastest way to build is to install [Docker](https://docker.com/) and
run `./make-docker -j8 DEBUG=1 all` to build all three versions.

The build tools it needs are specified in the `Dockerfile`, so if you're
working with the code regularly, you might want to install all that locally.

You'll get three versions:

- Linux under `build/native/arculator`
- Windows under build/win64/arculator.exe`
- WebAssembly under `build/wasm/arculator.html`

## Testing the web version

To try the WebAssembly version run `make -j8 DEBUG=1 serve`. After building 
it, you should be able to launch Arculator at http://localhost:3020/ .

If you reload the page, the web server should rebuild any source changes 
first.

The web front-end currently doesn't have any way to configure it at runtime.

## Testing the native version

You can launch the Linux or Windows versions reliably from the build directory, 
e.g. `./build/native/arculator` ought to pop up a window and boot RISC OS.

The web front-end currently doesn't have any way to configure it at runtime,
except via the separate
[Archimedes Live front-end project](https://github.com/pdjstone/archimedes-live).

## ROM status

RISC OS is still under copyright, and is vital to get a working build.

We will not distribute RISC OS with this source code, but the build script
fetches it from Sarah Walker's 
[Arculator releases](https://b-em.bbcmicro.com/arculator/download.html).

So you should not need to take any action to get a working Archimedes when
building from this fork, but the situation is a bit fragile.  Other sources
for RISC OS ROMs include:

* [archive.org Ultimate RISC OS Collection](https://archive.org/details/RISC-OS-ROM-collection)
* [4corn](https://www.4corn.co.uk/aview.php?sPath=/roms)

Most podule emulations require a ROM that (as far as we know) is only available
from Sarah's Arculator builds. We'll pull those in as we're able to support the
podule emulations again.

Arculator WASM
==============

A port of Arculator to WebAssembly using Emscripten.

What works:

* Graphics
* Mouse
* Keyboard
* Sound
* HostFS
* Loading disks via web interface

Not working:

* Fullscreen
* Podules

To build and test the Emscripten version:

* Install [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) which contains the whole build environment
* Run `make -j8 serve`
* Open [http://localhost:8000/build/wasm/arculator.html](http://localhost:8000/build/arculator.html) to see the default Emscripten front-end which should boot RISC OS 3.

You can also build a native equivalent for debugging by running `make -j8 native`.

We're working on a better front-end at the [Archimedes Live](https://github.com/pdjstone/archimedes-live) project. Join us!

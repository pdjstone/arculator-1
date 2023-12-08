Arculator WASM
==============

A port of [Arculator](https://github.com/sarah-walker-pcem/arculator) (an [Acorn Archimedes](https://en.wikipedia.org/wiki/Acorn_Archimedes) emulator) to WebAssembly using Emscripten.

What works:

* Graphics
* Sound
* Mouse
* Keyboard
* HostFS (via Emscripten MemFS)
* Loading disc images (via MemFS)

Not working or tested:

* Podules (WIP, can be compiled statically)
* Hard drives

To build and test the Emscripten version:

* Install [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) which contains all the tools to build the WebAssembly version.
* Activate Emscripten with e.g. `EMSDK_QUIET=1 source ~/emsdk/emsdk_env.sh`
* Run `make -j8 serve`
* Open [http://localhost:3020/](http://localhost:3020/) to see the default Emscripten front-end which should boot RISC OS 3.

You can also build a native equivalent by running `make -j8 native DEBUG=1`.

We're working on a better front-end at the [Archimedes Live](https://github.com/pdjstone/archimedes-live) project. Join us!

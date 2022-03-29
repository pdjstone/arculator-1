Arculator WASM
==============

Compile Arculator to WebAssembly using Emscripten.

What works:
 * Booting to the RISC OS 3 desktop
 * Mouse (difficult to control until the browser captures the cursor)
 * Keyboard
 * Sound (can be a bit glitchy)
 * Loading disks via web interface

Not working:
 * Fullscreen / resizing 
 * Podules

TODO:
 * Machine configuration
 * Javascript integration/UI
 * HostFS support 
 * Fix sound glitches
 * Dynamic frame rate support
 * Improve sound quality, particularly on Firefox


Build instructions:

Install emscripten - see https://emscripten.org/docs/getting_started/downloads.html
Then run these commands:

source /path/to/emsdk/emsdk_env.sh
emconfigure ./configure --enable-debug --disable-podules
emmake make

The output html/js/wasm files should be in emscripten_out. You can test them locally by running:

python3 -m http.server -d emscripten_out

You can then load http://localhost:8000/arculator.html in your browser to try it out
Arculator WASM
==============

Compile Arculator to WebAssembly using Emscripten.

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


Build instructions:

Install emscripten - see https://emscripten.org/docs/getting_started/downloads.html
Then run these commands:

source /path/to/emsdk/emsdk_env.sh
emconfigure ./configure --enable-debug --disable-podules
emmake make

The output html/js/wasm files should be in emscripten_out. You can test them locally by running:

python3 -m http.server -d emscripten_out

You can then load http://localhost:8000/arculator.html in your browser to try it out
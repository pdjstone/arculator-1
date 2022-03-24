Arculator v2.1 Emscripten build

What works:
* Booting to the RISC OS 3 desktop
* Mouse
* Keyboard

Not working:
* Sound (it should work as Arculator uses sound API supported by emscripten)

TODO:
* Machine configuration
* Loading disks
* Javascript integration/UI

Performance:
Currently runs < 100% speed on a fairly fast machine. Need to investigate whether this 
is due to slow performance of emulation under WASM or if there is any low-hanging fruit 
that can be improved (e.g. how main loop or graphics are handled)

Building:

Install emscripten - see https://emscripten.org/docs/getting_started/downloads.html

source /path/to/emscripten/emsdk_env.sh
emconfigure ./configure --enable-debug --disable-podules
emmake make

The output html/js/wasm files should be in emscripten_out. You can test them locally by running:

python3 -m http.server -d emscripten_out

You can then load http://localhost:8000/arculator.html in your browser to try it out
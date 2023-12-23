# Build environment for arculator
#
# Build with: docker build -t arculator-build .
# Run with: docker run -u $(id -u):$(id -g) -it --rm -v $(pwd):/src arculator-build
FROM debian:bookworm

VOLUME /src

RUN apt update && apt -yy install curl xz-utils git gcc make libsdl2-dev libz-dev libglu1-mesa-dev mingw-w64 mingw-w64-x86-64-dev 
RUN apt -yy install libz-mingw-w64-dev
RUN git clone https://github.com/emscripten-core/emsdk.git /emsdk
RUN cd /emsdk && ./emsdk install latest && ./emsdk activate latest
ENV EMSDK=/emsdk
ENV EM_CACHE=/tmp/emcache
ENV PATH=/emsdk:/emsdk/upstream/emscripten:$PATH
WORKDIR /src

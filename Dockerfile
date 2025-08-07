FROM emscripten/emsdk

RUN apt update && apt install -y pkg-config
RUN apt update && apt install -y liburing-dev libssl-dev  libcmocka-dev 

RUN git clone https://git.cryptomilk.org/projects/cmocka.git
RUN cd cmocka && cmake \
          -S . \
	  -B build.wasm \
	  -DCMAKE_C_FLAGS=" -sSUPPORT_LONGJMP=wasm" \
	  -DCMAKE_CXX_FLAGS="-fwasm-exceptions -sSUPPORT_LONGJMP=wasm" \
	  -DCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
RUN emmake make -C cmocka/build.wasm
RUN emmake make -C cmocka/build.wasm install


RUN git clone  --depth 1 --recurse-submodules --shallow-submodules  https://github.com/jedisct1/openssl-wasm.git


RUN  chmod -R ug+ws /emsdk/upstream/emscripten/cache
RUN  chown -R emscripten  /emsdk/upstream/emscripten/cache/*
RUN  chgrp -R emscripten  /emsdk/upstream/emscripten/cache/*



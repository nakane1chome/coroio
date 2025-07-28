USER:=${shell id -u}
GROUP:=${shell id -g}
DOCKER=docker run --rm -v ${CURDIR}:/src -v ${CURDIR}:${CURDIR} -u ${USER}:${GROUP}
#DOCKER_IMG=emscripten/emsdk
DOCKER_IMG=extended_emscripten
EMCC=${DOCKER} ${DOCKER_IMG} emcc
EMAR=${DOCKER} ${DOCKER_IMG} ar
EMMAKE=${DOCKER} ${DOCKER_IMG} emmake
EMRUN=${DOCKER} -p 6931:6931 ${DOCKER_IMG} emrun --no_browser
CMAKE=${DOCKER} ${DOCKER_IMG} cmake

CMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake

all : build.wasm/coroio/libcoroio.a

build.wasm/Makefile :
	if [ ! -d build.wasm ] ; then mkdir build.wasm ; fi
	docker build -t extended_emscripten .
	${CMAKE} \
		-S ${CURDIR} \
		-B build.wasm \
		-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}


build.wasm/coroio/libcoroio.a :  build.wasm/Makefile
	${EMMAKE} make -C build.wasm

shell :
	${DOCKER} -it ${DOCKER_IMG} bash

clean :
	rm -rf build.wasm

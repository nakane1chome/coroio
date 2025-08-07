USER:=${shell id -u}
GROUP:=${shell id -g}
DOCKER=docker run --rm -v ${CURDIR}:/src -v ${CURDIR}:${CURDIR} -u ${USER}:${GROUP}
#DOCKER_IMG=emscripten/emsdk
DOCKER_IMG=extended_emscripten
EMCC=${DOCKER} ${DOCKER_IMG} emcc
EMAR=${DOCKER} ${DOCKER_IMG} ar
EMMAKE=${DOCKER} ${DOCKER_IMG} emmake
EMRUN=${DOCKER} -p 6931:6931 ${DOCKER_IMG} emrun --no_browser
EMCMAKE=${DOCKER} ${DOCKER_IMG} cmake

EMCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
CMAKE=cmake

wasm : build.wasm/coroio/libcoroio.a

host : build.host/coroio/libcoroio.a

docker:
	docker build -t extended_emscripten .

build.wasm/Makefile :
	if [ ! -d build.wasm ] ; then mkdir build.wasm ; fi
	docker build -t extended_emscripten .
	${EMCMAKE} \
		-S ${CURDIR} \
		-B build.wasm \
		-DCMAKE_TOOLCHAIN_FILE=${EMCMAKE_TOOLCHAIN_FILE}

deps:
	sudo apt-get install -y liburing-dev
	sudo apt-get install libssl-dev
	sudo apt-get install cmock
	sudo apt-get install libcmocka-dev 


build.host/Makefile :
	if [ ! -d build.host ] ; then mkdir build.host ; fi
	${CMAKE} \
		-S . \
		-B build.host

build.host/coroio/libcoroio.a :  build.host/Makefile
	${MAKE} -C build.host

build.wasm/coroio/libcoroio.a :  build.wasm/Makefile
	${EMMAKE} make -C build.wasm

build.wasm.code : 
	${EMMAKE} make -C build.wasm

shell :
	${DOCKER} -it ${DOCKER_IMG} bash

clean :
	rm -rf build.wasm

examples_fail:
	node build.wasm/examples/echotest.js

examples_pass:
	node ./build.wasm/examples/bench.js 

examples_ok:
	node ./build.wasm/examples/resolver.js  --help
	node ./build.wasm/examples/wsclient.js  --uri wss://echo.websocket.org/

examples_host:
	./build.host/examples/wsclient  --uri wss://echo.websocket.org/.sse


test.host:
	./build.host/tests/ut_tests
	./build.host/tests/ut_test_actors

test.wasm:
	node build.wasm/tests/ut_test_actors.js
	node build.wasm/tests/ut_tests.js

rebuild.wasm: 
	${EMMAKE} make -C build.wasm

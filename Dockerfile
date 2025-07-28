FROM emscripten/emsdk

RUN apt update && apt install -y pkg-config

cmake_minimum_required(VERSION 3.22)
if (EXISTS ${CMAKE_BINARY_DIR}/local.cmake)
include(${CMAKE_BINARY_DIR}/local.cmake)
endif ()

project(netlib)
enable_testing()

if (EMSCRIPTEN)
    #set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_compile_options(-fwasm-exceptions -sSUPPORT_LONGJMP=wasm )
    add_link_options(-fwasm-exceptions -sSUPPORT_LONGJMP=wasm )
endif()


add_subdirectory(coroio)
add_subdirectory(coroio/actors)
add_subdirectory(examples)
add_subdirectory(tests)

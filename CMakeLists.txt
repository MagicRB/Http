cmake_minimum_required(VERSION 3.1)
project(Http)

set (CMAKE_CXX_STANDARD 11)

include_directories(include)

file(GLOB SOURCES "src/*.cpp" "include/*.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

add_executable(Http ${SOURCES})

target_link_libraries(Http pthread)

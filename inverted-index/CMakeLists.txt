cmake_minimum_required(VERSION 3.10)

project(InvertedIndex)

set(CMAKE_CXX_STANDARD 20)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    /usr/local/include/oleander
    /usr/local/include/croaring
)

file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
add_executable(inverted_index ${SOURCES})

link_directories(
    /usr/local/lib
)

target_link_libraries(inverted_index roaring)
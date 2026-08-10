cmake_minimum_required(VERSION 3.10)
project(asciiDJ CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(asciiDJ
    src/main.cpp
    src/Deck.cpp
    src/Mixer.cpp
    third_party/miniaudio/miniaudio_impl.cpp
)

target_include_directories(asciiDJ PRIVATE
    include
    third_party/miniaudio
)

target_link_libraries(asciiDJ PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)

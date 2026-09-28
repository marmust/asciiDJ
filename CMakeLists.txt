cmake_minimum_required(VERSION 3.20)
project(asciiDJ CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

add_executable(asciiDJ
    src/main.cpp
    src/Deck.cpp
    src/Mixer.cpp
    src/Equalizer.cpp
    src/InputReader.cpp
    src/InputInterpreter.cpp
    src/CentralController.cpp
    src/DirReader.cpp
    src/FileLoadManager.cpp
    src/Renderer.cpp
    src/TUIdisplay.cpp
    src/GraphicsComposer.cpp
    third_party/miniaudio/miniaudio_impl.cpp
)

target_include_directories(asciiDJ PRIVATE
    include
    third_party/miniaudio
)

target_link_libraries(asciiDJ PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m X11::X11)

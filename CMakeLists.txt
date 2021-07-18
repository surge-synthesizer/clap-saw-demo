cmake_minimum_required(VERSION 3.17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11 CACHE STRING "Build for 10.1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(stupisaw VERSION 0.9.0 LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)

add_subdirectory(libs/clap EXCLUDE_FROM_ALL)
add_subdirectory(libs/readerwriterqueue EXCLUDE_FROM_ALL)

add_library(stupisaw SHARED
        src/stupisaw.cpp
        src/stupivoice.cpp
        src/stupisaw_pluginentry.cpp)

if(APPLE)
    target_compile_definitions(stupisaw PRIVATE USE_MAC_UI=1 HAS_GUI=1)
    target_sources(stupisaw PRIVATE src/stupisaw_macgui.mm)
    target_link_libraries(stupisaw "-framework CoreFoundation" "-framework AppKit" "-framework CoreGraphics")
endif()

target_link_libraries(stupisaw clap-plugin-core readerwriterqueue)
set_target_properties(stupisaw PROPERTIES SUFFIX ".clap" PREFIX "")

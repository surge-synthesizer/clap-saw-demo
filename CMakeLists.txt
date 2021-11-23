cmake_minimum_required(VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11 CACHE STRING "Build for 10.1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(stupisaw VERSION 0.9.0 LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libs/clap EXCLUDE_FROM_ALL)
add_subdirectory(libs/clap-helpers EXCLUDE_FROM_ALL)
add_subdirectory(libs/readerwriterqueue EXCLUDE_FROM_ALL)

add_library(stupisaw MODULE
        src/stupisaw.cpp
        src/stupivoice.cpp
        src/stupisaw_pluginentry.cpp)
target_link_libraries(stupisaw clap-core clap-helpers readerwriterqueue)

if(APPLE)
    set_target_properties(stupisaw PROPERTIES
            BUNDLE True
            BUNDLE_EXTENSION clap
            MACOSX_BUNDLE_GUI_IDENTIFIER org.baconpaul.stupisaw
            MACOSX_BUNDLE_BUNDLE_NAME stupisaw
            MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/stupisaw.plist.in
            )
    target_compile_definitions(stupisaw PRIVATE USE_MAC_UI=1 HAS_GUI=1)
    target_sources(stupisaw PRIVATE src/stupisaw_macgui.mm)
    target_link_libraries(stupisaw "-framework CoreFoundation" "-framework AppKit" "-framework CoreGraphics")
else()
    set_target_properties(stupisaw PROPERTIES SUFFIX ".clap" PREFIX "")
endif()
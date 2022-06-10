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
add_subdirectory(libs/vstgui EXCLUDE_FROM_ALL)

add_library(stupisaw MODULE
        src/stupisaw.cpp
        src/stupieditor.cpp
        src/stupivoice.cpp
        src/stupisaw_pluginentry.cpp)
target_link_libraries(stupisaw vstgui clap-core clap-helpers readerwriterqueue)
# Oh VSTGUI why is your cmake so wierd and broken
target_include_directories(stupisaw PUBLIC libs/vstgui)
target_compile_definitions(stupisaw PRIVATE HAS_GUI=1)

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
    target_link_libraries(stupisaw "-framework CoreFoundation" "-framework AppKit" "-framework CoreGraphics")

    set(products_folder ${CMAKE_BINARY_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${PROJECT_NAME}.clap to ~/Library/Audio/Plug-Ins/CLAP/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/CLAP"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${PROJECT_NAME}.clap" "~/Library/Audio/Plug-Ins/CLAP/${PROJECT_NAME}.clap"
            )

else()
    set_target_properties(stupisaw PROPERTIES SUFFIX ".clap" PREFIX "")
endif()
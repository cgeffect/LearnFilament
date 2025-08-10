cmake_minimum_required(VERSION 3.21)
project(sdl_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES "arm64")

set(SDL3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libSDL/deploy/include)
set(SDL3_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libSDL/deploy/lib/libSDL3.a)

# Find required frameworks
find_library(COREMEDIA_FRAMEWORK CoreMedia)
find_library(COREVIDEO_FRAMEWORK CoreVideo)
find_library(COCOA_FRAMEWORK Cocoa)
find_library(IOKIT_FRAMEWORK IOKit)
find_library(FORCEFEEDBACK_FRAMEWORK ForceFeedback)
find_library(CARBON_FRAMEWORK Carbon)
find_library(COREAUDIO_FRAMEWORK CoreAudio)
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
find_library(AVFOUNDATION_FRAMEWORK AVFoundation)
find_library(FOUNDATION_FRAMEWORK Foundation)
find_library(GAMECONTROLLER_FRAMEWORK GameController)
find_library(METAL_FRAMEWORK Metal)
find_library(QUARTZCORE_FRAMEWORK QuartzCore)
find_library(COREHAPTICS_FRAMEWORK CoreHaptics)
find_library(UNIFORMTYPEIDENTIFIERS_FRAMEWORK UniformTypeIdentifiers)

add_executable(sdl_test sdl_test.cpp)
target_include_directories(sdl_test PRIVATE ${SDL3_INCLUDE_DIR})
target_link_libraries(sdl_test PRIVATE ${SDL3_LIBRARY} pthread m 
    ${COREMEDIA_FRAMEWORK} ${COREVIDEO_FRAMEWORK} ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK}
    ${FORCEFEEDBACK_FRAMEWORK} ${CARBON_FRAMEWORK} ${COREAUDIO_FRAMEWORK} ${AUDIOTOOLBOX_FRAMEWORK}
    ${AVFOUNDATION_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${GAMECONTROLLER_FRAMEWORK}
    ${METAL_FRAMEWORK} ${QUARTZCORE_FRAMEWORK} ${COREHAPTICS_FRAMEWORK}
    ${UNIFORMTYPEIDENTIFIERS_FRAMEWORK} objc) 
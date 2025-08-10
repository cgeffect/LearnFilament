cmake_minimum_required(VERSION 3.21)

project(hello_filament)

set(CMAKE_C_FLAGS_DEBUG   "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# 设置目标架构为
set(CMAKE_SYSTEM_PROCESSOR "arm64")
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND LIVE_TRD_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/filament/include/)
list(APPEND LIVE_TRD_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libSDL/deploy/include/)

set(SDL3_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libSDL/deploy/lib/libSDL3.a)
file(GLOB_RECURSE filament_lib ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/filament/lib/arm64/*.a)

# finds all required platform libraries.
find_library(LZMA_FRAMEWORK lzma)
find_library(LZ_FRAMEWORK z)
find_library(LBZ2_FRAMEWORK bz2)
find_library(LICONV_FRAMEWORK iconv)
set(SYS_LIBS ${LZMA_FRAMEWORK} ${LZ_FRAMEWORK} ${LBZ2_FRAMEWORK} ${LICONV_FRAMEWORK})
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
list(APPEND SYS_LIBS ${COREFOUNDATION_FRAMEWORK})
find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
list(APPEND SYS_LIBS ${COREVIDEO_FRAMEWORK})
find_library(VIDEOTOOLBOX VideoToolbox)
list(APPEND SYS_LIBS ${VIDEOTOOLBOX})
find_library(CORE_MEDIA CoreMedia)
list(APPEND SYS_LIBS ${CORE_MEDIA})
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
list(APPEND SYS_LIBS ${AUDIOTOOLBOX_FRAMEWORK})
find_library(SECURITY_FRAMEWORK Security)
list(APPEND SYS_LIBS ${SECURITY_FRAMEWORK})
# libavfilter使用
find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
list(APPEND SYS_LIBS ${COREGRAPHICS_FRAMEWORK})
find_library(COREIMAGE_FRAMEWORK CoreImage)
list(APPEND SYS_LIBS ${COREIMAGE_FRAMEWORK})
find_library(Metal_FRAMEWORK Metal)
list(APPEND SYS_LIBS ${Metal_FRAMEWORK})
find_library(APP_FRAMEWORK AppKit)
list(APPEND SYS_LIBS ${APP_FRAMEWORK})
find_library(OpenGL_FRAMEWORK OpenGL)
list(APPEND SYS_LIBS ${OpenGL_FRAMEWORK})
# SDL 使用
set(SDL3_LIBS -Wl,-framework,CoreMedia -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,IOKit -Wl,-framework,ForceFeedback -Wl,-framework,Carbon -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,AVFoundation -Wl,-framework,Foundation -Wl,-weak_framework,GameController -Wl,-weak_framework,Metal -Wl,-weak_framework,QuartzCore -Wl,-framework,CoreHaptics -lpthread -Wl,-weak_framework,UniformTypeIdentifiers -lm)
list(APPEND SYS_LIBS ${SDL3_LIBS})

# build exec
add_executable(01-triangle ${CMAKE_CURRENT_SOURCE_DIR}/macos-demo/01-triangle/main.cpp)
target_include_directories(01-triangle PRIVATE ${LIVE_TRD_INCLUDE})
target_link_libraries(01-triangle PRIVATE ${SYS_LIBS} ${SDL3_LIBRARY} ${filament_lib})

add_executable(02-cube ${CMAKE_CURRENT_SOURCE_DIR}/macos-demo/02-cube/main.cpp)
target_include_directories(02-cube PRIVATE ${LIVE_TRD_INCLUDE})
target_link_libraries(02-cube PRIVATE ${SYS_LIBS} ${SDL3_LIBRARY} ${filament_lib})

add_executable(03-morphing ${CMAKE_CURRENT_SOURCE_DIR}/macos-demo/03-morphing/main.cpp)
target_include_directories(03-morphing PRIVATE ${LIVE_TRD_INCLUDE})
target_link_libraries(03-morphing PRIVATE ${SYS_LIBS} ${SDL3_LIBRARY} ${filament_lib})

message("--end rtcapp complie---")
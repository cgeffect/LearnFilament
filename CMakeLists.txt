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

list(APPEND LIVE_TRD_INCLUDE ./src)
list(APPEND LIVE_TRD_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/filament/include/)

# file(GLOB_RECURSE libs1 ${CMAKE_CURRENT_SOURCE_DIR}/filament/lib/*.a)
file(GLOB_RECURSE libs2 ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/filament/lib/arm64/*.a)

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

file(GLOB_RECURSE allCpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

add_executable(hello_filament ${allCpp})
target_include_directories(hello_filament PRIVATE ${LIVE_TRD_INCLUDE})
target_link_libraries(hello_filament PRIVATE ${SYS_LIBS} ${libs1} ${libs2})

message("--end rtcapp complie---")
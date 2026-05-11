cmake_minimum_required(VERSION 3.18)

project(rc_native LANGUAGES C CXX)

include(FetchContent)

find_package(Threads REQUIRED)

if(ANDROID)
  if(NOT DEFINED CMAKE_ANDROID_NDK AND NOT DEFINED ANDROID_NDK)
    message(FATAL_ERROR "Android build requires the Android NDK CMake toolchain.")
  endif()

  message(STATUS "Building rc_native with Android NDK toolchain")
endif()

option(RC_NATIVE_USE_SYSTEM_MBEDTLS "Use an already installed mbedTLS package" OFF)
if(WIN32)
  set(RC_NATIVE_ENABLE_CODEC_DEFAULT ON)
else()
  set(RC_NATIVE_ENABLE_CODEC_DEFAULT OFF)
endif()
option(RC_NATIVE_ENABLE_CODEC "Build OpenH264/libyuv H264 encoder" ${RC_NATIVE_ENABLE_CODEC_DEFAULT})

if(RC_NATIVE_USE_SYSTEM_MBEDTLS)
  find_package(MbedTLS REQUIRED)
else()
  set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbedTLS programs" FORCE)
  set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedTLS tests" FORCE)
  set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "Disable mbedTLS fatal warnings" FORCE)
  set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON CACHE BOOL "Disable mbedTLS install rules" FORCE)
  set(INSTALL_MBEDTLS_HEADERS OFF CACHE BOOL "Disable mbedTLS header install rules" FORCE)
  FetchContent_Declare(
    mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG v3.6.2
  )
  FetchContent_GetProperties(mbedtls)
  if(NOT mbedtls_POPULATED)
    FetchContent_Populate(mbedtls)
    add_subdirectory(${mbedtls_SOURCE_DIR} ${mbedtls_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

if(RC_NATIVE_ENABLE_CODEC)
  FetchContent_Declare(
    openh264
    GIT_REPOSITORY https://github.com/cisco/openh264.git
    GIT_TAG 8c7008aeb6335e7d36ab0d9a023a63f82a8eaac0
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(openh264)
  if(NOT openh264_POPULATED)
    FetchContent_Populate(openh264)
  endif()

  FetchContent_Declare(
    libyuv
    GIT_REPOSITORY https://chromium.googlesource.com/libyuv/libyuv
    GIT_TAG e23282704f3b731534167ee2c689bda574c49dca
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(libyuv)
  if(NOT libyuv_POPULATED)
    FetchContent_Populate(libyuv)
  endif()

  file(GLOB_RECURSE OPENH264_SOURCES CONFIGURE_DEPENDS
    "${openh264_SOURCE_DIR}/codec/api/svc/*.cpp"
    "${openh264_SOURCE_DIR}/codec/common/src/*.cpp"
    "${openh264_SOURCE_DIR}/codec/encoder/core/src/*.cpp"
    "${openh264_SOURCE_DIR}/codec/encoder/plus/src/*.cpp"
    "${openh264_SOURCE_DIR}/codec/processing/src/*.cpp"
  )
  if(NOT WIN32)
    list(FILTER OPENH264_SOURCES EXCLUDE REGEX ".*/DllEntry\\.cpp$")
  endif()

  add_library(openh264_static STATIC ${OPENH264_SOURCES})
  target_include_directories(openh264_static PUBLIC
    "${openh264_SOURCE_DIR}/codec/api/wels"
    "${openh264_SOURCE_DIR}/codec/api/svc"
    "${openh264_SOURCE_DIR}/codec/common/inc"
    "${openh264_SOURCE_DIR}/codec/common/src"
    "${openh264_SOURCE_DIR}/codec/encoder/core/inc"
    "${openh264_SOURCE_DIR}/codec/encoder/plus/inc"
    "${openh264_SOURCE_DIR}/codec/processing/interface"
    "${openh264_SOURCE_DIR}/codec/processing/src/common"
  )
  target_compile_features(openh264_static PRIVATE cxx_std_17)

  file(GLOB LIBYUV_SOURCES CONFIGURE_DEPENDS "${libyuv_SOURCE_DIR}/source/*.cc")
  if(ANDROID)
    list(FILTER LIBYUV_SOURCES EXCLUDE REGEX ".*/.*_(any|gcc|mips|msa|neon|neon64|sve|sme|lsx|lasx|rvv|win)\\.cc$")
    list(FILTER LIBYUV_SOURCES EXCLUDE REGEX ".*/(compare|compare_common|mjpeg_decoder|mjpeg_validate|rotate|rotate_argb|scale|scale_argb|scale_common|scale_rgb|scale_uv|scale_win)\\.cc$")
  endif()
  add_library(libyuv_static STATIC ${LIBYUV_SOURCES})
  target_include_directories(libyuv_static PUBLIC "${libyuv_SOURCE_DIR}/include")
  target_compile_features(libyuv_static PRIVATE cxx_std_17)
  if(MSVC OR ANDROID)
    target_compile_definitions(libyuv_static PRIVATE LIBYUV_DISABLE_X86)
  endif()
endif()

add_library(rc_native SHARED
  src/rc_api.cpp
  src/rc_runtime.cpp
  src/socket_platform.cpp
  src/tcp_transport.cpp
  src/tls_transport.cpp
  src/udp_discovery.cpp
  src/udp_discovery_responder.cpp
)

if(RC_NATIVE_ENABLE_CODEC)
  target_sources(rc_native PRIVATE src/h264_encoder.cpp)
endif()

if(WIN32)
  target_sources(rc_native PRIVATE src/dxgi_capture.cpp)
elseif(ANDROID)
  target_sources(rc_native PRIVATE src/android_capture.cpp)
elseif(UNIX)
  target_sources(rc_native PRIVATE
    src/linux_capture_factory.cpp
    src/pipewire_capture.cpp
    src/x11_capture.cpp
  )
endif()

target_include_directories(rc_native
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(rc_native PRIVATE cxx_std_17)

target_compile_definitions(rc_native PRIVATE RC_NATIVE_BUILDING_LIBRARY)

target_link_libraries(rc_native PRIVATE mbedtls mbedx509 mbedcrypto Threads::Threads)

if(RC_NATIVE_ENABLE_CODEC)
  target_compile_definitions(rc_native PRIVATE RC_NATIVE_ENABLE_OPENH264)
  target_link_libraries(rc_native PRIVATE openh264_static libyuv_static)
endif()

if(WIN32)
  target_link_libraries(rc_native PRIVATE ws2_32 d3d11 dxgi)
endif()

set_target_properties(rc_native PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  C_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
  OUTPUT_NAME "rc_native"
)

if(MSVC)
  target_compile_options(rc_native PRIVATE /W4 /WX)
else()
  target_compile_options(rc_native PRIVATE -Wall -Wextra -Werror)
endif()

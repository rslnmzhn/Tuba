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
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(mbedtls)
  if(NOT mbedtls_POPULATED)
    FetchContent_Populate(mbedtls)
    add_subdirectory(${mbedtls_SOURCE_DIR} ${mbedtls_BINARY_DIR} EXCLUDE_FROM_ALL)
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

target_include_directories(rc_native
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(rc_native PRIVATE cxx_std_17)

target_compile_definitions(rc_native PRIVATE RC_NATIVE_BUILDING_LIBRARY)

target_link_libraries(rc_native PRIVATE mbedtls mbedx509 mbedcrypto Threads::Threads)

if(WIN32)
  target_link_libraries(rc_native PRIVATE ws2_32)
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

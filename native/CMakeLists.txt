cmake_minimum_required(VERSION 3.18)

project(rc_native LANGUAGES C CXX)

if(ANDROID)
  if(NOT DEFINED CMAKE_ANDROID_NDK AND NOT DEFINED ANDROID_NDK)
    message(FATAL_ERROR "Android build requires the Android NDK CMake toolchain.")
  endif()

  message(STATUS "Building rc_native with Android NDK toolchain")
endif()

add_library(rc_native SHARED
  src/rc_api.cpp
)

target_include_directories(rc_native
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(rc_native PRIVATE cxx_std_17)

target_compile_definitions(rc_native PRIVATE RC_NATIVE_BUILDING_LIBRARY)

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

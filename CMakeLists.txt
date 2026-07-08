cmake_minimum_required(VERSION 4.0)
project(
  loki23
  VERSION 0.0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
#   if(MSVC)
#     add_compile_options(/W4)
#   else()
#     add_compile_options(
#       -O0
#       -g3
#       -pedantic-errors
#       -fsanitize=address,undefined,leak
#       -Wall
#       -Wextra
#       -Wpedantic
#       -Wshadow
#       -Wconversion
#       -Wfloat-equal
#       -Wformat-nonliteral
#       -Wformat-security
#       -Wformat-y2k
#       -Wformat=2
#       -Wimport
#       -Winvalid-pch
#       -Wmissing-declarations
#       -Wmissing-field-initializers
#       -Wmissing-format-attribute
#       -Wmissing-include-dirs
#       -Wmissing-noreturn
#       -Wpacked
#       -Wpointer-arith
#       -Wredundant-decls
#       -Wstack-protector
#       -Wswitch-enum
#       -Wundef
#       -Wwrite-strings)
#       if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#         add_compile_options(
#           -fmodules-ts
#           -Wlogical-op
#           -Wstrict-null-sentinel
#         )
#       endif()
#       add_compile_options(-fdiagnostics-color=always)
#   endif()
#   add_link_options(-fsanitize=address,undefined,leak)
# endif()

add_subdirectory(lib)
add_subdirectory(src)
if(NOT WIN32)
  add_subdirectory(netprobe)
endif()

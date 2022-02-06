#
# Copyright 2022, Werner Dittmann
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This is a smale helper CMakeLists.txt file which creates a small lib which
# includes a well defined set of Botan modules. The calling CMakeLists.txt
# defines the required modules in variable BOTAN_MODULES (or as -D argument)
#
# A top level CMakeLists.txt usually uses this helper after it fetched this
# forked Botan repository (see cmake's FETCH_CONTENT mechanism). This file
# checks some Android, iOS or other arguments, sets the correct flags to
# compile Botan for the target platform, compiles and bilds the library.
project(botanlib)

cmake_minimum_required(VERSION 3.20.0)

set(LIBRARY_BUILD_TYPE "STATIC")      # or SHARED

if ("${BOTAN_LIB_NAME}" STREQUAL "")
    set (BOTAN_LIB_NAME botanstatic)
endif ()

option(ANDROID "Build for Android" OFF)
option(IOS "Build for iOS" OFF)

if (ANDROID OR IOS)
    set(EMBEDDED true)
endif()

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -D_REENTRANT -fPIC -g -fstack-protector -pthread")

if ("${BOTAN_MODULES}" STREQUAL "")
    MESSAGE(FATAL "No Botan module defined for amalgamation")
endif ()

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(basicCommand ${TOP_DIR}/configure.py --minimized-build --amalgamation
	       --cc=clang --enable-modules=${BOTAN_MODULES})

if (ANDROID)
    if("${ANDROID_ABI}" STREQUAL "x86_64")
        set(cpu x86_64)
        set(target --target=x86_64-none-linux-android${CMAKE_SYSTEM_VERSION})
    elseif("${ANDROID_ABI}" STREQUAL "armeabi-v7a")
        set(cpu arm32)
        set(target --target=armv7-none-linux-androideabi${CMAKE_SYSTEM_VERSION})
    elseif("${ANDROID_ABI}" STREQUAL "arm64-v8a")
        set(cpu arm64)
        set(target --target=aarch64-none-linux-android${CMAKE_SYSTEM_VERSION})
    elseif("${ANDROID_ABI}" STREQUAL "x86")
        set(cpu x86)
        set(target --target=i686-none-linux-android${CMAKE_SYSTEM_VERSION})
    else()
        MESSAGE(FATAL_ERROR "Unknown Android architecture: ${ANDROID_ABI}")
    endif()

    set(configCommand ${basicCommand} --os=android --cpu=${cpu} --cc-abi-flags=${target})

elseif(IOS)
    if (CMAKE_OSX_ARCHITECTURES STREQUAL "armv7")
        set(prefix iphone-32)
        set(cpu armv7)
        set(target "-arch armv7")
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(prefix iphone-64)
        set(cpu arm64)
        set(target "-arch arm64")
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(prefix iphone-simulator)
        set(cpu x86_64)
        set(target "-arch x86_64")
    else()
        message(FATAL_ERROR "Unknown iOS architecture: ${CMAKE_OSX_ARCHITECTURES}. Valid entries: armv7, arm64, x86_64")
    endif()

    set(configCommand ${basicCommand} --os=ios --prefix=${prefix} --cpu=${cpu} --cc-abi-flags=${target})
else()
    ## Amalgamation for Host system
    set(configCommand ${basicCommand})
endif()

set(ENV{CXX} ${CMAKE_CXX_COMPILER})     ## Botan configure.py uses CXX to detect/check compiler

## Run Botan configure to create the amalgamation files, then move them to
## build directory and remove artifacts
execute_process(COMMAND python ${configCommand}
        WORKING_DIRECTORY ${TOP_DIR}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdOut
        ERROR_VARIABLE errorOut
        COMMAND_ECHO STDERR
        )
#    MESSAGE("stdout: ${stdOut}")
if (NOT ${result} EQUAL 0)
    MESSAGE(FATAL_ERROR "botan amalgamation failed: error code: ${result}, error out: ${errorOut} standard out: ${stdOut}")
else()
    MESSAGE("##### botan amalgamation - DONE")
endif()

file(COPY ${TOP_DIR}/botan_all.cpp ${TOP_DIR}/botan_all.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(REMOVE_RECURSE ${TOP_DIR}/build)

set(botan_src
        ${CMAKE_CURRENT_BINARY_DIR}/botan_all.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/botan_all.h
        )

add_library(${BOTAN_LIB_NAME} STATIC ${botan_src})
target_include_directories(${BOTAN_LIB_NAME} PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR})

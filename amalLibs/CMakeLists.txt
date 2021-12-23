project(botanlib)

cmake_minimum_required(VERSION 3.20.0)

set(LIBRARY_BUILD_TYPE "STATIC")      # or SHARED

if ("${botanLib}" STREQUAL "")
    set (BotanLib botanstatic)
endif ()

option(ANDROID "Build for Android" OFF)
option(IOS "Build for iOS" OFF)

if (ANDROID OR IOS)
    set(EMBEDDED true)
endif()

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -D_REENTRANT -fPIC -g -fstack-protector -pthread")

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
    MESSAGE(FATAL_ERROR "Not yet implemented")
else()
    ## Amalgamation for Host system
    set(configCommand ${basicCommand})
endif()

set(ENV{CXX} ${CMAKE_CXX_COMPILER})     ## Botan configure.py uses CXX to detect/check compiler

## Run Botan configure to create the amalgamation files, then move them to
## build directory and remove artifacts
execute_process(COMMAND ${configCommand}
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

execute_process(COMMAND mv botan_all.cpp botan_all.h ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${TOP_DIR}
        )
execute_process(COMMAND rm -rf build
        WORKING_DIRECTORY ${TOP_DIR}
        )

set(botan_src
        ${CMAKE_CURRENT_BINARY_DIR}/botan_all.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/botan_all.h
        )

add_library(${botanLib} STATIC ${botan_src})
target_include_directories(${botanLib} PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR})

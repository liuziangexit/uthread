cmake_minimum_required(VERSION 2.8)
project(uthread)
ENABLE_LANGUAGE(C)

#debug flags
set(UTHREAD_DEBUG_FLAGS "-O0 -g -pg")

#release flags
set(UTHREAD_RELEASE_FLAGS "-O3")

#set C version: C11
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-std=c11" COMPILER_SUPPORTS_C11)
if(COMPILER_SUPPORTS_C11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
else()
    message(STATUS "The compiler ${CMAKE_C_COMPILER} has no C11 support.")    
endif()

#set warning level: enable all warning
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

#disable MSVC language extensions
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /Za")
endif()

message(STATUS "CMAKE_C_FLAGS     : ${CMAKE_C_FLAGS}")
message(STATUS "UTHREAD_DEBUG_FLAGS: ${UTHREAD_DEBUG_FLAGS}")
message(STATUS "UTHREAD_RELEASE_FLAGS: ${UTHREAD_RELEASE_FLAGS}")

message(STATUS "System            : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project           : ${PROJECT_NAME}")
message(STATUS "Project directory : ${PROJECT_SOURCE_DIR}")
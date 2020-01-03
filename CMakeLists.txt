cmake_minimum_required(VERSION 2.8)
project(uthread)
ENABLE_LANGUAGE(C)

#debug flags
set(UTHREAD_DEBUG_FLAGS "-O0 -g -pg")

#release flags
set(UTHREAD_RELEASE_FLAGS "-O3")

#set C version: C11
include(CheckCCompilerFlag)
check_c_compiler_flag("-std=c11" COMPILER_SUPPORTS_C11)
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

#options
SET(UTHREAD_STATIC_LINKING FALSE CACHE BOOL "Link libuthread statically")
SET(UTHREAD_OPTIMIZATION TRUE CACHE BOOL "Enable optimization")
SET(UTHREAD_ENABLE_HOOK TRUE CACHE BOOL "Enable system call hook")

message(STATUS "System            : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project           : ${PROJECT_NAME}")
message(STATUS "Project directory : ${PROJECT_SOURCE_DIR}")

#build libuthread
add_library(uthread STATIC src/uthread.c)
install(TARGETS uthread DESTINATION build/lib)

#build uthread_hook
add_library(uthread_hook SHARED src/uthread_hook.c)
install(TARGETS uthread_hook DESTINATION build/lib)

#build test
add_executable(test_yield test/yield.c)
target_link_libraries(test_yield uthread)
install(TARGETS test_yield DESTINATION build/test)

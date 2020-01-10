cmake_minimum_required(VERSION 2.8)
project(uthread)
ENABLE_LANGUAGE(C)

#debug flags
set(UTHREAD_DEBUG_FLAGS "-O0 -g -pg")

#release flags
set(UTHREAD_RELEASE_FLAGS "-O3")

#options
set(UTHREAD_STATIC_LINKING FALSE CACHE BOOL "Link libuthread statically")
set(UTHREAD_OPTIMIZATION TRUE CACHE BOOL "Enable optimization")
set(UTHREAD_ENABLE_HOOK TRUE CACHE BOOL "Enable system call hook")
set(UTHREAD_HOOK_STDOUT FALSE CACHE BOOL "Write infomations into standard output while hooked functions has been called")

#set C version: C11
include(CheckCCompilerFlag)
check_c_compiler_flag("-std=c11" COMPILER_SUPPORTS_C11)
if(COMPILER_SUPPORTS_C11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
else()
message(STATUS "The compiler ${CMAKE_C_COMPILER} has no C11 support.")    
endif()

#set debug or release flags
if(UTHREAD_OPTIMIZATION)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${UTHREAD_RELEASE_FLAGS}")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${UTHREAD_DEBUG_FLAGS}")
endif()

#set warning level: enable all warning
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

#disable MSVC language extensions
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Za")
endif()

message(STATUS "System            : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project           : ${PROJECT_NAME}")
message(STATUS "Project directory : ${PROJECT_SOURCE_DIR}")

#build libuthread
if(UTHREAD_STATIC_LINKING)
    add_library(uthread STATIC src/uthread.c)
    set_target_properties(uthread PROPERTIES ARCHIVE_OUTPUT_DIRECTORY build/lib)
else()
    add_library(uthread SHARED src/uthread.c)
    set_target_properties(uthread PROPERTIES LIBRARY_OUTPUT_DIRECTORY build/lib)
endif()

#build uthread_hook
add_library(uthread_hook SHARED src/uthread_hook.c)
if(UTHREAD_HOOK_STDOUT)
    target_compile_definitions(uthread_hook PRIVATE HOOK_STDOUT)
endif()
target_link_libraries(uthread_hook dl)
set_target_properties(uthread_hook PROPERTIES LIBRARY_OUTPUT_DIRECTORY build/lib)

#build test
add_executable(test_yield test/yield.c)
target_link_libraries(test_yield uthread)
set_target_properties(test_yield PROPERTIES RUNTIME_OUTPUT_DIRECTORY build/test)

add_executable(test_hook test/hook.c)
if(UTHREAD_ENABLE_HOOK)
    target_link_libraries(test_hook uthread_hook)
endif()
set_target_properties(test_hook PROPERTIES RUNTIME_OUTPUT_DIRECTORY build/test)

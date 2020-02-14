cmake_minimum_required(VERSION 3.1)
project(uthread)
ENABLE_LANGUAGE(C)

#options
set(UTHREAD_STATIC_LINKING FALSE CACHE BOOL "Link libuthread statically")
set(UTHREAD_OPTIMIZATION TRUE CACHE BOOL "Enable optimization")
set(UTHREAD_DEBUG FALSE CACHE BOOL "Build with debug information")
set(UTHREAD_ENABLE_HOOK TRUE CACHE BOOL "Enable system call hook")

#set C version: C11
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

#set optimization flags
if(UTHREAD_OPTIMIZATION)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
endif()

#debug flags
if(UTHREAD_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pg")
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
if(UTHREAD_DEBUG)
    target_compile_definitions(uthread PRIVATE UTHREAD_DEBUG)
else()
    target_compile_definitions(uthread PRIVATE NDEBUG)
endif()
target_link_libraries(uthread pthread)

#build uthread_hook
add_library(uthread_hook SHARED src/uthread_hook.c)
if(UTHREAD_DEBUG)
    target_compile_definitions(uthread_hook PRIVATE UTHREAD_DEBUG)
else()
    target_compile_definitions(uthread_hook PRIVATE NDEBUG)
endif()
target_link_libraries(uthread_hook dl)
set_target_properties(uthread_hook PROPERTIES LIBRARY_OUTPUT_DIRECTORY build/lib)

#build test
add_executable(test_vector test/vector.c)
if(UTHREAD_DEBUG)
    target_compile_definitions(test_vector PRIVATE UTHREAD_DEBUG)
else()
    target_compile_definitions(test_vector PRIVATE NDEBUG)
endif()
set_target_properties(test_vector PROPERTIES RUNTIME_OUTPUT_DIRECTORY build/test)

add_executable(test_yield test/yield.c)
if(UTHREAD_DEBUG)
    target_compile_definitions(test_yield PRIVATE UTHREAD_DEBUG)
else()
    target_compile_definitions(test_yield PRIVATE NDEBUG)
endif()
target_link_libraries(test_yield uthread)
set_target_properties(test_yield PROPERTIES RUNTIME_OUTPUT_DIRECTORY build/test)

add_executable(test_create test/create.c)
if(UTHREAD_DEBUG)
    target_compile_definitions(test_create PRIVATE UTHREAD_DEBUG)
else()
    target_compile_definitions(test_create PRIVATE NDEBUG)
endif()
target_link_libraries(test_create uthread)
set_target_properties(test_create PROPERTIES RUNTIME_OUTPUT_DIRECTORY build/test)

add_executable(test_tcp test/tcp.c)
if(UTHREAD_ENABLE_HOOK)
    target_link_libraries(test_tcp uthread_hook)
    target_link_libraries(test_tcp uthread)
endif()
set_target_properties(test_tcp PROPERTIES RUNTIME_OUTPUT_DIRECTORY build/test)

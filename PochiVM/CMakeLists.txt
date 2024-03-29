cmake_minimum_required(VERSION 3.10)

# set the project name
#
project(PochiVM)

# specify the C++ compiler
#
SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
else()
  message( FATAL_ERROR "ccache not found!" )
endif()

# specify the C++ standard
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build flavor
# DEBUG: test-debug build, no optimization, all assertions
# RELEASE: test-release build, optimized but with debug symbols, 'assert' disabled but 'TestAssert' enabled
# PRODUCTION: production build, optimized, no debug symbols, no assertions
#
set(BUILD_FLAVOR "" CACHE STRING "Build flavor, supports DEBUG/RELEASE/PRODUCTION")

if(BUILD_FLAVOR STREQUAL "DEBUG")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_FLAVOR=DEBUG -DTESTBUILD -O3 -g ")
  SET(GENERATED_FILES_DIR "${PROJECT_SOURCE_DIR}/__generated__/debug/generated")
  include_directories("${PROJECT_SOURCE_DIR}/__generated__/debug")
elseif(BUILD_FLAVOR STREQUAL "RELEASE")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_FLAVOR=RELEASE -DTESTBUILD -DNDEBUG -O3 -g ")
  SET(GENERATED_FILES_DIR "${PROJECT_SOURCE_DIR}/__generated__/release/generated")
  include_directories("${PROJECT_SOURCE_DIR}/__generated__/release")
elseif(BUILD_FLAVOR STREQUAL "PRODUCTION")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_FLAVOR=PRODUCTION -DNDEBUG -O3 ")
  SET(GENERATED_FILES_DIR "${PROJECT_SOURCE_DIR}/__generated__/production/generated")
  include_directories("${PROJECT_SOURCE_DIR}/__generated__/production")
else()
  message( FATAL_ERROR "Unknown build flavor!" )
endif()

# add -pthread
# 
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pthread ")

# -rdynamic so symbols are exposed to generated code
# 
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -rdynamic ")

# If debug symbols are emitted, redirect them to point at the right location
# The reference to source code files always have prefix ../../ (since we are building 
# in directory build/flavor/), remove this prefix
#
if((BUILD_FLAVOR STREQUAL "DEBUG") OR (BUILD_FLAVOR STREQUAL "RELEASE"))
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffile-prefix-map=../../= ")
endif()

# gtest header
#
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/gtest/include")

# build gtest, we don't need the additional cxxflags to build gtest.
#
add_subdirectory(gtest)

# Append extra cxx warning flags
#
SET(EXTRA_WARNING_FLAGS "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-global-constructors -Wno-exit-time-destructors -Wno-weak-vtables -Wno-padded -Wno-alloca -Wno-missing-prototypes -Wno-zero-length-array -Wno-undefined-reinterpret-cast -Werror ")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${EXTRA_WARNING_FLAGS} ")

# llvm-specific flags
#
SET(LLVM_EXTRA_COMPILE_FLAGS "-I/usr/local/include -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS ")
execute_process(
  COMMAND bash "-c" "llvm-config --ldflags"
  OUTPUT_VARIABLE LLVM_EXTRA_LINK_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# TODO: only append llvm-specific flags when needed
#
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${LLVM_EXTRA_COMPILE_FLAGS} ")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_EXTRA_LINK_FLAGS} ")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN/shared_libs' ")

# Get the llvm-specific link libraries
# There are two parts: 
# (1) the LLVM libraries (which are outputted in absolute paths)
#
execute_process(
  COMMAND bash "-c" "llvm-config --libfiles"
  OUTPUT_VARIABLE _tmp
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE " " ";" LLVM_EXTRA_LINK_LIBRARIES ${_tmp})

# (2) the system libraries. They are outputted in forms like "-lfoo -lbar". 
# We transform the output to format "foo bar", and append to LLVM_EXTRA_LINK_LIBRARIES
#
execute_process(
  COMMAND bash "-c" "llvm-config --system-libs"
  OUTPUT_VARIABLE _tmp
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE " " ";" _tmp ${_tmp})
foreach(_loop_var ${_tmp})
  STRING(REGEX REPLACE "^-l" "" _libname ${_loop_var})
  LIST(APPEND LLVM_EXTRA_LINK_LIBRARIES ${_libname})
endforeach()

include_directories(${PROJECT_SOURCE_DIR})
include_directories("${PROJECT_SOURCE_DIR}/pochivm")

# Build tools that generates the runtime library
#
add_subdirectory(runtime_lib_builder)

# Build runtime library
#
add_subdirectory(runtime)

# Build fastinterp library
#
add_subdirectory(fastinterp)

# Build PochiVM library
#
add_subdirectory(pochivm)

# Build application: mini_db_backend
#
add_subdirectory(mini_db_backend)

# add the executable
#
 add_executable(main
   main.cpp                 
   #test_sanity_arith_expr.cpp      
   #test_sanity_llvm_optimization.cpp
   # test_error_cases.cpp     
   # test_sanity_break_continue.cpp  
   # test_sanity_logical_op.cpp
   # test_fibonacci.cpp       
   test_taco_if.cpp
   # test_sanity_deref.cpp           
   # test_sanity_typecasts.cpp
   # test_find_nth_prime.cpp  
   # test_sanity_irdump.cpp
   # test_sanity_bitcode.cpp
  # test_call_cpp_func.cpp
  # test_catch_throw.cpp
  # test_metavar_framework.cpp
  # test_fast_interp_internals.cpp
    # test_fast_interp.cpp
    # test_paper_microbenchmarks.cpp
   # test_sanity_pointer_arithmetic.cpp
   test_generated_function_pointer.cpp
 )

# Important to link runtime using '--whole-archive'
# Symbols in runtime library, even if unused by main program, 
# may be used at runtime and must not be dropped.
#
target_link_libraries(main PUBLIC 
  "-Wl,--whole-archive" runtime "-Wl,--no-whole-archive" runtime_bc
)

target_link_libraries(main PUBLIC
  gtest
  pochivm
  mini_db_backend
  ${LLVM_EXTRA_LINK_LIBRARIES} 
)

add_subdirectory(post_build_verifier)

set(POST_BUILD_DUMMY_TIMESTAMP_FILE "${PROJECT_BINARY_DIR}/post_build_validate_symbols_fake_target")
add_custom_command(
  OUTPUT ${POST_BUILD_DUMMY_TIMESTAMP_FILE}
  COMMAND ${PROJECT_BINARY_DIR}/post_build_verifier/post_build_verifier ARGS "${PROJECT_BINARY_DIR}/main"
  COMMAND ${CMAKE_COMMAND} -E touch "${POST_BUILD_DUMMY_TIMESTAMP_FILE}"
  DEPENDS main post_build_verifier
)

add_custom_target(post_build_verifer_event ALL
  DEPENDS ${POST_BUILD_DUMMY_TIMESTAMP_FILE}
)


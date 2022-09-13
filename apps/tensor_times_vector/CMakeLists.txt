cmake_minimum_required(VERSION 2.8.12)
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
#project(tensor_times_vector)

SET(CMAKE_C_COMPILER /usr/local/bin/clang)
SET(CMAKE_CXX_COMPILER /usr/local/bin/clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
file(GLOB SOURCE_CODE ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(tensor_times_vector ./tensor_times_vector.cpp)

# To let the app be a standalone project 
if (NOT TACO_INCLUDE_DIR)
  if (NOT DEFINED ENV{TACO_INCLUDE_DIR} OR NOT DEFINED ENV{TACO_LIBRARY_DIR})
    message(FATAL_ERROR "Set the environment variables TACO_INCLUDE_DIR and TACO_LIBRARY_DIR")
  endif ()
  set(TACO_INCLUDE_DIR $ENV{TACO_INCLUDE_DIR})
  set(TACO_LIBRARY_DIR $ENV{TACO_LIBRARY_DIR})
  find_library(taco taco ${TACO_LIBRARY_DIR})
  target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${taco})
else()
  set_target_properties("tensor_times_vector" PROPERTIES OUTPUT_NAME "taco-tensor_times_vector")
  target_link_libraries(tensor_times_vector LINK_PUBLIC taco)
endif ()

# Include taco headers
include_directories(${TACO_INCLUDE_DIR})

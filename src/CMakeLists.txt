SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN/shared_libs' -L${PROJECT_SOURCE_DIR}/pochi_libs_and_includes/lib")
if (TACO_SHARED_LIBRARY)
  set(TACO_LIBRARY_TYPE SHARED)
  message("-- Shared library")
else()
  set(TACO_LIBRARY_TYPE STATIC)
  message("-- Static library")
endif()

set(TACO_SRC_DIRS . parser index_notation lower ir codegen storage error util)

foreach(dir ${TACO_SRC_DIRS})
  file(GLOB TACO_HEADERS ${TACO_HEADERS} ${dir}/*.h)
  file(GLOB TACO_HEADERS ${TACO_HEADERS} ${TACO_INCLUDE_DIR}/taco/${dir}/*.h)
  file(GLOB TACO_SOURCES ${TACO_SOURCES} ${dir}/*.cpp)
endforeach()

set(TACO_HEADERS ${TACO_HEADERS} ../include/taco/ir_tags.h)
set(TACO_SOURCES ${TACO_SOURCES} ir_tags.cpp)

add_definitions(${TACO_DEFINITIONS})
include_directories(${TACO_SRC_DIR})
add_library(taco ${TACO_LIBRARY_TYPE} ${TACO_HEADERS} ${TACO_SOURCES})
if (CUDA)
  include_directories(${CUDA_INCLUDE_DIRS})
  target_link_libraries(taco PUBLIC ${CUDA_LIBRARIES})
endif (CUDA)
install(TARGETS taco DESTINATION lib)

if (LINUX)
  target_link_libraries(taco PRIVATE ${TACO_LIBRARIES} dl)
else()
  target_link_libraries(taco PRIVATE ${TACO_LIBRARIES})
endif()

target_link_libraries( taco PUBLIC
  "-Wl,--whole-archive" runtime "-Wl,--no-whole-archive" runtime_bc
)
target_link_libraries( taco PUBLIC
  pochivm
  ${LLVM_EXTRA_LINK_LIBRARIES} 
)
target_link_libraries( taco PUBLIC
  "-Wl,--whole-archive" runtime "-Wl,--no-whole-archive" runtime_bc
)

# add_executable(exec test.cpp)
# target_link_libraries(exec taco dl)

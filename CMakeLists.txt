cmake_minimum_required(VERSION 3.5)
project(SpatialQueryBenchmark LANGUAGES C CXX)

option(USE_GPU "Find CUDA if enabled" ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# SIMD
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /arch:AVX2 /W1 /EHsc")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -xHost")
else ()
    # clang and gcc
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra")
endif ()


include("cmake/FindGFlags.cmake")
if (NOT GFLAGS_FOUND)
    message(STATUS "gflags not found, build without gflags")
else ()
    include_directories(SYSTEM ${GFLAGS_INCLUDE_DIRS})
endif ()

find_package(CGAL REQUIRED)

if (USE_GPU)
    enable_language(CUDA)
    add_compile_definitions(-DUSE_GPU)
    find_package(rtspatial REQUIRED)

    message("RTSPATIAL_INCLUDE_DIR: ${RTSPATIAL_INCLUDE_DIR}")
    message("RTSPATIAL_LIBRARIES: ${RTSPATIAL_LIBRARIES}")
    message("RTSPATIAL_SHADER_DIR: ${RTSPATIAL_SHADER_DIR}")
    message("CUDAToolkit_INCLUDE_DIRS: ${CUDAToolkit_INCLUDE_DIRS}")

    RTSPATIAL_COMPILE_SHADERS("${RTSPATIAL_INCLUDE_DIR}/rtspatial/default_handlers" "collecting_handlers.h"
            "${PROJECT_BINARY_DIR}/collecting_ptx" PROGRAM_MODULES_COLLECTING)

    RTSPATIAL_COMPILE_SHADERS("${RTSPATIAL_INCLUDE_DIR}/rtspatial/default_handlers" "counting_handlers.h"
            "${PROJECT_BINARY_DIR}/counting_ptx" PROGRAM_MODULES_COUNTING)
    # LBVH
    include_directories(thirdparty/lbvh)
    include_directories(${RTSPATIAL_INCLUDE_DIR})
    link_directories(${RTSPATIAL_LIBRARY_DIR})

    set(ENABLED_ARCHS "80")
endif ()
# GLIN
add_subdirectory(thirdparty/GLIN)
include_directories(thirdparty/GLIN)
include_directories(thirdparty/GLIN/src/core)
include_directories(${GEOS_INCLUDE_DIR})


find_package(Boost 1.76 REQUIRED COMPONENTS serialization)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARIES: ${Boost_LIBRARIES}")
message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

include_directories(src)
add_executable(gen src/gen/gen.cpp src/flags.cpp)
target_link_libraries(gen ${GFLAGS_LIBRARIES} ${Boost_LIBRARIES})

add_library(glin thirdparty/GLIN/glin/hilbert/hilbert.cpp)

set(GPU_SOURCES "")
list(APPEND GPU_SOURCES src/query/rtspatial/range_query.cu
        src/query/rtspatial/point_query.cu
        src/query/lbvh/range_query.cu
        src/query/lbvh/point_query.cu)

add_executable(query_collecting src/query/query.cpp
        ${GPU_SOURCES}
        src/flags.cpp
        ${PROGRAM_MODULES_COLLECTING})
target_compile_definitions(query_collecting PRIVATE PIECE) # GLIN requires for intersects query
target_link_libraries(query_collecting pthread glin ${GFLAGS_LIBRARIES} ${RTSPATIAL_LIBRARIES} ${GEOS_LIBRARY} ${Boost_LIBRARIES})
target_compile_definitions(query_collecting PRIVATE RTSPATIAL_PTX_DIR=\"${PROJECT_BINARY_DIR}/collecting_ptx\" -DCOLLECT_RESULTS)
if (USE_GPU)
    target_compile_options(query_collecting PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-extended-lambda --expt-relaxed-constexpr --use_fast_math>)
    set_target_properties(query_collecting PROPERTIES CUDA_ARCHITECTURES "${ENABLED_ARCHS}")
endif ()

add_executable(query_counting src/query/query.cpp
        ${GPU_SOURCES}
        src/flags.cpp
        ${PROGRAM_MODULES_COUNTING})
target_compile_definitions(query_counting PRIVATE PIECE) # GLIN requires for intersects query
target_link_libraries(query_counting pthread glin ${GFLAGS_LIBRARIES} ${RTSPATIAL_LIBRARIES} ${GEOS_LIBRARY} ${Boost_LIBRARIES})
target_compile_definitions(query_counting PRIVATE RTSPATIAL_PTX_DIR=\"${PROJECT_BINARY_DIR}/counting_ptx\" -DCOUNT_RESULTS)
if (USE_GPU)
    target_compile_options(query_counting PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-extended-lambda --expt-relaxed-constexpr --use_fast_math>)
    set_target_properties(query_counting PROPERTIES CUDA_ARCHITECTURES "${ENABLED_ARCHS}")
endif ()

cmake_minimum_required(VERSION 3.5)
project(SpatialQueryBenchmark LANGUAGES CUDA C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



include("cmake/FindGFlags.cmake")
if (NOT GFLAGS_FOUND)
    message(STATUS "gflags not found, build without gflags")
else ()
    include_directories(SYSTEM ${GFLAGS_INCLUDE_DIRS})
endif ()
find_package(Boost 1.76 REQUIRED COMPONENTS serialization)
find_package(rtspatial REQUIRED)

message("RTSPATIAL_INCLUDE_DIR: ${RTSPATIAL_INCLUDE_DIR}")
message("RTSPATIAL_LIBRARIES: ${RTSPATIAL_LIBRARIES}")
message("RTPATIAL_PTX: ${RTSPATIAL_PTX_DIR}")
message("CUDAToolkit_INCLUDE_DIRS: ${CUDAToolkit_INCLUDE_DIRS}")


# GLIN
add_subdirectory(thirdparty/GLIN)
include_directories(thirdparty/GLIN)
include_directories(thirdparty/GLIN/src/core)
include_directories(${GEOS_INCLUDE_DIR})
# SIMD
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /arch:AVX2 /W1 /EHsc")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -xHost")
else ()
    # clang and gcc
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra")
endif ()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${RTSPATIAL_INCLUDE_DIR})
link_directories(${RTSPATIAL_LIBRARY_DIR})

include_directories(src)
add_executable(gen src/gen/gen.cpp src/flags.cpp)
target_link_libraries(gen ${GFLAGS_LIBRARIES})

add_library(glin thirdparty/GLIN/glin/hilbert/hilbert.cpp)

set(ENABLED_ARCHS "80")

add_executable(query src/query/query.cpp
        src/query/rtspatial/range_query.cu
        src/query/rtspatial/point_query.cu
        src/flags.cpp)
target_compile_definitions(query PRIVATE PIECE) # GLIN requires for intersects query
target_link_libraries(query pthread glin ${GFLAGS_LIBRARIES} ${RTSPATIAL_LIBRARIES} ${GEOS_LIBRARY} ${Boost_LIBRARIES})
target_compile_definitions(query PRIVATE RTSPATIAL_LIBRARY_DIR=\"${RTSPATIAL_LIBRARY_DIR}\")
target_compile_options(query PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-extended-lambda --expt-relaxed-constexpr --use_fast_math>)
set_target_properties(query PROPERTIES CUDA_ARCHITECTURES "${ENABLED_ARCHS}")

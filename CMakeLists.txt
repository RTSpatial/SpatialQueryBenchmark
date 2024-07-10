cmake_minimum_required(VERSION 3.5)
project(SpatialQueryBenchmark LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("cmake/FindGFlags.cmake")
if (NOT GFLAGS_FOUND)
    message(STATUS "gflags not found, build without gflags")
else ()
    include_directories(SYSTEM ${GFLAGS_INCLUDE_DIRS})
endif ()

find_package(Boost REQUIRED)

include_directories(src)

add_executable(gen src/gen/gen.cpp src/flags.cpp)
target_link_libraries(gen ${GFLAGS_LIBRARIES})

add_executable(query src/query/query.cpp src/flags.cpp)
target_link_libraries(query ${GFLAGS_LIBRARIES})


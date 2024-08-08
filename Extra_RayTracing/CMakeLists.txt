cmake_minimum_required(VERSION 3.0)
project(PathRayTracing)

if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
endif

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

file(GLOB SOURCE_FILE CONFIGURE_DEPENDS
        *.cpp
        Common/*.cpp
        External/* )

add_executable(RayTracing ${SOURCE_FILE})

include_directories(${PROJECT_SOURCE_DIR})

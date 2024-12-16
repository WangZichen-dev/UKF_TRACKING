# cmake_minimum_required(VERSION 3 FATAL_ERROR)
cmake_minimum_required(VERSION 3.10)
set(CXX_FLAGS "-Wall -std=c++14 -o3")
set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")

project(playback)

find_package(PCL 1.2 REQUIRED)

set(CMAKE_BUILD_TYPE debug)
add_compile_options(-fPIC)
add_compile_options(-O0)

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})
list(REMOVE_ITEM PCL_LIBRARIES "vtkproj4")


add_executable (ukf_highway src/main.cpp src/ukf.cpp src/tools.cpp src/render/render.cpp)
target_link_libraries (ukf_highway ${PCL_LIBRARIES})





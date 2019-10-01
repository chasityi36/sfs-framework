cmake_minimum_required(VERSION 2.8.0)
project( sfs-framework  CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ")

find_package( OpenCV 4 REQUIRED )
find_package( Eigen3 REQUIRED )
find_package( yaml-cpp REQUIRED )

set(ROOT ./)
include_directories(${ROOT}/src)
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${YAML_CPP_INCLUDE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB sfs-framework_SRC
    "src/*.?pp"
	#src/framework-viewpointsource.?pp
    #src/framework-viewpoint.?pp
    #src/framework-sparsefeature.?pp
    #src/framework-database.?pp
    #src/framework.?pp
)

add_executable( sfs-framework ${sfs-framework_SRC} )
target_link_libraries( sfs-framework yaml-cpp ${OpenCV_LIBS} ${YAML_CPP_LIBRARIES} stdc++fs) 

message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")

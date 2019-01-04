#最低版本
cmake_minimum_required( VERSION 2.8 )

#项目名
set(pro_name "XSvr")

#project(${pro_name})

set(third_path ${PROJECT_SOURCE_DIR}/src/third)
set(protocol_path ${PROJECT_SOURCE_DIR}/src/protocol)
set(common_path ${PROJECT_SOURCE_DIR}/src/common)
include(${third_path}/CMakeLists.txt)
include(${protocol_path}/CMakeLists.txt)
include(${common_path}/CMakeLists.txt)

IF(NOT CMAKE_BUILD_TYPE)#（可选）如果没有指定cmake编译模式，就选择Release模式,必须写成三行
SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

set( CMAKE_CXX_FLAGS "-std=c++11" )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)#将产生的target文件输出到指定路径



include_directories(
    ${PROJECT_SOURCE_DIR}
    ${THIRD_INCLUDE}
    ${COMMON_INCLUDE}
    ${PROJECT_SOURCE_DIR}/src
)#添加头文件目录

#主目录
file(GLOB pro_cpp src/*.cpp )
file(GLOB pro_h src/*.h )

#包含文件
set(pro_src  
    ${pro_h} 
    ${pro_cpp} 
    ${THIRD_SRC} 
    ${THIRD_HEADER} 
    ${PROTOCOL_H}
    ${PROTOCOL_CC}
    ${COMMON_H}
    ${COMMON_SRC}
    ${COMMON_HPP}
)
#包含lib
set(pro_lib
    ${THIRD_LIB} 
)

add_executable(${pro_name} ${pro_src})#生成可执行文件

target_link_libraries(${pro_name} ${pro_lib}) 

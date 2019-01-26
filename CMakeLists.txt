cmake_minimum_required( VERSION 2.8 )

set(PRO_NAME "XSvr")

#add include path
set(SRC_PATH ${PROJECT_SOURCE_DIR}/src)
set(THIRD_PATH ${SRC_PATH}/third)
set(INCLUDE_PATH
    ${SRC_PATH}
    ${SRC_PATH}/common
    ${SRC_PATH}/common/base
    ${SRC_PATH}/common/net
    ${SRC_PATH}/common/third_help
    ${SRC_PATH}/common/utility
    #add third
    ${THIRD_PATH}
    ${THIRD_PATH}/fmt/include 
    ${THIRD_PATH}/libevent/include
    ${THIRD_PATH}/mysql/include
    ${THIRD_PATH}/protobuf/google
    ${THIRD_PATH}/protobuf
    ${THIRD_PATH}/spdlog/include
    ${THIRD_PATH}/hiredis
)
include_directories(${INCLUDE_PATH})
#add file
file(GLOB_RECURSE PRO_CPP src/*.cpp )
file(GLOB_RECURSE PRO_H src/*.h )
file(GLOB_RECURSE PRO_HPP src/*.hpp )
file(GLOB_RECURSE PRO_CC src/*.cc )
set(PRO_SRC ${PRO_H} ${PRO_CPP} ${PRO_HPP} ${PRO_CC})

#add lib
link_directories(/usr/local/lib)
link_directories(/usr/local/mysql/lib)
set(PRO_LIB
   -lstdc++
   libevent.a
   libevent_pthreads.a
   libprotobuf.a
   libmysqlclient.a
   -lrt 
   -lpthread 
   -ldl
   -lcrypto
   -lssl
   ${THIRD_PATH}/hiredis/lib/libhiredis.a
)



IF(NOT CMAKE_BUILD_TYPE)#（可选）如果没有指定cmake编译模式，就选择Release模式,必须写成三行
SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

set( CMAKE_CXX_FLAGS "-std=c++11" )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)#将产生的target文件输出到指定路径


add_executable(${PRO_NAME} ${PRO_SRC})#生成可执行文件

target_link_libraries(${PRO_NAME} ${PRO_LIB}) 

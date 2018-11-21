set(spdlog_path ${PROJECT_SOURCE_DIR}/src/third/spdlog)
file(GLOB_RECURSE SPDLOG_H ${spdlog_path}/*.h)
set(SPDLOG_INCLUDE 
    ${spdlog_path}
    ${spdlog_path}/include
    ${spdlog_path}/include/spdlog
)
set(SPDLOG_NEED_LIB -lpthread)
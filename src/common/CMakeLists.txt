set(common_path ${PROJECT_SOURCE_DIR}/src/common)
file(GLOB_RECURSE COMMON_H ${common_path}/*.h)
file(GLOB_RECURSE COMMON_SRC ${common_path}/*.cpp)
file(GLOB_RECURSE COMMON_HPP ${common_path}/*.hpp)
set(COMMON_INCLUDE 
    ${common_path}
)

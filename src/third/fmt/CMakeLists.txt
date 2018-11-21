set(fmt_path ${PROJECT_SOURCE_DIR}/src/third/fmt)
file(GLOB_RECURSE THIRD_FMT_H ${fmt_path}/*.h)
file(GLOB_RECURSE THIRD_FMT_CPP ${fmt_path}/*.cpp)
set(THIRD_FMT_INCLUDE ${fmt_path} ${fmt_path}/include)
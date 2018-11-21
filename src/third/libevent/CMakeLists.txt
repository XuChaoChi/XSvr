set(libevent_path ${PROJECT_SOURCE_DIR}/src/third/libevent)
set(LIBEVENT_LIB ${libevent_path}/lib/libevent.a)
file(GLOB_RECURSE LIBEVENT_H ${libevent_path}/*.h)
set(LIBEVENT_INCLUDE ${libevent_path}/include ${libevent_path})
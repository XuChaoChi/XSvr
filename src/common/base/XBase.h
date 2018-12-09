#pragma once

#define XSVR_NS_BEGIN namespace XSvr{
#define XSVR_NS_END }
#define USE_XSVR_NS using namespace XSvr

#define XSVR_1M 1048576
#define XSVR_1G 1073741824
#define XSVR_1KB 1024
#define XSVR_1b 1

#define XSVR_BIND(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_)
#define XSVR_BIND_1(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_, std::placeholders::_1)
#define XSVR_BIND_2(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_, std::placeholders::_1, std::placeholders::_2)
#define XSVR_BIND_3(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_, std::placeholders::_1, std::placeholders::_2, , std::placeholders::_3)

#define XSVR_INIT(_FUNC_, _INIT_TYPE_)\
    if(!_FUNC_())\
        throw _INIT_TYPE_

#define XSVR_UNUSED(_ARG_)\
    (void)_ARG_
enum XSvrInitType{
    eInitType_listen,
    eInitType_thread,
    eInitType_sql,
    eInitType_redis,
    eInitType_cnt,
};
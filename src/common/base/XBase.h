#pragma once

#define XSVR_NS_BEGIN namespace XSvr{
#define XSVR_NS_END }
#define USE_XSVR_NS XSvr

#define XSVR_BIND(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_)
#define XSVR_BIND_1(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_, std::placeholders::_1)
#define XSVR_BIND_2(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_, std::placeholders::_1, std::placeholders::_2)
#define XSVR_BIND_3(_FUNC_, _OBJ_) std::bind(_FUNC_, _OBJ_, std::placeholders::_1, std::placeholders::_2, , std::placeholders::_3)
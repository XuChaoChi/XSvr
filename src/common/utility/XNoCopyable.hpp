#pragma once
#include "base/XBase.h"

XSVR_NS_BEGIN

#define XSVR_NOCOPYABLE(_CLASS_)\
	_CLASS_ operator = (const _CLASS_&) = delete;\
	_CLASS_(const _CLASS_&) = delete;\
	_CLASS_(const _CLASS_&&) = delete;\
	_CLASS_ &operator == (const _CLASS_ &&) = delete;
	
class NoCopyable {
public:
	NoCopyable() {
	}
	~NoCopyable() {
	}
	NoCopyable operator = (const NoCopyable&) = delete;

	NoCopyable(const NoCopyable&) = delete;

	NoCopyable(const NoCopyable&&) = delete;

	NoCopyable &operator == (const NoCopyable &&) = delete;
};
XSVR_NS_END
#pragma once
#include "base/XBase.h"
#include "utility/XNoCopyable.hpp"
XSVR_NS_BEGIN
template <typename T>
class Singleton
{
  XSVR_NOCOPYABLE(Singleton)
  public:
    static T *instance()
    {
        static T instance;
        return &instance;
    }

  private:
    Singleton() {}
    ~Singleton() {}
};
XSVR_NS_END
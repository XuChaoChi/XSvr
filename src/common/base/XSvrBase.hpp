#pragma once
#include "base/XBase.h"
#include "utility/XNoCopyable.hpp"
#include "utility/XSingleton.hpp"
#include <condition_variable>
#include <mutex>
#include <atomic>
XSVR_NS_BEGIN
class XSvrBase
{
  public:
    XSvrBase() : m_bRunning(false)
    {
    }
    virtual ~XSvrBase()
    {
    }
    void start()
    {
        if (!m_bRunning.load() && _onSvrInit())
        {
            m_bRunning.store(true);
        }
    }
    void stop()
    {
        if(m_bRunning.load() && _onSvrStop()){
            m_bRunning.store(false);
        }
    }

  protected:
    virtual bool _onSvrInit() = 0;
    virtual bool _onSvrStop() = 0;

    std::atomic_bool m_bRunning;
};
XSVR_NS_END

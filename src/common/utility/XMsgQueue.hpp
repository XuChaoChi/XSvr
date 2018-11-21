#pragma once
#include "base/XBase.h"
#include "utility/XNoCopyable.hpp"
#include <condition_variable>
#include <vector>

XSVR_NS_BEGIN
template <typename TMsg>
class XMsgQueue
{
    XSVR_NOCOPYABLE(XMsgQueue)
  public:   
    bool isFull() const
    {
        return true;
    }
    bool isEmpty() const
    {
        return true;
    }
    bool push(const TMsg &msg)
    {
        return true;
    }
    bool pop(TMsg &msg)
    {
        return true;
    }
    size_t size() const
    {
        return 0;
    }
    private:
    std::vector<TMsg> m_queue;
    std::condition_variable m_emptyCv;
    std::condition_variable m_fullCv;
    std::mutex m_mutex;
};

XSVR_NS_END
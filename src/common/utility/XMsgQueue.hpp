#pragma once
#include "base/XBase.h"
#include "utility/XNoCopyable.hpp"
#include <condition_variable>
#include <vector>
#include <mutex>
#include <atomic>

XSVR_NS_BEGIN
template <typename TMsg>
class XMsgQueue
{
    XSVR_NOCOPYABLE(XMsgQueue)
  public:
    XMsgQueue(uint32_t nSize)
        : m_nFrontIndex(0),
          m_nTailIndex(0)
    {
        m_queue.resize(nSize);
    }
    bool isFull() const
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        return _isFull();
    }
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        return _isEmpty();
    }
    bool push(const TMsg &msg)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        if (_isFull())
        {
            return false;
        }
        else
        {
            m_queue[m_nTailIndex] = msg;
            m_nTailIndex = ++m_nTailIndex % m_queue.capacity();
            m_emptyCv.notify_one();
        }
        return true;
    }
    bool get(TMsg &msg)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        while (_isEmpty())
        {
            m_emptyCv.wait(lck);
        }
        msg = m_queue[m_nFrontIndex];
        m_nFrontIndex = ++m_nTailIndex % m_queue.capacity();
        m_emptyCv.notify_one();
        return true;
    }
    uint32_t size()
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        return (m_nTailIndex - m_nFrontIndex + m_queue.capacity()) % m_queue.capacity();
    }

  private:
    bool _isFull() const
    {
        uint32_t nTempFront = m_nFrontIndex % m_queue.capacity();
        uint32_t nTempTail = (m_nTailIndex + 1) % m_queue.capacity();
        if (nTempFront == nTempTail)
        {
            return true;
        }
        return false;
    }
    bool _isEmpty() const
    {
        return m_nFrontIndex == m_nTailIndex;
    }

    std::vector<TMsg> m_queue;
    std::condition_variable m_emptyCv;
    std::mutex m_mutex;
    uint32_t m_nFrontIndex;
    uint32_t m_nTailIndex;
};

XSVR_NS_END
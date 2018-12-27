//libevent客户端封装
//考虑到是服务端间的通行开了线程，多客户端性能不是很好。
//如果需要多客户端可以把event_base_dispatch和event_base提取出来
#pragma once
extern "C"
{
#include "event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/thread.h"
#include "event2/buffer.h"
#include "event2/util.h"
}
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <memory>
#include "base/XBase.h"
#define SOCKET_HEADER_LENGTH sizeof(uint32_t)
XSVR_NS_BEGIN
class XEventServer;
class XEventClient
{
  friend XEventServer;

public:
  XEventClient()
      : m_bConnected(false) {}
  ~XEventClient()
  {
    if (m_bConnected.load())
    {
      close();
    }
  }
  bool connnectToHost(const std::string &strIP, uint16_t nPort)
  {
    bool bRet = false;
    do
    {
      if (m_bConnected.load())
      {
        onError("SOCKET ALREADY CONNECTED");
        break;
      }
      m_pEvBase = std::shared_ptr<event_base>(event_base_new(), [](event_base *pEvBase) {
        if (pEvBase)
        {
          event_base_free(pEvBase);
        }
      });
      if (!m_pEvBase)
      {
        onError("event_base_new");
        break;
      }

      m_stAddr.sin_family = AF_INET;
      m_stAddr.sin_port = htons(nPort);
      m_stAddr.sin_addr.s_addr = strIP.empty() ? 0 : inet_addr(strIP.c_str());
      m_pBufferEv = std::shared_ptr<bufferevent>(bufferevent_socket_new(m_pEvBase.get(), -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE),
                                                 [](bufferevent *pEvBuf) {
                                                   if (pEvBuf)
                                                   {
                                                     bufferevent_free(pEvBuf);
                                                   }
                                                 });
      if (!m_pBufferEv)
      {
        onError("bufferevent_socket_new");
        break;
      }
      if (0 != bufferevent_socket_connect(m_pBufferEv.get(), (struct sockaddr *)&m_stAddr, sizeof(m_stAddr)))
      {
        onError("bufferevent_socket_connect");
        break;
      }
      bufferevent_setcb(m_pBufferEv.get(), readCallback, NULL, eventCallback, (void *)this);
      if (0 != bufferevent_enable(m_pBufferEv.get(), EV_READ | EV_PERSIST))
      {
        onError("bufferevent_enable");
        break;
      }
      if (0 != evutil_make_socket_nonblocking(bufferevent_getfd(m_pBufferEv.get())))
      {
        onError("evutil_make_socket_nonblocking");
        break;
      }
      m_pThread = std::make_shared<std::thread>([this]() {
        if (0 != event_base_dispatch(m_pEvBase.get()))
        {
          onError("event_base_dispatch");
        }
        int a = 10;
      });
      bRet = true;
    } while (0);
    return bRet;
  }

  bool close()
  {
    if (!m_bConnected.load())
    {
      onError("ID(" + std::to_string(id()) + "):NOT CONNECTED, CANT CLOSE");
      return false;
    }
    else
    {
      if (m_pEvBase)
      {
        m_pBufferEv = nullptr;
        if (0 != event_base_loopbreak(m_pEvBase.get()))
        {
          onError("event_base_loopbreak");
          return false;
        }
        m_pEvBase = nullptr;
      }
      if (m_pThread)
      {
        m_pThread->join();
      }
      m_bConnected.store(false);
    }

    return true;
  }

  int32_t send(const std::string &strData)
  {
    if (!m_bConnected.load())
    {
      onError("SOCKET NOT CONNECTED");
      return -1;
    }
    if (0 != bufferevent_write(m_pBufferEv.get(), strData.c_str(), strData.size()))
    {
      onError("bufferevent_write");
      return -1;
    }
    return strData.size();
  }

  std::string ip() const
  {
    return inet_ntoa(m_stAddr.sin_addr);
  }

  uint16_t port() const
  {
    return htons(m_stAddr.sin_port);
  }

  uint32_t id() const
  {
    return m_nID;
  }

private:
  static void readCallback(bufferevent *pBEv, void *pArg)
  {
    auto pClient = static_cast<XEventClient *>(pArg);
    if (pClient)
    {
      auto pInputBuf = pBEv->input;
      const char *pHeader = nullptr;
      uint32_t nPacketLen = 0;
      pHeader = (const char *)evbuffer_pullup(pInputBuf, -1);
      uint32_t nCurLen = evbuffer_get_length(pInputBuf);
      pClient->onLog("cursize:" + std::to_string(nCurLen));
      if (pHeader && nCurLen >= SOCKET_HEADER_LENGTH)
      {
        memcpy(&nPacketLen, pHeader, SOCKET_HEADER_LENGTH);
        pClient->onLog("packetsize:" + std::to_string(nPacketLen));
        if (nPacketLen < 0)
        {
          pClient->onError("packetsize < 0");
          pClient->close();
        }
        if (nCurLen >= nPacketLen)
        {
          if (pClient->m_pReadFunc)
          {
            pClient->m_pReadFunc(pHeader + SOCKET_HEADER_LENGTH, nPacketLen - SOCKET_HEADER_LENGTH);
            if (0 != evbuffer_drain(pInputBuf, nPacketLen))
            {
              pClient->onError("evbuffer_drain");
              pClient->close();
            }
          }
        }
      }
    }
  }

  static void eventCallback(bufferevent *pBufferEv, short nEvent, void *pArg)
  {
    auto pClient = static_cast<XEventClient *>(pArg);
    if (nEvent & BEV_EVENT_EOF)
    {
      pClient->onError("BEV_EVENT_EOF");
    }
    else if (nEvent & BEV_EVENT_ERROR)
    {
      auto err = EVUTIL_SOCKET_ERROR();
      pClient->onError(evutil_socket_error_to_string(err));
    }
    else if ((nEvent & BEV_EVENT_TIMEOUT))
    {
      pClient->onLog("BEV_EVENT_TIMEOUT");
    }
    else if ((nEvent & BEV_EVENT_CONNECTED))
    {
      pClient->onConnected();
      pClient->onLog("BEV_EVENT_CONNECTED");
    }
  }

  void onLog(const std::string &strMsg)
  {
    if (m_pLogFunc)
    {
      m_pLogFunc(std::move(strMsg));
    }
  }
  void onError(const std::string &strMsg)
  {
    if (m_pErrorFunc)
    {
      m_pErrorFunc(std::move(strMsg));
    }
  }
  void onConnected()
  {
    m_bConnected.store(true);
    if (m_pConnectedFunc)
    {
      m_pConnectedFunc();
    }
  }
  std::function<void(const char *pBuf, uint32_t nLen)> m_pReadFunc = nullptr;
  std::function<void(const std::string &)> m_pErrorFunc = nullptr;
  std::function<void(const std::string &)> m_pLogFunc = nullptr;
  std::function<void()> m_pConnectedFunc = nullptr;

  std::shared_ptr<bufferevent> m_pBufferEv = nullptr;
  std::shared_ptr<event_base> m_pEvBase = nullptr;
  std::atomic_bool m_bConnected;
  uint32_t m_nID = 0;
  sockaddr_in m_stAddr;
  std::shared_ptr<std::thread> m_pThread = nullptr;
};
XSVR_NS_END
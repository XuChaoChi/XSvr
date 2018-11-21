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
#include <memory>
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <memory.h>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <arpa/inet.h>
#include "base/XBase.h"
#include "utility/XNoCopyable.hpp"
#include "third_help/XEventClient.hpp"
XSVR_NS_BEGIN
#define SOCKET_HEADER_LENGTH sizeof(uint32_t)
using XEvClientPtr = std::shared_ptr<XEventClient>;
using BufferEvPtr = std::shared_ptr<bufferevent>;
class XEventServer
{
    XSVR_NOCOPYABLE(XEventServer)
  public:
    XEventServer()
    {
        m_nClientID.store(0);
    }
    ~XEventServer()
    {
        onLog("SVR EXIT.....");
    }
    bool listen(std::string strIp, uint16_t nPort)
    {
        sockaddr_in stAddr;
        memset(&stAddr, 0, sizeof(sockaddr_in));
        stAddr.sin_family = AF_INET;
        stAddr.sin_port = htons(nPort);
        stAddr.sin_addr.s_addr = strIp.empty() ? 0 : inet_addr(strIp.c_str());

        m_pEventBase = std::shared_ptr<event_base>(::event_base_new(), [](event_base *pBase) {
            event_base_free(pBase);
        });

        auto pListener = std::shared_ptr<evconnlistener>(evconnlistener_new_bind(
                                                             m_pEventBase.get(),
                                                             &listenCallback,
                                                             this,
                                                             LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE,
                                                             -1,
                                                             (sockaddr *)&stAddr,
                                                             sizeof(sockaddr_in)),
                                                         [](evconnlistener *pListener) {
                                                             if (pListener)
                                                             {
                                                                 evconnlistener_free(pListener);
                                                             }
                                                         });
        event_base_dispatch(m_pEventBase.get());
        // m_pThread = std::make_shared<std::thread>([this]() {
        //     int nCode  = event_base_dispatch(m_pEventBase.get());
        //     onLog("loop end:" + std::to_string(nCode));
        // });

        return true;
    }
    bool stop() {return true;}
    using readFuncPtr = std::function<void(XEvClientPtr, const char *, uint32_t)>;
    using msgFuncPtr = std::function<void(std::string)>;
    void setCallback(readFuncPtr readCbFunc,
                     msgFuncPtr errCbFunc,
                     msgFuncPtr logCbFunc)
    {
        m_readCbFunc = readCbFunc;
        m_errCbFunc = errCbFunc;
        m_logCbFunc = logCbFunc;
    }

    bool initEnvironment()
    {
#ifdef _WIN32
        evthread_use_windows_threads();
#else
#ifdef EVENT__HAVE_PTHREADS
        evthread_use_pthreads();
#else
        onError("pthread not define");
#endif
#endif
return true;
    }

  private:
    void onError(const std::string &strMsg)
    {
        if (m_errCbFunc)
        {
            m_errCbFunc(strMsg);
        }
    }
    void onLog(const std::string &strLog)
    {
        if (m_logCbFunc)
        {
            m_logCbFunc(strLog);
        }
    }
    static void listenCallback(evconnlistener *pListener, evutil_socket_t nFd, struct sockaddr *pSock, int socklen, void *pArg)
    {
        auto pSvr = static_cast<XEventServer *>(pArg);
        pSvr->onLog("new connected");
        std::shared_ptr<XEventClient> pClient = std::make_shared<XEventClient>();
        pClient->m_nID = pSvr->m_nClientID++;
        evutil_make_socket_nonblocking(nFd);
        pClient->m_pBufferEv = std::shared_ptr<bufferevent>(bufferevent_socket_new(pSvr->m_pEventBase.get(), nFd, BEV_OPT_CLOSE_ON_FREE));
        bufferevent_setcb(pClient->m_pBufferEv.get(), readCallback, NULL, eventCallback, pArg);
        bufferevent_enable(pClient->m_pBufferEv.get(), EV_READ | EV_PERSIST);
        pSvr->addClient(pClient->m_pBufferEv.get(), pClient);
    }

    static void readCallback(bufferevent *pBEv, void *pArg)
    {
        auto pSvr = static_cast<XEventServer *>(pArg);
        auto pInputBuf = pBEv->input;
        const char *pHeader = nullptr;
        uint32_t nPacketLen = 0;
        pHeader = (const char *)evbuffer_pullup(pInputBuf, -1);
        uint32_t nCurLen = evbuffer_get_length(pInputBuf);
        pSvr->onLog("cursize:" + std::to_string(nCurLen));
        if (pHeader && nCurLen >= SOCKET_HEADER_LENGTH)
        {
            memcpy(&nPacketLen, pHeader, SOCKET_HEADER_LENGTH);
            pSvr->onLog("packetsize:" + std::to_string(nPacketLen));
            if (nPacketLen < 0)
            {
                //todo error
            }
            if (nCurLen >= nPacketLen)
            {
                if (pSvr->m_readCbFunc)
                {
                    auto pClient = pSvr->getClient(pBEv);
                    pSvr->m_readCbFunc(pClient, pHeader + SOCKET_HEADER_LENGTH, nPacketLen - SOCKET_HEADER_LENGTH);
                    if (-1 == evbuffer_drain(pInputBuf, nPacketLen))
                    {
                        //todo error
                    }
                }
            }
        }
    }

    static void eventCallback(bufferevent *pBufferEv, short nEvent, void *pArg)
    {
        auto pSvr = static_cast<XEventServer *>(pArg);
        if (nEvent & BEV_EVENT_EOF)
        {
            pSvr->onLog("BEV_EVENT_EOF");
        }
        else if (nEvent & BEV_EVENT_ERROR)
        {
            auto err = EVUTIL_SOCKET_ERROR();
            pSvr->onError(evutil_socket_error_to_string(err));
            pSvr->delClient(pBufferEv);
        }
        else if ((nEvent & BEV_EVENT_TIMEOUT))
        {
            pSvr->onLog("BEV_EVENT_TIMEOUT");
        }
    }
    void addClient(bufferevent *pBEv, XEvClientPtr pClient)
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        m_mapEvClient[pBEv] = pClient;
    }
    uint32_t getClientCount()
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        return m_mapEvClient.size();
    }
    bool delClient(bufferevent *pBEv)
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        bool bRet = false;
        auto mIter = m_mapEvClient.find(pBEv);
        if (mIter != m_mapEvClient.end())
        {
            m_mapEvClient.erase(mIter);
            bRet = true;
        }
        return bRet;
    }

    bool delClient(XEvClientPtr pClient)
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        bool bRet = false;
        for (auto &mIter : m_mapEvClient)
        {
            if (pClient == mIter.second)
            {
                m_mapEvClient.erase(mIter.first);
                bRet = true;
                break;
            }
        }
        return bRet;
    }

    XEvClientPtr getClient(bufferevent *pBEv)
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        auto mIter = m_mapEvClient.find(pBEv);
        if (mIter == m_mapEvClient.end())
        {
            return nullptr;
        }
        else
        {
            return mIter->second;
        }
    }

    std::atomic_int m_nClientID;
    std::mutex m_clientMutex;
    std::map<bufferevent *, XEvClientPtr> m_mapEvClient;
    std::shared_ptr<event_base> m_pEventBase;
    readFuncPtr m_readCbFunc;
    std::function<void(std::string)> m_errCbFunc;
    std::function<void(std::string)> m_logCbFunc;
    std::shared_ptr<std::thread> m_pThread = nullptr;
};
XSVR_NS_END
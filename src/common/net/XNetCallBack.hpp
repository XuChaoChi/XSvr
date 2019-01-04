#pragma once
#include "base/XBase.h"
#include "memory.h"
#include "XNetDefine.h"
XSVR_NS_BEGIN
using errorCbFunc = std::function<void(const std::string &strError)>;
using logCbFunc = std::function<void(const std::string &strData)>;
template <typename TClient>
class XNetSvrCallBack
{
  public:
    using connectSvrCbFunc = std::function<void(NetState eState, std::shared_ptr<TClient> pClient)>;
    using readCbSvrFunc = std::function<void(std::shared_ptr<TClient> pClient, const char *szBuf, uint32_t nLen)>;
    void setCallBack(logCbFunc logFunc, errorCbFunc errFunc, readCbSvrFunc readFunc, connectSvrCbFunc connectFunc)
    {
        m_logFunc = logFunc;
        m_errorFunc = errFunc;
        m_readFunc = readFunc;
        m_connectFunc = connectFunc;
    }
    void onError(const std::string &strError)
    {
        if (m_errorFunc)
        {
            m_errorFunc(std::move(strError));
        }
    }
    void onLog(const std::string &strLog)
    {
        if (m_logFunc)
        {
            m_logFunc(std::move(strLog));
        }
    }
    void onRead(std::shared_ptr<TClient> pClient, const char *szBuf, uint32_t nLen)
    {
        if (m_readFunc)
        {
            m_readFunc(pClient, szBuf, nLen);
        }
    }

    void onConnectedState(std::shared_ptr<TClient> pClient, NetState eState)
    {
        if (m_connectFunc)
        {
            m_connectFunc(eState, pClient);
        }
    }

  protected:
    readCbSvrFunc m_readFunc;
    connectSvrCbFunc m_connectFunc;
    logCbFunc m_logFunc;
    errorCbFunc m_errorFunc;
};

template <typename TClient>
class XNetClientCallBack
{
  public:
    using readCbFunc = std::function<void(const char *szBuf, uint32_t nLen)>;
    using connectCbFunc = std::function<void(NetState eState)>;
    void setCallBack(logCbFunc logFunc, errorCbFunc errFunc, readCbFunc readFunc, connectCbFunc connectFunc)
    {
        m_logFunc = logFunc;
        m_errorFunc = errFunc;
        m_readFunc = readFunc;
        m_connectFunc = connectFunc;
    }
    void onError(const std::string &strError)
    {
        if (m_errorFunc)
        {
            m_errorFunc(std::move(strError));
        }
    }
    void onLog(const std::string &strLog)
    {
        if (m_logFunc)
        {
            m_logFunc(std::move(strLog));
        }
    }
    void onRead(const char *szBuf, uint32_t nLen)
    {
        if (m_readFunc)
        {
            m_readFunc(szBuf, nLen);
        }
    }

    void onConnectedState(NetState eState)
    {
        if (m_connectFunc)
        {
            m_connectFunc(eState);
        }
    }

  protected:
    readCbFunc m_readFunc;
    connectCbFunc m_connectFunc;
    logCbFunc m_logFunc;
    errorCbFunc m_errorFunc;
};
XSVR_NS_END
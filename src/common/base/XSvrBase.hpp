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
  using  LogFunc = std::function<void(const std::string &strMsg, XSvrLogLevel eLevel)>;
    XSvrBase() : m_bRunning(false)
    {
    }
    virtual ~XSvrBase()
    {
    }
    void start()
    {
        if( m_bDaemon ){
            if(!daemon()){
                return;
            }
        } 
        if (!m_bRunning.load() && _onSvrInit())
        {   
            
            m_bRunning.store(true);
            logInfo("SVR START....");
            _waitForShutDown();     
        } else{
            logError("SVR FAILED....");
        }
    }
    void stop()
    {
        if(m_bRunning.load() && _onSvrStop()){
            m_bRunning.store(false);
        }
    }
    void setLogFunc(const LogFunc &logFunc){
        m_logFunc = logFunc;
    }
  protected:
    void logInfo(const std::string &strMsg){
        if(m_logFunc){
            m_logFunc(strMsg, XSvrLogLevel::eLog_info);
        }
    }
    void logError(const std::string &strMsg){
         if(m_logFunc){
            m_logFunc(strMsg, XSvrLogLevel::eLog_err);
        }
    }
    bool daemon(){
       int32_t nPID = fork();
        if(nPID < 0){
            LOG_E(fmt::format("XSVR START, CANT FORK FROM SELF"));
            return 0;
        }else if(nPID > 0){
            exit(0);
            LOG_I(fmt::format("XSVR START, PID={}", nPID));
            setsid();	//在子进程中创建新会话
            chdir("/");	//改变当前目录为工作目录
            umask(0);	//重设文件权限掩码
          
        }
        return true;
    }
    virtual bool _onSvrInit() = 0;
    virtual bool _onSvrStop() = 0;
    virtual bool _waitForShutDown() = 0;
    LogFunc m_logFunc;
    std::atomic_bool m_bRunning;
    bool m_bDaemon = true;
};
XSVR_NS_END

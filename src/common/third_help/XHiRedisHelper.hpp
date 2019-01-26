#pragma once
#include "base/XBase.h"
#include "hiredis/hiredis.h"
XSVR_NS_BEGIN
class XHiRedisHelper{
    using RedisReplyPtr = std::shared_ptr<redisReply>;
public:
    bool connect(const std::string &strIp, uint16_t nPort, std::string strPw = ""){
        bool bRet = false;
        do {
            m_pRedisCtx = redisConnect(strIp.c_str(), nPort);
            if (m_pRedisCtx == nullptr || m_pRedisCtx->err) {
                if (m_pRedisCtx) {
                    onError(m_pRedisCtx->errstr);
                } else {
                    onError("can't allocate redis context");
                }
                break;
            }
            if(!strPw.empty()){
                auto reply = static_cast<redisReply*>(redisCommand(m_pRedisCtx, "AUTH %s", "chi120047898"));
                if (reply->type == REDIS_REPLY_ERROR) {
                    onError("redis auth failed");
                    break;
                }
                else {
                    onLog("redis connected success");
                    bRet = true;
                }
            }
        } while(0);
        return bRet;
    }

    RedisReplyPtr command(const std::string &strCommand){
        RedisReplyPtr pReply = nullptr;
        do{
            if (!m_pRedisCtx){
                onError("redis context empty");
                break;
            }
            pReply = RedisReplyPtr(static_cast<redisReply*>(redisCommand(m_pRedisCtx, strCommand.c_str())), [](redisReply *pReply){
                freeReplyObject(pReply);
            });
            if(!pReply) {
                onError("command:"+strCommand + "\n" + "error data:" + m_pRedisCtx->errstr);
                break;
            }
        } while(0);
        return pReply;
    }

    void setLogCallBack(std::function<void(const std::string&)> pErrorFunc, std::function<void(const std::string&)> pLogFunc = nullptr){
        m_errFunc = pErrorFunc;
        m_logFunc = pLogFunc;
    }


private:
    void onError(const std::string &strErrMsg){
        if(m_errFunc){
            m_errFunc(strErrMsg);
        }
    }
    void onLog(const std::string &strLogMsg){
        if(m_logFunc){
            m_logFunc(strLogMsg);
        }
    }
    std::function<void(const std::string&)> m_errFunc;
    std::function<void(const std::string&)> m_logFunc;
    redisContext *m_pRedisCtx = nullptr;
};

XSVR_NS_END

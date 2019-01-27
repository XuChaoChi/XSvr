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

    template<typename ...Args>
    RedisReplyPtr command(const char *szBuf, Args ...args){
        RedisReplyPtr pReply = nullptr;
        do{
            if (!m_pRedisCtx){
                onError("redis context empty");
                break;
            }
            pReply = RedisReplyPtr(static_cast<redisReply*>(redisCommand(m_pRedisCtx, szBuf, args...)), [](redisReply *pReply){
                freeReplyObject(pReply);
            });
            if(!checkReply(pReply)) {
                pReply = nullptr;
                break;
            }
        } while(0);
        return pReply;
    }

    template<typename ...Args>
    bool commandString(std::string &strData, const char *szBuf, Args ...args){
        auto pReply = command(szBuf, args...);
        if(checkReply(pReply)){
            strData = std::string(pReply->str, pReply->len);
            return true;
        }
        return false;
    }

    template<typename ...Args>
    bool commandSet(std::vector<std::string> &vSet, const char *szBuf, Args ...args){
        auto pReply = command(szBuf, std::forward<Args...>(args...));
        if(checkReply(pReply)){
            for (uint32_t i = 0; i < pReply->elements; i++){
                vSet.push_back(pReply->element[i]->str);
            }
            return true;
        }
        return false;
    }

    template<typename ...Args>
    bool commandInteger(int64_t &nValue, const char *szBuf, Args ...args){
        auto pReply = command(szBuf, std::forward<Args...>(args...));
        if(checkReply(pReply)){
            nValue = pReply->integer;
            return true;
        }
        return false;
    }

    void setLogCallBack(std::function<void(const std::string&)> pErrorFunc, std::function<void(const std::string&)> pLogFunc = nullptr){
        m_errFunc = pErrorFunc;
        m_logFunc = pLogFunc;
    }


    //abort key
    bool del(const std::string & strKey){
        return checkReply(command("del %s", strKey.c_str()));
    }

    bool exists(const std::string & strKey){
        return checkReplyBoolFromInt(command("exists %s", strKey.c_str()));
    }

    bool expire(const std::string &strKey, uint64_t nSec){
        return checkReplyBoolFromInt(command("expire %s %lld", strKey.c_str(), nSec));
    }

    bool expireAt(const std::string &strKey, uint64_t nTimeStamp){
        return checkReplyBoolFromInt(command("expireat %s %lld", strKey.c_str(), nTimeStamp));
    }

    bool pExpire(const std::string &strKey, uint64_t nMilliSec){
        return checkReplyBoolFromInt(command("pexpire %s %lld", strKey.c_str(), nMilliSec));
    }

    bool pExpireAt(const std::string &strKey, uint64_t nTimeStamp){
        return checkReplyBoolFromInt(command("pexpireat %s %lld", strKey.c_str(), nTimeStamp));
    }

    bool keys(const std::string &strPattern, std::vector<std::string> &vKeys){
        return commandSet(vKeys, "keys %s", strPattern.c_str());
    }

    bool move(const std::string strKey, uint32_t nIndex){
        return checkReplyBoolFromInt(command("move %s %d", strKey.c_str(), nIndex));
    }

    bool persist(const std::string &strKey){
        return checkReplyBoolFromInt(command("persist %s", strKey.c_str()));
    }

    bool pTTL(const std::string &strKey, int64_t &nPTTL){
        return commandInteger(nPTTL, "pttl %s", strKey.c_str());
    }

    bool TTL(const std::string &strKey, int64_t &nTTL){
        return commandInteger(nTTL, "ttl %s", strKey.c_str());
    }

    bool randomKey(std::string &strValue){
        return commandString(strValue, "randomkey");
    }

    bool rename(const std::string &strKeyName, const std::string strNewKeyName){
        return (nullptr != command("rename %s %s", strKeyName.c_str(), strNewKeyName.c_str()));
    }

    bool renameNX(const std::string &strKeyName, const std::string strNewKeyName){
        return checkReplyBoolFromInt(command("renamenx %s %s", strKeyName.c_str(), strNewKeyName.c_str()));
    }

    bool type(const std::string &strKey, std::string &strType){
        return commandString(strType, "type %s", strKey.c_str());
    }


//    bool dump(const std::string &strKey, std::string &strValue){
//        return commandString(strValue, "dump %s", strKey.c_str());
//    }

    //abort string
    bool set(const std::string& strKey, const std::string &strValue){
        return checkReply(command("set %s %s", strKey.c_str(), strValue.c_str()));
    }

    bool get(const std::string &strKey, std::string &strValue){
        return commandString(strValue, "get %s", strKey.c_str());
    }

    bool getRange(const std::string &strKey, uint32_t nBegin, uint32_t nEnd,std::string &strValue){
        auto pReply = command("getrange %s %d %d", strKey.c_str(), nBegin, nEnd);
        bool bRet = checkReply(pReply);
        if (bRet){
            strValue = std::move(std::string(pReply->str, pReply->len));
        }
        return bRet;
    }
    //abort hash
private:
    bool checkReplyBoolFromInt(RedisReplyPtr pReply){
        if(pReply){
            if(pReply->type == REDIS_REPLY_INTEGER  && pReply->integer){
                return true;
            }
        }
        return false;
    }

    bool checkReply(RedisReplyPtr pReply){
        bool bRet = true;
        do{
            if(!pReply){
                bRet = false;
                break;
            }
            switch(pReply->type){
            case REDIS_REPLY_STRING:
                break;
            case REDIS_REPLY_ARRAY:
                break;
            case REDIS_REPLY_INTEGER:
                break;
            case REDIS_REPLY_NIL:
                bRet = false;
                break;
            case REDIS_REPLY_STATUS:
                if( pReply->len == 2 && 0 != strcmp(pReply->str, "OK")){
                    bRet = false;
                }
                break;
            case REDIS_REPLY_ERROR:
                bRet = false;
                break;
            default:
                bRet = false;
            }
        } while(0);
        if (!bRet && pReply){
            std::string strErrorMsg = "ERROR CTX:";
            strErrorMsg += m_pRedisCtx->errstr;
            strErrorMsg += "\nERROR REPLY:";
            strErrorMsg += pReply->str;
            onError(strErrorMsg);
        }
        return bRet;
    }


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

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
    bool commandArray(std::vector<std::string> &vValues, const char *szBuf, Args ...args){
        auto pReply = command(szBuf, args...);
        if(checkReply(pReply)){
            for (uint32_t i = 0; i < pReply->elements; i++){
                if (pReply->element[i]->str){
                    vValues.push_back(pReply->element[i]->str);
                }
            }
            return true;
        }
        return false;
    }

    template<typename ...Args>
    bool commandInteger(int64_t &nValue, const char *szBuf, Args ...args){
        auto pReply = command(szBuf, args...);
        if(checkReply(pReply)){
            nValue = pReply->integer;
            return true;
        }
        return false;
    }

    template<typename ...Args>
    bool commandDouble(double &nValue, const char *szBuf, Args ...args){
        auto pReply = command(szBuf, args...);
        if(checkReply(pReply)){
            if(pReply->str){
                nValue = std::atof(std::string(pReply->str, pReply->len).c_str());
            }
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
        return commandArray(vKeys, "keys %s", strPattern.c_str());
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

    bool getRange(const std::string &strKey, int64_t nBegin, int64_t nEnd,std::string &strValue){
        return commandString(strValue, "getrange %s %lld %lld", strKey.c_str(), nBegin, nEnd);
    }

    bool getSet(const std::string &strKey, const std::string &strNewValue, std::string &strOldValue){
        return commandString(strOldValue, "getset %s %s", strKey.c_str(), strNewValue.c_str());
    }

    bool getBit(const std::string &strKey, int64_t nOffset){
        return checkReplyBoolFromInt(command("getbit %s %lld", strKey.c_str(), nOffset));
    }

    bool mGet(const std::vector<std::string> vKeys, std::vector<std::string> &vValues){
        return commandArray(vValues,  makeKeysToString("mget", vKeys).c_str());
    }

    bool setBit(const std::string &strKey, int64_t nOffset, bool bBit){
        return checkReplyBoolFromInt(command("setbit %s %lld %d", strKey.c_str(), nOffset, (int32_t)bBit));
    }

    bool setEx(const std::string &strKey, int64_t nSec, const std::string strValue){
        return checkReply(command("setex %s %lld %s", strKey.c_str(), nSec, strValue.c_str()));
    }

    bool setNx(const std::string &strKey, const std::string &strValue) {
        return checkReplyBoolFromInt(command("setnx %s %s", strKey.c_str(), strValue.c_str()));
    }

    bool setRange(const std::string &strKey, int64_t nOffset, const std::string &strValue){
        return checkReplyBoolFromInt(command("setrange %s %lld %s", strKey.c_str(), nOffset, strValue.c_str()));
    }

    bool strlen(const std::string &strKey, int64_t &nLen){
        return commandInteger(nLen, "strlen %s", strKey.c_str());
    }

    bool mSet(const std::map<std::string, std::string> &mData){
        return checkReply(command(makeKeyValuesToString("mset", mData).c_str()));
    }

    bool mSetNx(const std::map<std::string, std::string> &mData){
        return checkReplyBoolFromInt(command(makeKeyValuesToString("msetnx", mData).c_str()));
    }
    //5.0.2 unknow command
    bool pSetEx(const std::string &strKey, int64_t nMilliSec){
        return checkReplyBoolFromInt(command("psetex %s %lld", strKey.c_str(), nMilliSec));
    }

    bool incr(const std::string &strKey, int64_t &nRet){
        return commandInteger(nRet, "incr %s", strKey.c_str());
    }

    bool incrBy(const std::string &strKey, int64_t nStep, int64_t &nRet){
       return commandInteger(nRet, "incrby %s %lld", strKey.c_str(), nStep);
    }

    bool incrByFloat(const std::string &strKey, double nStep, double &nRet){
        return commandDouble(nRet, "incrbyfloat %s %f", strKey.c_str(), nStep);
    }

    bool decr(const std::string &strKey, int64_t &nRet){
        return commandInteger(nRet, "decr %s", strKey.c_str());
    }

    bool decrBy(const std::string &strKey, int64_t nStep, int64_t &nRet){
        return commandInteger(nRet, "decrby %s %lld", strKey.c_str(), nStep);
    }

    bool append(const std::string &strKey, const std::string &strAppend, int64_t &nRet){
        return commandInteger(nRet, "append %s %s", strKey.c_str(), strAppend.c_str());
    }
    //abort hash
private:

    std::string makeKeysToString(const std::string &strCommand, const std::vector<std::string> &vKeys) const {
        std::string strRet = strCommand;
        for (auto &vIter : vKeys){
            strRet += " ";
            strRet += vIter;
        }
        return strRet;
    }

    std::string makeKeyValuesToString(const std::string &strCommand, const std::map<std::string, std::string> mData){
        std::string strRet = strCommand;
        for(auto &mIter : mData){
            strRet += " " + mIter.first + " " + mIter.second;
        }
        return strRet;
    }

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
            if(pReply->str){
                strErrorMsg += pReply->str;
            }
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

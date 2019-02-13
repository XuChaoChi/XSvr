#pragma once
#include "base/XBase.h"
#include "hiredis/hiredis.h"
XSVR_NS_BEGIN


class XHiRedisHelper {
    using RedisReplyPtr = std::shared_ptr<redisReply>;
public:
    bool connect(const std::string &strIp, uint16_t nPort, std::string strPw = "") {
        bool bRet = false;
        do {
            m_pRedisCtx = redisConnect(strIp.c_str(), nPort);
            if (m_pRedisCtx == nullptr || m_pRedisCtx->err) {
                if (m_pRedisCtx) {
                    onError(m_pRedisCtx->errstr);
                }
                else {
                    onError("can't allocate redis context");
                }
                break;
            }
            if (!strPw.empty()) {
                auto reply = static_cast<redisReply*>(redisCommand(m_pRedisCtx, "AUTH %s", strPw.c_str()));
                if (reply->type == REDIS_REPLY_ERROR) {
                    onError("redis auth failed");
                    break;
                }
                else {
                    onLog("redis connected success");
                    bRet = true;
                }
            }
        } while (0);
        return bRet;
    }

    template<typename ...Args>
    RedisReplyPtr command(const char *szBuf, Args ...args) {
        RedisReplyPtr pReply = nullptr;
        do {
            if (!m_pRedisCtx) {
                onError("redis context empty");
                break;
            }
            pReply = RedisReplyPtr(static_cast<redisReply*>(redisCommand(m_pRedisCtx, szBuf, args...)), [](redisReply *pReply) {
                if (pReply) {
                    freeReplyObject(pReply);
                }
            });
            if (!checkReply(pReply)) {
                pReply = nullptr;
                break;
            }
        } while (0);
        return pReply;
    }

    template<typename ...Args>
    bool commandString(std::string &strData, const char *szBuf, Args ...args) {
        auto pReply = command(szBuf, args...);
        if (checkReply(pReply)) {
            if (pReply->str) {
                strData = std::string(pReply->str, pReply->len);
            }
            else {
                strData = "null";
            }

            return true;
        }
        return false;
    }

    template<typename ...Args>
    bool commandArray(std::vector<std::string> &vValues, const char *szBuf, Args ...args) {
        auto pReply = command(szBuf, args...);
        return getCommandArray(pReply, vValues);
    }

    template<typename ...Args>
    bool commandHash(std::map<std::string, std::string> &mData, const char *szBuf, Args ...args) {
        auto pReply = command(szBuf, args...);
        return getCommandHash(pReply, mData);
    }

    template<typename ...Args>
    bool commandInteger(int64_t &nValue, const char *szBuf, Args ...args) {
        auto pReply = command(szBuf, args...);
        if (checkReply(pReply)) {
            nValue = pReply->integer;
            return true;
        }
        return false;
    }

    template<typename ...Args>
    bool commandDouble(double &nValue, const char *szBuf, Args ...args) {
        auto pReply = command(szBuf, args...);
        if (checkReply(pReply)) {
            if (pReply->str) {
                nValue = std::atof(std::string(pReply->str, pReply->len).c_str());
            }
            return true;
        }
        return false;
    }

    void setLogCallBack(std::function<void(const std::string&)> pErrorFunc, std::function<void(const std::string&)> pLogFunc = nullptr) {
        m_errFunc = pErrorFunc;
        m_logFunc = pLogFunc;
    }


    //abort key
    bool del(const std::string & strKey) {
        return checkReplyBoolFromInt(command("del %s", strKey.c_str()));
    }

    bool exists(const std::string & strKey) {
        return checkReplyBoolFromInt(command("exists %s", strKey.c_str()));
    }

    bool expire(const std::string &strKey, uint64_t nSec) {
        return checkReplyBoolFromInt(command("expire %s %lld", strKey.c_str(), nSec));
    }

    bool expireAt(const std::string &strKey, uint64_t nTimeStamp) {
        return checkReplyBoolFromInt(command("expireat %s %lld", strKey.c_str(), nTimeStamp));
    }

    bool pExpire(const std::string &strKey, uint64_t nMilliSec) {
        return checkReplyBoolFromInt(command("pexpire %s %lld", strKey.c_str(), nMilliSec));
    }

    bool pExpireAt(const std::string &strKey, uint64_t nTimeStamp) {
        return checkReplyBoolFromInt(command("pexpireat %s %lld", strKey.c_str(), nTimeStamp));
    }

    bool keys(const std::string &strPattern, std::vector<std::string> &vKeys) {
        return commandArray(vKeys, "keys %s", strPattern.c_str());
    }

    bool move(const std::string strKey, uint32_t nIndex) {
        return checkReplyBoolFromInt(command("move %s %d", strKey.c_str(), nIndex));
    }

    bool persist(const std::string &strKey) {
        return checkReplyBoolFromInt(command("persist %s", strKey.c_str()));
    }

    bool pTTL(const std::string &strKey, int64_t &nPTTL) {
        return commandInteger(nPTTL, "pttl %s", strKey.c_str());
    }

    bool TTL(const std::string &strKey, int64_t &nTTL) {
        return commandInteger(nTTL, "ttl %s", strKey.c_str());
    }

    bool randomKey(std::string &strValue) {
        return commandString(strValue, "randomkey");
    }

    bool rename(const std::string &strKeyName, const std::string strNewKeyName) {
        return (nullptr != command("rename %s %s", strKeyName.c_str(), strNewKeyName.c_str()));
    }

    bool renameNX(const std::string &strKeyName, const std::string strNewKeyName) {
        return checkReplyBoolFromInt(command("renamenx %s %s", strKeyName.c_str(), strNewKeyName.c_str()));
    }

    bool type(const std::string &strKey, std::string &strType) {
        return commandString(strType, "type %s", strKey.c_str());
    }


    //    bool dump(const std::string &strKey, std::string &strValue){
    //        return commandString(strValue, "dump %s", strKey.c_str());
    //    }

        //abort string
    bool set(const std::string& strKey, const std::string &strValue) {
        return checkReply(command("set %s %s", strKey.c_str(), strValue.c_str()));
    }

    bool get(const std::string &strKey, std::string &strValue) {
        return commandString(strValue, "get %s", strKey.c_str());
    }

    bool getRange(const std::string &strKey, int64_t nBegin, int64_t nEnd, std::string &strValue) {
        return commandString(strValue, "getrange %s %lld %lld", strKey.c_str(), nBegin, nEnd);
    }

    bool getSet(const std::string &strKey, const std::string &strNewValue, std::string &strOldValue) {
        return commandString(strOldValue, "getset %s %s", strKey.c_str(), strNewValue.c_str());
    }

    bool getBit(const std::string &strKey, int64_t nOffset) {
        return checkReplyBoolFromInt(command("getbit %s %lld", strKey.c_str(), nOffset));
    }

    bool mGet(const std::vector<std::string> vKeys, std::vector<std::string> &vValues) {
        std::string strCommand = "mget" + makeKeysToString(vKeys);
        return commandArray(vValues, strCommand.c_str());
    }

    bool setBit(const std::string &strKey, int64_t nOffset, bool bBit) {
        return checkReplyBoolFromInt(command("setbit %s %lld %d", strKey.c_str(), nOffset, (int32_t)bBit));
    }

    bool setEx(const std::string &strKey, int64_t nSec, const std::string strValue) {
        return checkReply(command("setex %s %lld %s", strKey.c_str(), nSec, strValue.c_str()));
    }

    bool setNx(const std::string &strKey, const std::string &strValue) {
        return checkReplyBoolFromInt(command("setnx %s %s", strKey.c_str(), strValue.c_str()));
    }

    bool setRange(const std::string &strKey, int64_t nOffset, const std::string &strValue) {
        return checkReplyBoolFromInt(command("setrange %s %lld %s", strKey.c_str(), nOffset, strValue.c_str()));
    }

    bool strlen(const std::string &strKey, int64_t &nLen) {
        return commandInteger(nLen, "strlen %s", strKey.c_str());
    }

    bool mSet(const std::map<std::string, std::string> &mData) {
        std::string strCommand = "mset" + makeKeyValuesToString(mData);
        return checkReply(command(strCommand.c_str()));
    }

    bool mSetNx(const std::map<std::string, std::string> &mData) {
        std::string strCommand = "msetnx" + makeKeyValuesToString(mData);
        return checkReplyBoolFromInt(command(strCommand.c_str()));
    }
    //5.0.2 unknow command
    bool pSetEx(const std::string &strKey, int64_t nMilliSec) {
        return checkReplyBoolFromInt(command("psetex %s %lld", strKey.c_str(), nMilliSec));
    }

    bool incr(const std::string &strKey, int64_t &nRet) {
        return commandInteger(nRet, "incr %s", strKey.c_str());
    }

    bool incrBy(const std::string &strKey, int64_t nStep, int64_t &nRet) {
        return commandInteger(nRet, "incrby %s %lld", strKey.c_str(), nStep);
    }

    bool incrByFloat(const std::string &strKey, double nStep, double &nRet) {
        return commandDouble(nRet, "incrbyfloat %s %f", strKey.c_str(), nStep);
    }

    bool decr(const std::string &strKey, int64_t &nRet) {
        return commandInteger(nRet, "decr %s", strKey.c_str());
    }

    bool decrBy(const std::string &strKey, int64_t nStep, int64_t &nRet) {
        return commandInteger(nRet, "decrby %s %lld", strKey.c_str(), nStep);
    }

    bool append(const std::string &strKey, const std::string &strAppend, int64_t &nRet) {
        return commandInteger(nRet, "append %s %s", strKey.c_str(), strAppend.c_str());
    }
    //abort hash

    bool hDel(const std::string &strKey, const std::vector<std::string> &vFiled) {
        std::string strCommand = "hdel " + strKey + makeKeysToString(vFiled);
        return checkReplyBoolFromInt(command(strCommand.c_str()));
    }

    bool hExists(const std::string &strKey, const std::string &strFiled) {
        return checkReplyBoolFromInt(command("hexists %s %s", strKey.c_str(), strFiled.c_str()));
    }

    bool hGetAll(const std::string &strKey, std::map<std::string, std::string> &mData) {
        return commandHash(mData, "hgetall %s", strKey.c_str());
    }

    bool hIncrBy(const std::string &strKey, const std::string &strFiled, int64_t nIncr, int64_t &nRet) {
        return commandInteger(nRet, "hincrby %s %s %lld", strKey.c_str(), strFiled.c_str(), nIncr);
    }

    bool hIncrByFloat(const std::string &strKey, const std::string &strFiled, double nIncr, double &nRet) {
        return commandDouble(nRet, "hincrbyfloat %s %s %f", strKey.c_str(), strFiled.c_str(), nIncr);
    }

    bool hKeys(const std::string &strKey, std::vector<std::string> &vFiled) {
        return commandArray(vFiled, "hkeys %s", strKey.c_str());
    }

    bool hLen(const std::string &strKey, int64_t &nLen) {
        return commandInteger(nLen, "hlen %s", strKey.c_str());
    }

    bool hMGet(const std::string &strKey, const std::vector<std::string> vFiled, std::vector<std::string> &vValue) {
        std::string strCommand = "hmget " + strKey + makeKeysToString(vFiled);
        return commandArray(vValue, strCommand.c_str());
    }

    bool hMSet(const std::string &strKey, const std::map<std::string, std::string> &mData) {
        std::string strCommand = "hmset " + strKey + makeKeyValuesToString(mData);
        return checkReply(command(strCommand.c_str()));
    }

    bool hSet(const std::string &strKey, const std::string &strFiled, const std::string &strValue) {
        return checkReplyBoolFromInt(command("hset %s %s %s", strKey.c_str(), strFiled.c_str(), strValue.c_str()));
    }

    bool hSetNx(const std::string &strKey, const std::string &strFiled, const std::string &strValue) {
        return checkReplyBoolFromInt(command("hsetnx %s %s %s", strKey.c_str(), strFiled.c_str(), strValue.c_str()));
    }

    bool hVals(const std::string &strKey, std::vector<std::string> &vValues) {
        return commandArray(vValues, "hvals %s", strKey.c_str());
    }

    bool hScan(const std::string &strKey, int64_t nPattern, int64_t nPageCnt, int64_t &nCnt, std::map<std::string, std::string> &mData) {
        auto pReply = command("hscan %s %lld count %lld", strKey.c_str(), nPattern, nPageCnt);
        if (checkReply(pReply)) {
            if (pReply) {
                nCnt = std::atoll(pReply->element[0]->str);
            }
        }
        auto pDataReply = RedisReplyPtr(pReply->element[1], [](redisReply *pReply) {
            if (pReply) {
                freeReplyObject(pReply);
            }
        });
        return getCommandHash(pDataReply, mData);
    }
    //abort list
    bool bLPop(const std::string &strKey, int32_t nTimeOut, std::string &strPop) {
        std::vector<std::string> vData;
        if (commandArray(vData, "blpop %s %d", strKey.c_str(), nTimeOut)) {
            if (vData.size() == 2) {
                strPop = vData.at(1);
                return true;
            }
        }
        return false;
    }

    bool bRPop(const std::string &strKey, int32_t nTimeOut, std::string &strPop) {
        std::vector<std::string> vData;
        if (commandArray(vData, "bRpop %s %d", strKey.c_str(), nTimeOut)) {
            if (vData.size() == 2) {
                strPop = vData.at(1);
                return true;
            }
        }
        return false;
    }

    bool bRPopLPush(const std::string &strSource, const std::string &strDest, int32_t nTimeOut, std::string &strChange) {
        return commandString(strChange, "brpoplpush %s %s %d", strSource.c_str(), strDest.c_str(), nTimeOut);
    }

    bool lIndex(const std::string &strKey, int64_t nIndex, std::string &strValue) {
        return commandString(strValue, "lindex %s %ld", strKey.c_str(), nIndex);
    }

    bool lPop(const std::string &strKey, std::string &strValue) {
        return commandString(strValue, "lpop %s ", strKey.c_str());
    }

    bool lPush(const std::string &strKey, const std::vector<std::string> vKeys, int64_t &nCnt) {
        std::string strCommand = "lpush " + strKey + makeKeysToString(vKeys);
        return commandInteger(nCnt, strCommand.c_str());
    }

    bool lPushX(const std::string &strKey, const std::string &strValue, int64_t &nCnt) {
        if (commandInteger(nCnt, "lpushx %s %s", strKey.c_str(), strValue.c_str()) && nCnt != 0) {
            return true;
        }
        return false;
    }

    bool lRange(const std::string &strKey, int64_t nBegin, int64_t nEnd, std::vector<std::string> &vValues) {
        return commandArray(vValues, "lrange %s %lld %lld", strKey.c_str(), nBegin, nEnd);
    }

    bool lRem(const std::string &strKey, int64_t nCnt, const std::string &strValue, int64_t &nRet) {
        if (commandInteger(nRet, "lrem %s %lld %s", strKey.c_str(), nCnt, strValue.c_str()) && nRet != 0) {
            return true;
        }
        return false;
    }

    bool lSet(const std::string &strKey, int64_t nIndex, const std::string &strValue) {
        return checkReply(command("lset %s %lld %s", strKey.c_str(), nIndex, strValue.c_str()));
    }

    bool lTrim(const std::string &strKey, int64_t nBegin, int64_t nEnd) {
        return checkReply(command("ltrim %s %lld %lld", strKey.c_str(), nBegin, nEnd));
    }

    bool rRop(const std::string &strKey, std::string &strValue) {
        return commandString(strValue, "rpop %s", strKey.c_str());
    }

    bool rPopLPush(const std::string &strSource, const std::string &strDest, std::string &strValue) {
        return commandString(strValue, "rpoplpush %s %s", strSource.c_str(), strDest.c_str());
    }

    bool rPush(const std::string &strKey, const std::vector<std::string> &vKeys, int64_t &nRet) {
        std::string strCommand = "rpush " + strKey + makeKeysToString(vKeys);
        return commandInteger(nRet, strCommand.c_str());
    }

    bool rPushX(const std::string &strKey, const std::string &strValue, int64_t &nCnt) {
        if (commandInteger(nCnt, "rpushx %s %s", strKey.c_str(), strValue.c_str()) && nCnt != 0) {
            return true;
        }
        return false;
    }

    bool lLen(const std::string &strKey, int64_t &nRet) {
        return commandInteger(nRet, "llen %s", strKey.c_str()) && nRet > 0;
    }

    bool lInsert(const std::string &strKey, bool bBefore, const std::string &strPivot, const std::string &strValue, int64_t &nRet) {
        std::string strCommand = "linsert " + strKey;
        strCommand += bBefore ? " before " : " after ";
        strCommand += strPivot + " " + strValue;
        return commandInteger(nRet, strCommand.c_str()) && nRet > 0;
    }

    //abort set
    bool sAdd(const std::string &strKey, const std::vector<std::string> &vMem) {
        std::string strCommand = "sadd " + strKey + " " + makeKeysToString(vMem);
        return checkReplyBoolFromInt(command(strCommand.c_str()));
    }

    bool sCard(const std::string &strKey, int64_t &nCnt) {
        return commandInteger(nCnt, "scard %s", strKey.c_str());
    }

    bool sDiff(const std::string &strKey1, const std::string &strKey2, std::vector<std::string> &vDiff) {
        return commandArray(vDiff, "sdiff %s %s", strKey1.c_str(), strKey2.c_str());
    }

    bool sDiffStore(const std::string &strDest, const std::string &strKey1, const std::string &strKey2, int64_t &nRet) {
        return commandInteger(nRet, "sdiffstore %s %s %s", strDest.c_str(), strKey1.c_str(), strKey2.c_str()) && nRet > 0;
    }

    bool sInter(const std::string &strKey1, const std::string &strKey2, std::vector<std::string> &vValues) {
        return commandArray(vValues, "sinter %s %s", strKey1.c_str(), strKey2.c_str());
    }

    bool sInterStore(const std::string &strDest, const std::string &strKey1, const std::string &strKey2, int64_t &nRet) {
        return commandInteger(nRet, "sinterstore %s %s %s", strDest.c_str(), strKey1.c_str(), strKey2.c_str()) && nRet > 0;
    }

    bool sisMember(const std::string &strKey, const std::string &strMem) {
        return checkReplyBoolFromInt(command("sismember %s %s", strKey.c_str(), strMem.c_str()));
    }

    bool sMembers(const std::string &strKey, std::vector<std::string> &vValues) {
        return commandArray(vValues, "smembers %s", strKey.c_str());
    }

    bool sMove(const std::string &strSource, const std::string &strDest, const std::string &strMem) {
        return checkReplyBoolFromInt(command("smove %s %s %s", strSource.c_str(), strDest.c_str(), strMem.c_str()));
    }

    bool sPop(const std::string &strKey, std::string &strValue) {
        return commandString(strValue, "spop %s", strKey.c_str());
    }

    bool sRandMember(const std::string &strKey, int64_t nCnt, std::vector<std::string> &vValues) {
        return commandArray(vValues, "srandmember %s %lld", strKey.c_str(), nCnt);
    }

    bool sRem(const std::string &strKey, const std::vector <std::string> &vMems, int64_t &nRet) {
        std::string strCommand = "srem " + strKey + " " + makeKeysToString(vMems);
        return commandInteger(nRet, strCommand.c_str()) && nRet > 0;
    }

    bool sUnion(const std::vector<std::string> vKeys, std::vector<std::string> &vValues) {
        std::string strCommand = "sunion " + makeKeysToString(vKeys);
        return commandArray(vValues, strCommand.c_str());
    }

    bool sUnionStore(const std::string &strKey, const std::vector<std::string> vKeys, int64_t &nRet) {
        std::string strCommand = "sunionstore " + strKey + " " + makeKeysToString(vKeys);
        return commandInteger(nRet, strCommand.c_str()) && nRet > 0;
    }

    bool sScan(const std::string &strKey, int64_t nCursor, int64_t &nRet, std::vector<std::string> &vData, std::string strPattern = "*", int64_t nCnt = 10) {
        std::string strCommand = "sScan " + strKey + " " + std::to_string(nCursor) + " match " + strPattern + " count " + std::to_string(nCnt);
        auto pReply = command(strCommand.c_str());
        if (checkReply(pReply)) {
            if (pReply) {
                nRet = std::atoll(pReply->element[0]->str);
            }
        }
        auto pDataReply = RedisReplyPtr(pReply->element[1], [](redisReply *pReply) {
            if (pReply) {
                freeReplyObject(pReply);
            }
        });
        return getCommandArray(pDataReply, vData);
    }

    //abort sortedset
    bool zAdd(const std::string &strKey, std::map<std::string, std::string> &mData, int64_t &nCnt) {
        std::string strCommad = "zadd " + strKey + makeKeyValuesToString(mData, true);
        return commandInteger(nCnt, strCommad.c_str());
    }

    bool zCard(const std::string &strKey, int64_t &nRet) {
        return commandInteger(nRet, "zcard %s", strKey.c_str()) && nRet > 0;
    }

    bool zCount(const std::string &strKey, double nScoreBegin, double nScoreEnd, int64_t &nRet) {
        return commandInteger(nRet, "zcount %s %f %f", strKey.c_str(), nScoreBegin, nScoreEnd);
    }

    bool zIncrby(const std::string &strKey, double nCnt, const std::string &strMem, double &nRet) {
        return commandDouble(nRet, "zincrby %s %f %s", strKey.c_str(), nCnt, strMem.c_str());
    }

    bool zInterStore(const std::string &strDestKey, const std::vector<std::string> &vKeys, int64_t &nRet) {
        std::string strCommand = "zinterstore " + strDestKey + " " + std::to_string(vKeys.size()) + makeKeysToString(vKeys);
        return commandInteger(nRet, strCommand.c_str());
    }

    bool zLexCount(const std::string &strKey, const std::string &strBegin, const std::string &strEnd, int64_t &nRet) {
        std::string strCommand = "zlexcount " + strKey + " " + strBegin + " " + strEnd;
        return commandInteger(nRet, strCommand.c_str());
    }

    bool zRange(const std::string &strKey, int64_t nBegin, int64_t nEnd, std::map <std::string, std::string> &mData) {
        return commandHash(mData, "zrange %s %lld %lld withscores", strKey.c_str(), nBegin, nEnd);
    }

    bool zRangeByLex(const std::string &strKey, const std::string &strMin, const std::string strMax, std::vector<std::string> &vData, int64_t nOffset = 0, int64_t nCnt = -1){
        std::string strMinTemp = strMax.empty() ?  " - " + strMin : strMin;
        std::string strCommand = "zrangebylex " + strKey + " " + strMinTemp + " " + strMax + " limit " + std::to_string(nOffset) + " " + std::to_string(nCnt);
        return  commandArray(vData, strCommand.c_str());
    }

    bool zRangeByScore(const std::string &strKey, const std::string &strMin, const std::string &strMax,  std::map<std::string, std::string> &mData, int64_t nOffset = 0, int64_t nCnt = -1){
        std::string strCommand = "zrangebyscore " + strKey + " " + strMin + " " + strMax + " withscores limit " + std::to_string(nOffset) + " " + std::to_string(nCnt);
        return  commandHash(mData, strCommand.c_str());
    }

    bool zRank(const std::string &strKey, const std::string &strMem, int64_t &nIndex){
        return commandInteger(nIndex, "zrank %s %s", strKey.c_str(), strMem.c_str());
    }

    bool zRem(const std::string &strKey, const std::vector<std::string> &vMems, int64_t &nRet){
        std::string strCommand = "zrem " + strKey + makeKeysToString(vMems);
        return commandInteger(nRet, strCommand.c_str()) && nRet > 0;
    }

    bool zRemRangeByLex(const std::string &strKey, const std::string &strMin, const std::string &strMax, int64_t &nRet){
        return commandInteger(nRet, "zremrangebylex %s %s %s", strKey.c_str(), strMin.c_str(), strMax.c_str()) && nRet > 0;
    }

    bool zRemRangeByRank(const std::string &strKey, int64_t nBegin, int64_t nEnd, int64_t &nRet){
        return commandInteger(nRet, "zremrangebyrank %s %lld %lld", strKey.c_str(), nBegin, nEnd) && nRet > 0;
    }

    bool zRemRangeByScore(const std::string &strKey, double nMin, double nMax, int64_t &nRet){
        return commandInteger(nRet, "zremrangebyscore %s %f %f", strKey.c_str(), nMin, nMax) && nRet > 0;
    }

    bool zRevRange(const std::string &strKey, int64_t nBegin, int64_t nEnd, std::map<std::string, std::string> &mData){
        return commandHash(mData, "zrevrange %s %lld %lld withscores", strKey.c_str(), nBegin, nEnd);
    }

    bool zRevRangeByScore(const std::string &strKey, double nMin, double nMax, std::map<std::string, std::string> &mData){
        return commandHash(mData, "zrevrangebyscore %s %f %f withscores", strKey.c_str(), nMax, nMin);
    }

    bool zRevRank(const std::string &strKey, const std::string &strMem, int64_t &nRet){
        return commandInteger(nRet, "zrevrank %s %s", strKey.c_str(), strMem.c_str());
    }

    bool zScore(const std::string &strKey, const std::string &strMem, double &nRet){
        return commandDouble(nRet, "zscore %s %s", strKey.c_str(), strMem.c_str());
    }

    bool zUnionStore(const std::string &strKey, const std::map<std::string, double> &mKeyWidgets, int64_t &nRet, std::string strAggregate = "sum"){
        std::string strCommand = "zunionstore " + strKey + " " + std::to_string(mKeyWidgets.size());
        std::string strKeys;
        std::string strWidgets;
        for(auto &keyIter : mKeyWidgets){
            strKeys += " ";
            strWidgets += " ";
            strKeys += keyIter.first;
            strWidgets += std::to_string(keyIter.second);
        }
        strCommand = strCommand + strKeys + " weights " + strWidgets + " AGGREGATE " + strAggregate;
        return commandInteger(nRet, strCommand.c_str()) && nRet > 0;
    }


    bool zScan(const std::string &strKey, int64_t nCursor, int64_t &nRet, std::map<std::string, std::string> &mData, std::string strPattern = "*", int64_t nCnt = 10) {
        std::string strCommand = "zScan " + strKey + " " + std::to_string(nCursor) + " match " + strPattern + " count " + std::to_string(nCnt);
        auto pReply = command(strCommand.c_str());
        if (checkReply(pReply)) {
            if (pReply) {
                nRet = std::atoll(pReply->element[0]->str);
            }
        }
        auto pDataReply = RedisReplyPtr(pReply->element[1], [](redisReply *pReply) {
            if (pReply) {
                freeReplyObject(pReply);
            }
        });
        return getCommandHash(pDataReply, mData);
    }


private:
    bool getCommandHash(RedisReplyPtr pReply, std::map<std::string, std::string> &mData) {
        if (checkReply(pReply)) {
            for (uint32_t i = 0; i < pReply->elements; i += 2) {
                if (pReply->element[i]->str && pReply->element[i + 1]->str) {
                    if (pReply->element[i + 1]->str) {
                        mData[pReply->element[i]->str] = pReply->element[i + 1]->str;
                    }
                    else {
                        mData[pReply->element[i]->str] = "null";
                    }

                }
            }
            if (pReply->len != 0 || pReply->elements != 0) {
                return true;
            }
        }
        return false;
    }

    bool getCommandArray(RedisReplyPtr pReply, std::vector<std::string> &vData) {
        if (checkReply(pReply)) {
            for (uint32_t i = 0; i < pReply->elements; i++) {
                if (pReply->element[i]->str) {
                    vData.push_back(pReply->element[i]->str);
                }
                else {
                    vData.push_back("null");
                }
            }
            if (pReply->len != 0 || pReply->elements != 0) {
                return true;
            }
        }
        return false;
    }

    std::string makeKeysToString(const std::vector<std::string> &vKeys) const {
        std::string strRet;
        for (auto &vIter : vKeys) {
            strRet += " ";
            strRet += vIter;
        }
        return strRet;
    }

    std::string makeKeyValuesToString(const std::map<std::string, std::string> mData, bool bReverse = false) {
        std::string strRet;
        for (auto &mIter : mData) {
            if (!bReverse) {
                strRet += " " + mIter.first + " " + mIter.second;
            }
            else {
                strRet += " " + mIter.second + " " + mIter.first;
            }

        }
        return strRet;
    }

    bool checkReplyBoolFromInt(RedisReplyPtr pReply) {
        if (pReply) {
            if (pReply->type == REDIS_REPLY_INTEGER && pReply->integer) {
                return true;
            }
        }
        return false;
    }

    bool checkReply(RedisReplyPtr pReply) {
        bool bRet = true;
        do {
            if (!pReply) {
                bRet = false;
                break;
            }
            switch (pReply->type) {
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
                if (pReply->len == 2 && 0 != strcmp(pReply->str, "OK")) {
                    bRet = false;
                }
                break;
            case REDIS_REPLY_ERROR:
                bRet = false;
                break;
            default:
                bRet = false;
            }
        } while (0);
        if (!bRet && pReply) {
            std::string strErrorMsg = "ERROR CTX:";
            strErrorMsg += m_pRedisCtx->errstr;
            strErrorMsg += "\nERROR REPLY:";
            if (pReply->str) {
                strErrorMsg += pReply->str;
            }
            onError(strErrorMsg);
        }
        return bRet;
    }


    void onError(const std::string &strErrMsg) {
        if (m_errFunc) {
            m_errFunc(strErrMsg);
        }
    }
    void onLog(const std::string &strLogMsg) {
        if (m_logFunc) {
            m_logFunc(strLogMsg);
        }
    }
    std::function<void(const std::string&)> m_errFunc;
    std::function<void(const std::string&)> m_logFunc;
    redisContext *m_pRedisCtx = nullptr;
};
XSVR_NS_END

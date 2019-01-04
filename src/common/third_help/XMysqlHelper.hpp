#pragma once
#include "memory"
#include "mysql.h"
#include <string>
#include <functional>
#include <sstream>
#include "base/XBase.h"
XSVR_NS_BEGIN
enum SqlRetCode{
    eSqlRet_success = 0,
    eSqlRet_empty = 1,
    eSqlRet_data = 2,
    eSqlRet_error = 3,
    eSqlRet_cnt
};
class XMysqlHelper {
public:
	XMysqlHelper(){
		
	}
	~XMysqlHelper(){
		
	}
	bool connect(const std::string &strIp, uint16_t nPort, const std::string &strUser
		, const std::string &strPw, const std::string &strDB, const std::string strCharacterset = "") {
		bool bRet = false;
		do 
		{
			m_pMysql = mysql_init(nullptr);
			if (!m_pMysql) {
				_errCallback("mysql_init error");
				break;
			}
			if (strIp.empty() || nPort < 0) {
				_errCallback("ip or port error");
				break;
			}
            auto bConnect = 1;
            mysql_options(m_pMysql, MYSQL_OPT_RECONNECT, &bConnect);
			if (!strCharacterset.empty()) {
				if (mysql_set_character_set(m_pMysql, strCharacterset.c_str()) != 0) {
					_errCallback("mysql_set_character_set error");
					break;
				}
			}
			m_pMysql = mysql_real_connect(m_pMysql, strIp.c_str(), strUser.c_str(), strPw.c_str(), strDB.c_str(), nPort, nullptr, 0);
			if (!m_pMysql) {
				_errCallback("mysql_real_connect error");
				break;
			}
			bRet = true;
		} while (0);
		return bRet;
	}

	void close() {
		if (m_pMysql) {
			mysql_close(m_pMysql);
			m_pMysql = nullptr;
		}
	}

	void setErrorCallback(const std::function<void(const std::string&)>& errCallBack) {
		m_pErrFunc = errCallBack;
	}

    SqlRetCode exectue(const std::string &strSql){
        SqlRetCode eRet = eSqlRet_error;
        do{
            if(!query(strSql)){
                _sqlErrMsg(strSql);
                break;
            }
            else{
                m_nAffect = mysql_affected_rows(m_pMysql);
                eRet = m_nAffect == 0 ? eSqlRet_error : eSqlRet_success;
            }
        }while(0);
        return eRet;
    }

    SqlRetCode query(const std::string &strQuery){
        SqlRetCode eRet = eSqlRet_error;
        do{
            if(mysql_query(m_pMysql,strQuery.c_str()) != 0){
                _sqlErrMsg(strQuery);
                break;
            }
            if(m_pRes){
                mysql_free_result(m_pRes);
                m_pRes = nullptr;
            }
            m_pRes = ::mysql_store_result(m_pMysql);
            if(!m_pRes){
                break;
            }
            if(m_pRes->row_count == 0){
                eRet = eSqlRet_empty;
                break;
            }
            eRet = eSqlRet_data;
        }while(0);
        return eRet;
    }

    template<typename ...TArgs>
    SqlRetCode queryOne(const std::string &strQuery, TArgs &...args){
        SqlRetCode eRet = eSqlRet_error;
        do{
            eRet = query(strQuery);
            if(eRet != eSqlRet_data){
                break;
            }
            if(!getNextRow(args...)){
                eRet = eSqlRet_empty;
                break;
            }
            eRet = eSqlRet_success;
        }while(0);
        return eRet;
    }

    template<typename ...TArgs>
    bool getNextRow(TArgs &...args){
        bool bRet = false;
        do{
            if(!m_pRes){
                break;
            }
            m_pRow = mysql_fetch_row(m_pRes);
            if(m_pRow == nullptr){
                break;
            }
            return fetch(0, args...);
        }while(0);
        return bRet;
    }

    uint64_t getAffectCount() const{
        return m_nAffect;
    }

    uint64_t getFiledCount() const{
        if(!m_pRes){
            return 0;
        }
        return m_pRes->field_count;
    }

    uint64_t getRowCount() const{
        if(!m_pRes){
            return 0;
        }
        return m_pRes->row_count;
    }
private:
    template<typename TValue>
    void _parseValue(const std::string &strFiled, TValue & tValue){
        std::istringstream stream(strFiled);
        stream >> tValue;
    }

    void _parseValue(const std::string& strFiled,std::string& strValue){
        strValue = strFiled;
    }

    void _parseValue(const std::string& strFiled,bool& bValue){
        bValue = (strFiled == "TRUE" || strFiled == "True" || strFiled == "true" || strFiled == "1") ? true : false;
    }

    void _parseValue(const std::string& strFiled,char& szResult){
        int16_t temp = 0;
        _parseValue<int16_t>(strFiled,temp);
        szResult = (char)temp;
    }

    template<typename TValue>
    bool _getFiled(uint32_t nIndex, TValue &tValue){
        bool bRet = false;
        do{
            if(nIndex >= m_pRes->row_count){
                break;
            }
            if(m_pRow[nIndex]){
                _parseValue(m_pRow[nIndex], tValue);
            }
            bRet = true;
        }while(0);
        return bRet;
    }

    template<typename TValue, typename ...TArgs>
    bool _fetch(uint32_t nIndex, TValue &tValue, TArgs &...args){
        if(nIndex >= m_pRes->field_count){
            return true;
        }
        if(!_getFiled(nIndex, tValue)){
            return false;
        }
        return _fetch(nIndex + 1, args...);
    }

    bool _fetch(uint32_t nIndex){
        XSVR_UNUSED(nIndex);
        return true;
    }

	void _sqlErrMsg(const std::string &strSql) {
		std::string strErrMsg;
		const char *pMsg = mysql_error(m_pMysql);
		strErrMsg = "ERROR QUERY:" + strSql + "\n";
		strErrMsg += "ERROR CODE:";
        strErrMsg += std::to_string(mysql_errno(m_pMysql)) + "\n";
		strErrMsg += "ERROR MSG:";
		strErrMsg += pMsg;
		_errCallback(strErrMsg);
	}

	void _errCallback(const std::string &strError) {
		if (m_pErrFunc) {
			m_pErrFunc(strError);
		}
	}

	std::function<void(const std::string&)> m_pErrFunc = nullptr;
	MYSQL* m_pMysql = nullptr;
    uint64_t m_nAffect = 0;
    MYSQL_ROW m_pRow = nullptr;
    MYSQL_RES * m_pRes = nullptr;

};
XSVR_NS_END
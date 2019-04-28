#pragma once
#include "memory"
#include "mysql.h"
#include <string>
#include <functional>
#include <sstream>
#include <mutex>
#include "base/XBase.h"
XSVR_NS_BEGIN
enum SqlRetCode{
	eSqlRet_Error = 1,
	eSqlRet_Data = 2,
	eSqlRet_Empty = 3,
    eSqlRet_Affect = 4, 
	eSqlRet_NoAffect = 5,
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
        static std::mutex mysqlConnMutex;
        std::lock_guard<std::mutex> lck(mysqlConnMutex);
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
        SqlRetCode eRet = eSqlRet_Error;
        do{
            if(!query(strSql)){
                _sqlErrMsg(strSql);
                break;
            }
            else{
                m_nAffect = mysql_affected_rows(m_pMysql);
                eRet = m_nAffect == 0 ? eSqlRet_NoAffect : eSqlRet_Affect;
            }
        }while(0);
        return eRet;
    }

    SqlRetCode query(const std::string &strQuery){
        SqlRetCode eRet = eSqlRet_Error;
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
                eRet = eSqlRet_Empty;
                break;
            }
            eRet = eSqlRet_Data;
        }while(0);
        return eRet;
    }

    template<typename ...TArgs>
    SqlRetCode queryOne(const std::string &strQuery, TArgs &&...args){
        SqlRetCode eRet = eSqlRet_Error;
        do{
            eRet = query(strQuery);
            if(eRet != eSqlRet_Data){
                break;
            }
            if(!getNextRow(std::forward<TArgs>(args)...)){
                eRet = eSqlRet_Empty;
                break;
            }
        }while(0);
        return eRet;
    }

	template<typename ...TArgs>
	bool getNextRow(TArgs && ...args) {
		if (!m_pRes) {
			return false;
		}
		m_pRow = mysql_fetch_row(m_pRes);
		if (m_pRow == nullptr) {
			return false;
		}
		if (sizeof...(args) > getFiledCount()) {
			return false;
		}
		uint32_t nColIndex = 0;
		std::initializer_list<int> {(_parseValue(m_pRow[nColIndex++], std::forward<TArgs>(args)), 0) ... };
		return true;
	}

   
	std::vector<std::vector<std::string>> getAll(const std::string &strQuery, SqlRetCode *eRet = nullptr) {
		std::vector<std::vector<std::string>> vvRet;
		SqlRetCode eRetCode;
		do {
			eRetCode = query(strQuery);
			if (eRetCode != eSqlRet_Data) {
				break;
			}
			std::string strData;
			while (m_pRow = mysql_fetch_row(m_pRes), m_pRow) {
				uint32_t nCol = 0;
				std::vector<std::string> vCol;
				while (nCol < getFiledCount()) {
					strData = std::string(m_pRow[nCol++]);
					vCol.push_back(strData);
				}
				vvRet.push_back(vCol);
			}
		} while (0);
		if (eRet) {
			*eRet = eRetCode;
		}
		return std::forward<std::vector<std::vector<std::string>>>(vvRet);
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
        int8_t temp = 0;
        _parseValue<int8_t>(strFiled,temp);
        szResult = (char)temp;
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
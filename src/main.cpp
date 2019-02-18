#include "common/third_help/XSpdLogHelper.hpp"
#include "logic/XSvrLogic.hpp"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "common/third_help/XEventClient.hpp"
#include "common/third_help/XMysqlHelper.hpp"
#include "common/third_help/XHiRedisHelper.hpp"
#include "signal.h"
#include <sys/stat.h> 
#include "base/XSvrBase.hpp"
int main(int argc, char const *argv[])
{
    LOG_START("XSVR");
    
    XSvr::Singleton<XSvrLogic>::instance()->start();
    XSvr::Singleton<XSvrLogic>::instance()->setLogFunc([&](const std::string &strMsg, XSvrLogLevel eLevel){
        if (eLevel == XSvrLogLevel::eLog_err){
            LOG_E(strMsg);
        }else{
            LOG_I(strMsg);
        }
    });
    while(getchar() != 'q');
    XSvr::Singleton<XSvrLogic>::instance()->stop();
    LOG_STOP();
    return 0;
}

#include "common/third_help/XSpdLogHelper.hpp"
#include "logic/XSvrLogic.hpp"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "common/third_help/XEventClient.hpp"

int main(int argc, char const *argv[])
{
    LOG_START("XSVR");
    LOG_I("start");
    XSvr::Singleton<XSvrLogic>::instance()->start();
    while(getchar() != 'q');
    XSvr::Singleton<XSvrLogic>::instance()->stop();
    LOG_STOP();
    return 0;
}

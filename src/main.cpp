#include <iostream>
#include <memory>
#include "third/fmt/fmt.h"
#include "protocol/TestMsg.pb.h"
#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "third_help/XEventServer.hpp"
#include "third_help/XProtobufDispather.hpp"
#include "folly/ProducerConsumerQueue.h"

static auto logger = spdlog::stderr_color_mt("console");
class ProtoTest
{
  public:
    void onTest(std::shared_ptr<TestMsg2> pMsg)
    {
        logger->info(pMsg->str());
    }
};

void dispath(XSvr::XEvClientPtr pClient, const char *pData, uint32_t nLen)
{
    logger->info(fmt::format("id:{}, len:{}", pClient->id(), nLen));
    int32_t nMsgTypeSize = 0;
    memcpy(&nMsgTypeSize, pData, sizeof(int32_t));
    std::string strMsgType(pData + sizeof(int32_t), pData + sizeof(int32_t) + nMsgTypeSize);
    logger->info(strMsgType);
    std::string strData(pData + sizeof(int32_t) + nMsgTypeSize, pData + nLen);
    XSvr::Singleton<XSvr::ProtoBufDispather>::instance()->onMsgCallBack(strMsgType, strData);
}
int main(int argc, char const *argv[])
{
    spdlog::set_pattern("[%Y-%m-%d %T][thread %t][%l]%v");
    auto pTest = std::make_shared<ProtoTest>();
    auto pDispather = XSvr::Singleton<XSvr::ProtoBufDispather>::instance();
    pDispather->registerMsgCallBack<TestMsg2>(XSVR_BIND_1(&ProtoTest::onTest, pTest));
    XSvr::XEventServer svr;
    svr.initEnvironment();
    svr.setCallback(&dispath,
                    [](const std::string &strLog) { logger->error(strLog); }, [](const std::string &strLog) { logger->info(strLog); });
    svr.listen("", 8888);
    logger->info("next");
    getchar();
    return 0;
}

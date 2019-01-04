#pragma once
#include "common/utility/XNoCopyable.hpp"
#include "common/utility/XSingleton.hpp"
#include "common/third_help/XEventServer.hpp"
#include "third_help/XProtobufDispather.hpp"
#include "common/third_help/XSpdLogHelper.hpp"
#include "base/XSvrBase.hpp"
#include "utility/XMsgQueue.hpp"
#include "protocol/TestMsg.pb.h"
class MsgItem
{
public:
  enum MsgType
  {
    eMsgType_ProtoBuf,
    eMsgType_Quit,
  };
  MsgType eMsgType = eMsgType_ProtoBuf;
  std::shared_ptr<google::protobuf::Message> pMsg = nullptr;
  XSvr::XEvClientPtr pClient = nullptr;
};

class XSvrLogic : public XSvr::XSvrBase
{
  ~XSvrLogic()
  {
  }
  friend XSvr::Singleton<XSvrLogic>;

protected:
  virtual bool _onSvrInit() override
  {
    try
    {
      XSVR_INIT(initThreads, eInitType_thread);
      XSVR_INIT(initProtobuf, eInitType_proto);
      XSVR_INIT(initListen, eInitType_listen);
    }
    catch (XSvrInitType eType)
    {
      LOG_E(fmt::format("XSVR INIT ERROR CODE:{}", (int32_t)eType));
    }
    catch (std::exception &e)
    {
      LOG_E(e.what());
    }
    catch (...)
    {
      LOG_E("UNKOWN ERROR");
    }
    return true;
  }

  virtual bool _onSvrStop() override
  {
    //todo 退出有问题
    for (uint32_t i = 0; i < m_vThreadPool.size(); i++)
    {
      MsgItem stItem;
      stItem.eMsgType = MsgItem::eMsgType_Quit;
      m_msgQueue.push(stItem);
    }
    
    for (auto &vIter : m_vThreadPool)
    {
      vIter->join();
    }
    m_pListen->close();
    LOG_I("EXIT");
    return true;
  }

  bool initThreads()
  {
    //还没有封装读取配置的和配置有关的暂时都写死
    for (uint32_t i = 0; i < 4; i++)
    {
      auto pMsgThread = std::make_shared<std::thread>([this]() {
        run();
      });
      m_vThreadPool.push_back(pMsgThread);
    }
    return true;
  }

  bool initProtobuf()
  {
#define PROTOBUF_DISPATH(_PROTO_, _FUNC_, _OBJ_) \
  XSvr::Singleton<XSvr::ProtoBufDispather<XSvr::XEventClient>>::instance()->registerMsgCallBack<_PROTO_>(XSVR_BIND_2(_FUNC_, _OBJ_))
    //这里添加协议的回调函数
    PROTOBUF_DISPATH(TestMsg2, &XSvrLogic::onTest, this);
    return true;
  }

  void run()
  {
    while (true)
    {
      //std::this_thread::sleep_for(std::chrono::seconds(5));

      MsgItem tempMsg;
      if (m_msgQueue.get(tempMsg))
      {
        switch (tempMsg.eMsgType)
        {
        case MsgItem::eMsgType_ProtoBuf:
          XSvr::Singleton<XSvr::ProtoBufDispather<XSvr::XEventClient>>::instance()->onMsgCallBack(tempMsg.pMsg, tempMsg.pClient);
          break;
        case MsgItem::eMsgType_Quit:
        LOG_I("QUIT");
          return;
        }
      }
      else
      {
        LOG_E("MSG QUEUR GET FAILED");
      }
    }
  }

  void dispath(XSvr::XEvClientPtr pClient, const char *pData, uint32_t nLen)
  {
    int32_t nMsgTypeSize = 0;
    memcpy(&nMsgTypeSize, pData, sizeof(int32_t));
    std::string strMsgType(pData + sizeof(int32_t), pData + sizeof(int32_t) + nMsgTypeSize);
    LOG_I(strMsgType);
    std::string strData(pData + sizeof(int32_t) + nMsgTypeSize, pData + nLen);
    auto pMsg = XSvr::Singleton<XSvr::ProtoBufDispather<XSvr::XEventClient>>::instance()->CreateMsg(strMsgType);
    pMsg->ParseFromString(std::move(strData));
    MsgItem stItem;
    stItem.eMsgType = MsgItem::eMsgType_ProtoBuf;
    stItem.pMsg = pMsg;
    stItem.pClient = pClient;
    m_msgQueue.push(stItem);
  }

  bool initListen()
  {
    m_pListen = std::make_shared<XSvr::XEventServer>();
    m_pListen->initEnvironment();
    m_pNetCb = std::make_shared<XSvr::XNetSvrCallBack<XSvr::XEventClient>>();
    m_pNetCb->setCallBack(
        [](const std::string &strMsg) { LOG_I("[SOCKET]" + std::move(strMsg)); },
        [](const std::string &strError) { LOG_E("[SOCKET]" + std::move(strError)); },
        XSVR_BIND_3(&XSvrLogic::dispath, this),
        [](XSvr::NetState eState, XSvr::XEvClientPtr pClient) {
          LOG_I(fmt::format("[CLIENT][{}]:state:{}", pClient->ip(), eState));
        });
    m_pListen->setNetCallBack(m_pNetCb);
    m_pListen->listen("", 8888);
    return true;
  }

  void onTest(std::shared_ptr<TestMsg2> pMsg, XSvr::XEvClientPtr pClient)
  {
    LOG_I(pMsg->str());
  }

protected:
  std::shared_ptr<XSvr::XEventServer> m_pListen = nullptr;
  std::shared_ptr<XSvr::XNetSvrCallBack<XSvr::XEventClient>> m_pNetCb = nullptr;
  std::vector<std::shared_ptr<std::thread>> m_vThreadPool;
  XSvr::XMsgQueue<MsgItem> m_msgQueue;
};
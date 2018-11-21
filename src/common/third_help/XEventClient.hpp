#pragma once
extern "C"
{
#include "event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/thread.h"
#include "event2/buffer.h"
#include "event2/util.h"
}
#include <string>
#include <memory>
#include "base/XBase.h"
//#include "third_help/XEventServer.hpp"
XSVR_NS_BEGIN
class XEventServer;
class XEventClient
{
public:
  XEventClient() {}
  ~XEventClient() {}
  bool connnectToHost(const std::string &strIP, uint16_t nPort)
  {
    return true;
  }

  bool close()
  {
    return true;
  }

  int32_t send(const std::string &strData)
  {
    return 0;
  }

  std::string ip() const
  {
    return "";
  }

  uint16_t port() const
  {
    return 0;
  }

  uint32_t id() const
  {
    return m_nID;
  }

  friend XEventServer;

private:
  std::shared_ptr<bufferevent> m_pBufferEv;
  uint32_t m_nID = 0;
};
XSVR_NS_END
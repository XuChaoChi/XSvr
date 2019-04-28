#include "gtest/gtest.h"
#include "common/utility/XMsgQueue.hpp"

TEST(XMsgQueue,push)
{
    XSvr::XMsgQueue<int> tQueue;
    ASSERT_TRUE(tQueue.push(1));  
}

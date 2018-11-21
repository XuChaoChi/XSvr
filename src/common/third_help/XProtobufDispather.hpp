#pragma once
#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"
#include "utility/XSingleton.hpp"

XSVR_NS_BEGIN
using PbMsgPtr = std::shared_ptr<google::protobuf::Message>;
class Callback
{
public:
	virtual ~Callback() {};
	virtual void onMessage(PbMsgPtr pMsgReq) const = 0;
};

template <typename TReq>
class CallBackT :public Callback {	
public:
	using CallBackFuncT = std::function<void(std::shared_ptr<TReq>)>;
	CallBackT(CallBackFuncT pCb) :
		m_cbFunc(pCb) {
	}
	virtual void onMessage(PbMsgPtr pReq) const
	{
		std::shared_ptr<TReq> msgReq = std::dynamic_pointer_cast<TReq>(pReq);
		m_cbFunc(msgReq);
	}
protected:
	CallBackFuncT m_cbFunc;
};

class ProtoBufDispather {
public:
	template <typename TReq>
	void registerMsgCallBack(const typename CallBackT<TReq>::CallBackFuncT & cbFunc) {
		std::shared_ptr<CallBackT<TReq> > pCb(new CallBackT<TReq>(cbFunc));
		m_cbMap[TReq::descriptor()] = pCb;
	}
	void onMsgCallBack(const PbMsgPtr pMsg) const{
		if (pMsg) {
			auto pIter = m_cbMap.find(pMsg->GetDescriptor());
			if (pIter != m_cbMap.end()) {
				pIter->second->onMessage(pMsg);
			}
		}	
	}
	void onMsgCallBack(const std::string &strTypeName, const std::string &strBuf) const{
		auto pMsg = CreateMsg(strTypeName);
		if (pMsg) {
			pMsg->ParseFromString(strBuf);
			onMsgCallBack(pMsg);
		}
	}
	static PbMsgPtr CreateMsg(const std::string &strTypeName) {
		PbMsgPtr pPbMsg = nullptr;
		const google::protobuf::Descriptor* pDescriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(strTypeName);
		if (pDescriptor)
		{
			const google::protobuf::Message* protoType = google::protobuf::MessageFactory::generated_factory()->GetPrototype(pDescriptor);
			if (protoType)
			{
				PbMsgPtr pTemp(protoType->New());
				pPbMsg.swap(pTemp);
			}
		}
		return pPbMsg;
	}
	friend Singleton<ProtoBufDispather>;
protected:
	std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback>> m_cbMap;
};
XSVR_NS_END
#pragma once
#include "base/XBase.h"
#include "utility/XSingleton.hpp"
#include "spdlog/details/file_helper.h"
#include "spdlog/details/null_mutex.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <ctime>
#include <mutex>
#include <string>
#include <chrono>
#include <time.h>
#include <iomanip>
#include <sstream>
#include <sys/io.h>
#include <sys/stat.h>
#include <unistd.h>
XSVR_NS_BEGIN
template <typename Mutex>
class XTimeSink final : public spdlog::sinks::base_sink<Mutex>
{
  public:
	XTimeSink(const std::string &strBaseName, uint32_t nMaxSize) : m_strBaseFileName(std::move(strBaseName)),
																   m_nMaxSize(nMaxSize)
	{
		m_fileHelper.open(calcBaseFileName_());
		m_nCurSize = m_fileHelper.size();
	}

  private:
	std::string calcBaseFileName_() const
	{
		auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::stringstream strStream;
		strStream << std::put_time(std::localtime(&t), "%Y-%m-%d-%H-%M-%S");
		std::string strFullCurPath = getcwd(NULL, 0);
		strFullCurPath += m_strFilePath;
		if (access(strFullCurPath.c_str(), 0) != 0)
		{
			if (0 != mkdir(strFullCurPath.c_str(), 0755))
			{
				printf("errno=%d\n", errno);
			}
		}

		uint32_t nIndex = 0;
		std::string strRet;
		while (true)
		{
			strRet = fmt::format("{}{}_{}_{}{}", strFullCurPath, m_strBaseFileName, strStream.str(), nIndex, m_strSuffix);
			if (!spdlog::details::file_helper::file_exists(strRet))
			{
				if (nIndex == 0)
				{
					auto beginIter = strRet.begin() + (strRet.size() - m_strSuffix.size() - 2);
					strRet.erase(beginIter, beginIter + 2);
				}
				break;
			}
			nIndex++;
		}
		return strRet;
	}

	virtual void sink_it_(const spdlog::details::log_msg &msg) override
	{
		fmt::memory_buffer formatted;
		spdlog::sinks::sink::formatter_->format(msg, formatted);
		m_nCurSize += formatted.size();
		if (m_nCurSize > m_nMaxSize)
		{
			newFile_();
			m_nCurSize = formatted.size();
		}
		m_fileHelper.write(formatted);
	}

	virtual void flush_() override
	{
		m_fileHelper.flush();
	}

	void newFile_()
	{
		using spdlog::details::os::filename_to_str;
		m_fileHelper.close();
		m_fileHelper.open(calcBaseFileName_(), true);
	}
	const std::string m_strSuffix = ".log";
	const std::string m_strFilePath = "/log/";
	std::string m_strBaseFileName;
	uint32_t m_nMaxSize;
	uint32_t m_nCurSize;
	spdlog::details::file_helper m_fileHelper;
};

using XTimeSinkMt = XTimeSink<std::mutex>;
using XTimeSinkSt = XTimeSink<spdlog::details::null_mutex>;

class XSpdLogHelper
{
  public:
	friend Singleton<XSpdLogHelper>;

	void start(const std::string &strBaseName, spdlog::level::level_enum eLevel, uint32_t nMaxSize)
	{
		auto pFileSink = std::make_shared<XTimeSinkMt>(strBaseName, nMaxSize);
		auto pStdCout = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		spdlog::sinks_init_list vSinks{pStdCout, pFileSink};
		m_pLog = std::make_shared<spdlog::logger>(strBaseName, vSinks);
		spdlog::register_logger(m_pLog);
		spdlog::set_pattern("[%C-%m-%d %H:%M:%S.%e][%t][%L] %v");
		spdlog::set_level(eLevel);
		spdlog::flush_on(spdlog::level::info);
	}

	void setPattern(const std::string &strPattren)
	{
		spdlog::set_pattern(strPattren);
	}

	void setLevel(spdlog::level::level_enum eLevel)
	{
		spdlog::set_level(eLevel);
	}

	void setFlushon(spdlog::level::level_enum eLevel)
	{
		spdlog::flush_on(eLevel);
	}

	void log(spdlog::level::level_enum eLevel, const std::string &strLog)
	{
		m_pLog->log(eLevel, strLog);
	}

	void stop()
	{
		spdlog::drop_all();
		m_pLog = nullptr;
	}

  private:
	std::shared_ptr<spdlog::logger> m_pLog = nullptr;
};

#define LOG_START(_FILE_NAME_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->start(_FILE_NAME_, spdlog::level::level_enum::info, XSVR_1M * 50);
#define LOG_START_DETAIL(_FILE_NAME_, _LEVEL_, _MAX_SIZE_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->start(_FILE_NAME_, _LEVEL_, _MAX_SIZE_)
#define LOG_STOP() XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->stop()
#define LOG_I(_MSG_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->log(spdlog::level::level_enum::info, _MSG_)
#define LOG_E(_MSG_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->log(spdlog::level::level_enum::err, _MSG_)
#define LOG_W(_MSG_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->log(spdlog::level::level_enum::warn, _MSG_)
#define LOG_C(_MSG_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->log(spdlog::level::level_enum::critical, _MSG_)
#define LOG_D(_MSG_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->log(spdlog::level::level_enum::debug, _MSG_)
#define LOG_T(_MSG_) XSvr::Singleton<XSvr::XSpdLogHelper>::instance()->log(spdlog::level::level_enum::trace, _MSG_)

XSVR_NS_END
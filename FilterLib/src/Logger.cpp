#include "FilterLib/FilterUtils.h"
#include "FilterLib/Logger.h"

#include <iostream>
#include <format>

#include <direct.h>

LoggerLock::LoggerLock(Logger* logger, LogLevel::LogLevel logLevel) : pLogger(logger), lock(logger->mutex), logLevel(logLevel)
{
}

LoggerStream::LoggerStream(LogLevel::LogLevel minimumLogLevel, std::string fileName) : minimumLogLevel(minimumLogLevel), fstream(fileName, std::ios::app), pStream(&fstream)
{
}

void LoggerLock::Write(std::string_view str)
{
	for (auto& s : pLogger->streams)
	{
		if (logLevel >= s.minimumLogLevel)
		{
			s.Stream() << str;
			s.Flush();
		}
	}
}

void LoggerLock::WriteLine()
{
	for (auto& s : pLogger->streams)
	{
		if (logLevel >= s.minimumLogLevel)
		{
			s.Stream() << std::endl;
			s.Flush();
		}
	}
}

const char* GetLogLevelString(LogLevel::LogLevel logLevel)
{
	switch (logLevel)
	{
	case LogLevel::Info:
		return "Info";
	case LogLevel::Status:
		return "Status";
	case LogLevel::Warning:
		return "Warning";
	case LogLevel::Error:
		return "Error";
	default:
		return "";
	}
}

void LoggerLock::WriteTimestamp()
{
	Write(std::format("{} [{}]  ", FilterUtils::FormatDateTime(FilterUtils::DateTimeFormat::Milliseconds, false), GetLogLevelString(logLevel)));
}

void LoggerLock::WriteLine(std::string_view str)
{
	Write(str);
	WriteLine();
}

Logger::Logger() : minimumLogLevel(LogLevel::Error), fileLogCount(0)
{
	InitBase();
}

Logger::Logger(std::span<LoggerStreamInfo> streamInfos) : minimumLogLevel(LogLevel::Error), fileLogCount(0)
{
	InitBase();

	for (const auto& si : streamInfos)
		AddStream(si);
}

Logger::Logger(std::initializer_list<LoggerStreamInfo> streamInfos) : minimumLogLevel(LogLevel::Error), fileLogCount(0)
{
	InitBase();

	for (const auto& si : streamInfos)
		AddStream(si);
}

void Logger::InitBase()
{
	auto success = _mkdir("Logs");
}

void Logger::AddStream(const LoggerStreamInfo& si)
{
	if (si.streamType == LoggerStreamInfo::StreamType::ConsoleOut)
		streams.emplace_back(si.minimumLogLevel, std::cout);
	else if (si.streamType == LoggerStreamInfo::StreamType::ConsoleErr)
		streams.emplace_back(si.minimumLogLevel, std::cerr);
	else if (si.streamType == LoggerStreamInfo::StreamType::File)
	{
		if (fileLogCount == 0)
			streams.emplace_back(si.minimumLogLevel, std::format("Logs/Log {}.txt", FilterUtils::FormatDateTime(FilterUtils::DateTimeFormat::Hours, true)));
		else
			streams.emplace_back(si.minimumLogLevel, std::format("Logs/Log {} {}.txt", FilterUtils::FormatDateTime(FilterUtils::DateTimeFormat::Hours, true), fileLogCount));
		++fileLogCount;
	}

	if (si.minimumLogLevel < minimumLogLevel)
		minimumLogLevel = si.minimumLogLevel;
}

void Logger::AddStreams(std::span<LoggerStreamInfo> streamInfos)
{
	for (const auto& si : streamInfos)
		AddStream(si);
}

bool Logger::IsIgnored(LogLevel::LogLevel logLevel) const
{
	return logLevel < minimumLogLevel;
}

LoggerLock Logger::Lock(LogLevel::LogLevel logLevel)
{
	return LoggerLock(this, logLevel);
}

void Logger::WriteSingleLine(LogLevel::LogLevel logLevel, std::string_view str)
{
	auto lock = Lock(logLevel);
	lock.WriteTimestamp();
	lock.WriteLine(str);
}

bool Logger::ParseCommandLineArgs(int argc, const char** argv)
{
	std::vector<std::string_view> options { "Info", "Status", "Warning", "Error", "None" };
	intptr_t index;
	if (!FilterUtils::ParseCommandLineOption(argc, argv, "log-to-console", options, index, 1))
		return false;

	if (index != 4)
	{
		LogLevel::LogLevel targetLevel;
		switch (index)
		{
		case 0:
			targetLevel = LogLevel::Info;
			break;
		case 1:
			targetLevel = LogLevel::Status;
			break;
		case 2:
			targetLevel = LogLevel::Warning;
			break;
		case 3:
			targetLevel = LogLevel::Error;
			break;
		default:
			return false;
		}

		AddStream(LoggerStreamInfo(targetLevel, LoggerStreamInfo::StreamType::ConsoleOut));
	}

	intptr_t fileIndex;
	if (!FilterUtils::ParseCommandLineOption(argc, argv, "log-to-file", options, fileIndex, 1))
		return false;

	if (fileIndex != 4)
	{
		LogLevel::LogLevel targetLevel;
		switch (fileIndex)
		{
		case 0:
			targetLevel = LogLevel::Info;
			break;
		case 1:
			targetLevel = LogLevel::Status;
			break;
		case 2:
			targetLevel = LogLevel::Warning;
			break;
		case 3:
			targetLevel = LogLevel::Error;
			break;
		default:
			return false;
		}

		AddStream(LoggerStreamInfo(targetLevel, LoggerStreamInfo::StreamType::File));
	}

	return true;
}
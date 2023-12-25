#pragma once

#include <mutex>
#include <vector>
#include <optional>
#include <fstream>
#include <span>

namespace LogLevel
{
	enum LogLevel
	{
		Info,
		Status,
		Warning,
		Error
	};
}

class Logger;

class LoggerLock
{
private:
	friend class Logger;

	Logger* pLogger;
	std::unique_lock<std::mutex> lock;
	LogLevel::LogLevel logLevel;

	LoggerLock(Logger* logger, LogLevel::LogLevel logLevel);
public:
	LoggerLock(const LoggerLock&) = delete;
	LoggerLock(LoggerLock&&) = default;
	LoggerLock& operator=(const LoggerLock&) = delete;
	LoggerLock& operator=(LoggerLock&&) = default;

	void Write(std::string_view);
	void WriteTimestamp();
	void WriteLine();
	void WriteLine(std::string_view);
};

class LoggerStream
{
private:
	friend class Logger;
	friend class LoggerLock;

	LogLevel::LogLevel minimumLogLevel;
	std::ofstream fstream;
	std::ostream* pStream;

public:
	LoggerStream(LogLevel::LogLevel minimumLogLevel, std::ostream& streamRef) : minimumLogLevel(minimumLogLevel), pStream(&streamRef) {}
	LoggerStream(LogLevel::LogLevel minimumLogLevel, std::string fileName);

	LoggerStream(const LoggerStream&) = delete;
	LoggerStream(LoggerStream&&) = default;
	LoggerStream& operator=(const LoggerStream&) = delete;
	LoggerStream& operator=(LoggerStream&&) = default;

	std::ostream& Stream() { return *pStream; }
	void Flush() { if (pStream == nullptr) fstream.flush(); }
};

class LoggerStreamInfo
{
public:
	enum class StreamType
	{
		ConsoleOut,
		ConsoleErr,
		File,
	};

private:
	friend class Logger;

	LogLevel::LogLevel minimumLogLevel;
	StreamType streamType;

public:
	LoggerStreamInfo(LogLevel::LogLevel minimumLogLevel, StreamType streamType) : minimumLogLevel(minimumLogLevel), streamType(streamType) {}

	LoggerStreamInfo(const LoggerStreamInfo&) = default;
	LoggerStreamInfo(LoggerStreamInfo&&) = default;
	LoggerStreamInfo& operator=(const LoggerStreamInfo&) = default;
	LoggerStreamInfo& operator=(LoggerStreamInfo&&) = default;
};

class Logger
{
private:
	friend class LoggerLock;

	std::mutex mutex;

	std::vector<LoggerStream> streams;

	size_t fileLogCount;
	LogLevel::LogLevel minimumLogLevel;

	void InitBase();
public:
	Logger();
	Logger(std::span<LoggerStreamInfo> streamInfos);
	Logger(std::initializer_list<LoggerStreamInfo> streamInfos);

	bool ParseCommandLineArgs(int argc, const char** argv);

	void AddStream(const LoggerStreamInfo& streamInfo);
	void AddStreams(std::span<LoggerStreamInfo> streamInfos);

	bool IsIgnored(LogLevel::LogLevel logLevel) const;

	LoggerLock Lock(LogLevel::LogLevel logLevel);

	void WriteSingleLine(LogLevel::LogLevel logLevel, std::string_view);
};

extern Logger Log;
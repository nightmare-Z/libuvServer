#ifndef __LOGGER_H__
#define __LOGGER_H__

enum
{
	LOGDEBUG = 0,
	LOGWARNING,
	LOGERROR,
};

#define logDebug(msg,...) Logger::log(__FILE__,__LINE__,LOGDEBUG,msg,## __VA_ARGS__);
#define logWarning(msg,...) Logger::log(__FILE__,__LINE__,LOGWARNING,msg,## __VA_ARGS__);
#define logError(msg,...) Logger::log(__FILE__,__LINE__,LOGERROR,msg,## __VA_ARGS__);


class Logger
{
public:
	//path 打印的路劲  prefix 前缀  std_output 1 控制台+日志  0 日志
	static void init(const char* path, const char* prefix, bool std_output = false);
	//不定参数已经在msgLog开始
	static void log(const char* fileName, int fileLine, int logLevel, const char* msgLog, ...);
};

#endif
#ifndef __TIME_H__
#define __TIME_H__

#include <fcntl.h>
#include <time.h>
#include <string>
#include "uv.h"

static uint32_t gCurrrentDay;
static uint32_t gLastSecond;
static char gFormatTime[64] = { 0 };

struct timer
{
	uv_timer_t uvTimer;
	void(*onTimer)(void* uData);
	void* uData;
	//-1 == 一直循环  
	int repeatCount;
};

#ifdef __cplusplus
extern "C" {
#endif // _cplusplus

	//创建一个计时器
	struct timer* scheduleRepeat(
		//计时器结束的时候调用的函数		data是用户自定义参数
		void(*onTimer)(void* uData),
		void* uData,
		//多少秒后开始执行这个计时器
		int afterMsec,
		//执行多少次销毁这个计时器   -1表示一直执行
		int repeatCount,
		//每执行一次之后 间隔过久再次执行
		int repeatmsec
	);

	//释放一个计时器
	void cancelTimer(timer* t);

	//创建一个单次计时器
	struct timer* scheduleOnce(
		void(*onTimer)(void* uData),
		void* uData,
		int afterMsec
	);
	void* get_timer_udata(struct timer* t);
	//得到当前时间戳
	unsigned long timeStamp();

	//日期转成时间戳
	unsigned long dateToTimeStamp(const char* formatDate, const char* date);

	//时间戳转日期
	void timeStampToDate(unsigned long t, const char* formatDate,char* outBuf,int buffLen);

	//想要哪天的时间戳自己转
	unsigned long timeStampToday();

#ifdef __cplusplus
}
#endif // _cplusplus

#endif
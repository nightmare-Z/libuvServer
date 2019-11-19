#include "time.h"

struct timer* allcoTimer(void(*onTimer)(void* uData), void* uData, int repeatCount)
{
	timer* t = (timer*)malloc(sizeof(struct timer));
	memset(t, 0, sizeof(t));
	t->onTimer = onTimer;
	t->uData = uData;
	t->repeatCount = repeatCount;
	uv_timer_init(uv_default_loop(), &t->uvTimer);

	return t;
}

void freeTimer(timer* t)
{
	free(t);
}

//计时处理
void onUvTimer(uv_timer_t* handle)
{
	timer* t =(timer*)handle->data;

	//不断触发
	if (t->repeatCount < 0)t->onTimer(t->uData);
	else
	{
		t->repeatCount--;
		t->onTimer(t->uData);
		if (t->repeatCount == 0)
		{
			uv_timer_stop(&t->uvTimer);
			freeTimer(t);
		}
	}
}

timer* scheduleRepeat(void(*onTimer)(void* uData), void* uData, int afterMsec, int repeatCount, int repeatmsec)
{
	timer* t = allcoTimer(onTimer, uData, repeatCount);

	t->uvTimer.data = t;

	uv_timer_start(&t->uvTimer, onUvTimer, afterMsec,repeatmsec);

	return t;
}

void cancelTimer(timer* t)
{
	//被uv自动释放了
	if (t->repeatCount == 0)return;
	uv_timer_stop(&t->uvTimer);
	freeTimer(t);
}

timer* scheduleOnce(void(*onTimer)(void* uData), void* uData, int afterMsec)
{
	return scheduleRepeat(onTimer, uData, afterMsec, 1,0);
}

void* get_timer_udata(struct timer* t) {
	return t->uData;
}

unsigned long timeStamp()
{
	time_t t;
	t = time(NULL);
	return (unsigned long)t;
}

unsigned long dateToTimeStamp(const char* formatDate, const char* date)
{
	tm tempTime;
	memset(&tempTime, 0, sizeof(tempTime));

	//linux
	//my_strptime(date, formatDate, &tempTime);
	//wnd
	time_t t = mktime(&tempTime);
	return (unsigned long)t;
}

void timeStampToDate(unsigned long t, const char* formatDate,char* outBuf, int buffLen)
{
	time_t tTemp = t;
	tm* date = localtime(&tTemp);
	strftime(outBuf, buffLen, formatDate, date);
}

unsigned long timeStampToday()
{
	time_t now = time(NULL);
	tm* date = localtime(&now);

	date->tm_sec = 0;
	date->tm_min = 0;
	date->tm_hour = 0;

	return (unsigned long)mktime(date);
}
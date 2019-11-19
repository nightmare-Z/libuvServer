#ifndef __REDISMANAGER_H__
#define __REDISMANAGER_H__

#include "package/hiredis/hiredis.h"

class redisManager {
public:
	static void connect(char* ip, int port,
		void(*open_cb)(const char* err, void* context, void* uData), void* uData = 0);

	static void closeRedis(void* context);

	static void query(void* context,char* cmd,
		void(*query_cb)(const char* err, redisReply* result,void* uData),void* uData =0);
};


#endif

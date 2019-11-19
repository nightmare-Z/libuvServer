#ifndef __MYSQLMANAGER_H__
#define __MYSQLMANAGER_H__

#include "package/mysql/mysql/jdbc.h"

class mysqlManager {
public:
	static void connect(char* ip, int port,char* db_name, char* uname, char* pwd,
		void(*open_cb)(const char* err, void* context,void* uData),void* uData = 0);

	static void close(void* context);

	static void query(void* context,char* sql,
		void(*query_cb)(const char* err,sql::ResultSet* result, void* uData), void* uData = 0);
};


#endif
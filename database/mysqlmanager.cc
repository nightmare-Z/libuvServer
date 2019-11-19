#include "uv.h"
#include "mysqlmanager.h"
#include "utils/Cache.h"
using namespace sql;

#define my_malloc smallCacheAllocer
#define my_free smallCacheFree
#define mystrdup strdupCache

struct connect_req_m {
	char* ip;
	int port;

	char* db_name;

	char* uname;
	char* upwd;

	void (*open_cb)(const char* err, void* context, void* uData);
	void* data;
	char* err;
	void* context;
};

struct mysql_context {
	void* pConn; // mysql
	uv_mutex_t lock;
	char* sqlState;
	char* use_db;
	int is_closed;
};

static void
on_connect_complete(uv_work_t* req, int status) {
	connect_req_m* r = (connect_req_m*)req->data;
	r->open_cb(r->err, r->context, r->data);

	if (r->context == NULL)
	{
		my_free(r);
		my_free(req);
	}
	else
	{
		my_free(r->ip);
		my_free(r->uname);
		my_free(r->upwd);
		my_free(r->err);
		my_free(req);
	}
}


static void
connect_work(uv_work_t* req) {
	connect_req_m* r = (connect_req_m*)req->data;
	mysql::MySQL_Driver* driver = mysql::get_driver_instance();
	Connection* conn=NULL;
	try
	{
		Connection* conn = driver->connect(r->ip, r->uname, r->upwd);
		if (conn != NULL) { // 
			mysql_context* c = (mysql_context*)my_malloc(sizeof(mysql_context));
			memset(c, 0, sizeof(mysql_context));

			c->pConn = conn;
			c->use_db = r->db_name;
			uv_mutex_init(&c->lock);

			r->context = c;
			r->err = NULL;
			uv_mutex_init(&c->lock);
			driver->threadEnd();
			driver->~MySQL_Driver();
		}
	}
	catch(SQLException& e)
	{
		r->context = NULL;
		r->err = (char*)e.what();
		driver->threadEnd();
	}
}

void 
mysqlManager::connect(char* ip, int port,
	char* db_name, char* uname, char* pwd,
	void(*open_cb)(const char* err, void* context, void* uData),void* uData) {
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	connect_req_m* r = (connect_req_m*)my_malloc(sizeof(connect_req_m));
	memset(r, 0, sizeof(connect_req_m));

	r->ip = mystrdup(ip);
	r->port = port;
	r->db_name = mystrdup(db_name);
	r->uname = mystrdup(uname);
	r->upwd = mystrdup(pwd);
	r->open_cb = open_cb;
	r->data = uData;
	w->data = (void*)r;
	uv_queue_work(uv_default_loop(), w, connect_work, on_connect_complete);
}

static void
close_work(uv_work_t* req) {
	mysql_context* r = (mysql_context*)(req->data);
	uv_mutex_lock(&r->lock);
	Connection* conn = (Connection*)r->pConn;
	conn->close();
	conn->~Connection();
	uv_mutex_unlock(&r->lock);
}

static void
on_close_complete(uv_work_t* req, int status) {
	mysql_context* r = (mysql_context*)(req->data);
	my_free(r);
	my_free(req);
}

void 
mysqlManager::close(void* context) {
	mysql_context* c = (mysql_context*) context;
	if (c->is_closed) {
		return;
	}

	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	w->data = (context);

	c->is_closed = 1;
	uv_queue_work(uv_default_loop(), w, close_work, on_close_complete);
}

struct query_req_m {
	void* context;
	char* sql;
	char* ues_db;
	char* err;
	ResultSet* result;
	void(*query_cb)(const char* err, ResultSet* result,void* uData);
	void* uData;
};

static void
query_work(uv_work_t* req) {
	query_req_m* r = (query_req_m*)req->data;

	mysql_context* my_conn = (mysql_context*)(r->context);

	uv_mutex_lock(&my_conn->lock);

	Connection* conn = (Connection*)my_conn->pConn;
	ResultSet* result;
	Statement* state;
	try
	{		
		state = conn->createStatement();
		conn->setSchema(r->ues_db);
		result = state->executeQuery(r->sql);
	}
	catch (SQLException& e)
	{
		int ret = e.getErrorCode();
		if (ret != 0)
		{
			char msgerr[256];
			sprintf(msgerr, "%d:%s", e.getErrorCode(), e.what());
			r->err = mystrdup(msgerr);
			r->result = NULL;
			uv_mutex_unlock(&my_conn->lock);
			return;
		}
	}
	r->err = NULL;
	r->result = NULL;
	if (strstr(r->sql, "SELECT")|| strstr(r->sql, "select"))
	{
		r->result = result;
	}

	state->close();
	state->~Statement();
	uv_mutex_unlock(&my_conn->lock);
}

static void
mysql_query_complete(uv_work_t* req, int status) {
	query_req_m* r = (query_req_m*)req->data;
	
	r->query_cb(r->err, r->result,r->uData);

	if (r->sql) {
		my_free(r->sql);
	}

	if (r->err) {
		my_free(r->err);
	}
	if (r->result) {
		delete r->result;
	}
	my_free(r);
	my_free(req);


}

void 
mysqlManager::query(void* context,
                     char* sql,
					 void(*query_cb)(const char* err, ResultSet* result, void* uData),void* uData) {
	mysql_context* c = (mysql_context*) context;
	if (c->is_closed) {
		return;
	}

	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	query_req_m* r = (query_req_m*)my_malloc(sizeof(query_req_m));
	memset(r, 0, sizeof(query_req_m));
	r->context = context;
	r->sql = mystrdup(sql);
	r->query_cb = query_cb;
	r->ues_db = c->use_db;
	r->uData = uData;
	w->data = r;
	uv_queue_work(uv_default_loop(), w, query_work, mysql_query_complete);
}


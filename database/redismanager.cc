#include "uv.h"
#include "utils/logger.h"
#include "redismanager.h"
#include "utils/Cache.h"

#define my_malloc smallCacheAllocer
#define my_free smallCacheFree
#define mystrdup strdupCache


struct connect_req_r {
	char* ip;
	int port;

	void (*open_cb)(const char* err, void* context, void* udata);

	char* err;
	void* context;
	void* udata;
};

struct redis_context {
	void* pConn; // redis
	uv_mutex_t lock;
	int is_closed;
};

static void
connect_work(uv_work_t* req) {
	
	connect_req_r* r = (connect_req_r*)req->data;
	//设置redis链接超时
	timeval timeout = { 5,0 };
	redisContext* redisCon = redisConnectWithTimeout(r->ip, r->port, timeout);
	//连接错误
	if (redisCon->err)
	{
		r->err = mystrdup(redisCon->errstr);
		r->context = NULL;
		redisFree(redisCon);
	}
	else
	{
		redis_context* rc = (redis_context*)my_malloc(sizeof(redis_context));
		memset(rc, 0, sizeof(redis_context));
		rc->pConn = redisCon;
		uv_mutex_init(&rc->lock);
		r->err = NULL;
		r->context = rc;
	}
}

static void
on_connect_complete(uv_work_t* req, int status) {
	connect_req_r* r = (connect_req_r*)req->data;
	r->open_cb(r->err, r->context, r->udata);

	if (r->ip) {
		my_free(r->ip); 
	}
	if (r->err) {
		my_free(r->err);
	}

	my_free(r);
	my_free(req);
}

void 
redisManager::connect(char* ip, int port,
	void(*open_cb)(const char* err, void* context, void* uData), void* uData) {
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	connect_req_r* r = (connect_req_r*)my_malloc(sizeof(connect_req_r));
	memset(r, 0, sizeof(connect_req_r));
	r->ip = mystrdup(ip);
	r->port = port;
	r->open_cb = open_cb;
	r->udata = uData;
	w->data = (void*)r;
	uv_queue_work(uv_default_loop(), w, connect_work, on_connect_complete);
}

static void
close_work(uv_work_t* req) {
	redis_context* r = (redis_context*)(req->data);
	uv_mutex_lock(&r->lock);
	redisContext* redisCon = (redisContext*)r->pConn;
	redisFree(redisCon);
	r->pConn = NULL;
	uv_mutex_unlock(&r->lock);
}

static void
on_close_complete(uv_work_t* req, int status) {
	redis_context* r = (redis_context*)(req->data);
	my_free(r);
	my_free(req);
}

void 
redisManager::closeRedis(void* context) {
	redis_context* c = (redis_context*) context;
	if (c->is_closed) {
		return;
	}

	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	w->data = (context);

	c->is_closed = 1;
	uv_queue_work(uv_default_loop(), w, close_work, on_close_complete);
}

struct query_req {
	void* context;
	char* cmd;
	void(*query_cb)(const char* err, redisReply* result, void* uData);
	char* err;
	redisReply* result;
	void* uData;
};

static void
query_work(uv_work_t* req) {
	query_req* r = (query_req*)req->data;

	redis_context* my_conn = (redis_context*)(r->context);

	redisContext* redisCon = (redisContext*)my_conn->pConn;

	uv_mutex_lock(&my_conn->lock);
	
	redisReply* reply = (redisReply*)redisCommand(redisCon, r->cmd);
	//如果有错 那么redis reply就会把错误保存在str字符串中
	if (reply->type==REDIS_REPLY_ERROR)
	{
		r->err = mystrdup(reply->str);
		r->result = NULL;
		freeReplyObject(reply);
	}
	else
	{
		r->result = reply;
		r->err = NULL;
	}
	uv_mutex_unlock(&my_conn->lock);
}

static void
redis_query_complete(uv_work_t* req, int status) {
	query_req* r = (query_req*)req->data;
	r->query_cb(r->err, r->result,r->uData);

	if (r->cmd) {
		my_free(r->cmd);
	}
	if (r->result) {
		freeReplyObject(r->result);
	}

	if (r->err) {
		my_free(r->err);
	}

	my_free(r);
	my_free(req);
}

void redisManager::query(void* context, char* cmd,
						void(*query_cb)(const char* err, redisReply* result, void* uData), 
						void* uData) {
	redis_context* c = (redis_context*) context;
	if (c->is_closed) {
		return;
	}

	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	query_req* r = (query_req*)my_malloc(sizeof(query_req));
	memset(r, 0, sizeof(query_req));
	r->context = context;
	r->cmd = mystrdup(cmd);
	r->query_cb = query_cb;
	r->uData = uData;

	w->data = r;
	uv_queue_work(uv_default_loop(), w, query_work, redis_query_complete);
}


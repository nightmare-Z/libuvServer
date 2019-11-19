#include <stdlib.h>
#include <string.h>

#include "Cache.h"


//小内存分配器
#define SMALLCACHE				128
#define SMALLCACHENUM			10240

//池链链表
struct cacheNode
{
	cacheNode* next;
};

//池的信息
struct cacheMan
{
	//池大小
	unsigned char* cacheMem;
	//池的链表
	cacheNode* listCache;

	//连接数
	int capacity;

	// 每个连接分配的大小
	int elem_size;
};


static cacheMan* smallCache = NULL;

struct cacheMan* cacheCreator(int capacity, int elem_t)
{
	int i;
	struct cacheMan* mem = (struct cacheMan*)malloc(sizeof(struct cacheMan));
	memset(mem, 0, sizeof(struct cacheMan));
	elem_t = (elem_t < sizeof(cacheNode)) ? sizeof(cacheNode) : elem_t;
	mem->capacity = capacity;
	mem->elem_size = elem_t;
	mem->cacheMem = (unsigned char*)malloc(capacity * elem_t);
	memset(mem->cacheMem, 0, capacity * elem_t);
	mem->listCache = NULL;
	for (i = 0; i < capacity; i++)
	{
		cacheNode* walk = (cacheNode*)(mem->cacheMem + i * elem_t);
		walk->next = mem->listCache;
		mem->listCache = walk;
	}
	return mem;

}


void cacheDestroy(struct cacheMan* cache, int elem_t)
{
	if (cache->cacheMem != NULL)free(cache->cacheMem);
	free(cache);
}

void* cacheAllocer(struct cacheMan* cache, int elem_t)
{
	//池中没有空闲cache就创建一个系统内存
	if (cache->elem_size < elem_t)return malloc(elem_t);

	//分配池中cache出去
	if (cache->listCache != NULL)
	{
		void* now = cache->listCache;
		cache->listCache = cache->listCache->next;
		return now;
	}
	return malloc(elem_t);
}

void cacheFree(struct cacheMan* cache, void* cacheMem)
{
	//如果销毁的时池中cache
	if (((unsigned char*)cacheMem) >= cache->cacheMem &&
		((unsigned char*)cacheMem) < cache->cacheMem + cache->capacity * cache->elem_size) {
		struct cacheNode* node = (struct cacheNode*)cacheMem;
		node->next = cache->listCache;
		cache->listCache = node;
		return;
	}

	free(cacheMem);
}

void* smallCacheAllocer(int elem_t)
{
	if (smallCache == NULL)
		smallCache = cacheCreator(SMALLCACHENUM, SMALLCACHE);
	return cacheAllocer(smallCache,elem_t);
}

void smallCacheFree(void* mem)
{
	if (smallCache == NULL)
		smallCache = cacheCreator(SMALLCACHENUM, SMALLCACHE);
	cacheFree(smallCache, mem);
}

char* strdupCache(char* str)
{
	char* Str = (char*)smallCacheAllocer(strlen(str)+2);
	memmove(Str, str, strlen(str)+1);
	return Str;
}

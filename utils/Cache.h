#ifndef __CACHE_H__
#define __CACHE_H__

#ifdef __cplusplus
extern "C" {
#endif

	//分配总cache池
	struct cacheMan* cacheCreator(int capacity, int elem_t);

	//销毁总cache池
	void cacheDestroy(struct cacheMan* cache, int elem_t);

	//在池中分配一个cache出去
	void* cacheAllocer(struct cacheMan* cache, int elem_t);

	//释放池中的cache
	void cacheFree(struct cacheMan* cache, void* cacheMem);

	void* smallCacheAllocer(int elem_t);

	void smallCacheFree(void* mem);

	char* strdupCache(char* str);

#ifdef __cplusplus
}
#endif 


#endif


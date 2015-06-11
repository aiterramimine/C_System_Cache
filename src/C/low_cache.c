/* 
 * low_cache.c
 * 
 * [NOT TESTED]
 */
 
 #include "low_cache.h"
 #include "strategy.h"

struct Cache_Block_Header *Get_Free_Block(struct Cache *pcache)
{
	struct Cache_Block_Header *ret = pcache->pfree;	
	
	pcache->pfree = NULL;
	
	for(Cache_Block_Header *free = pcache->headers; free < (pcache->headers + pcache->nblocks); free++)
	{
		if(!(free->flags & VALID) && free != ret)
		{
			pcache->pfree = free;
			break;
		}
	}	
	return ret;
}

/* 
 * low_cache.c
 * 
 * [NOT TESTED]
 */

struct Cache_Block_Header *Get_Free_Block(struct Cache *pcache)
{
	struct Cache_Block_Header *ret = pcache->pfree;	
	
	for(Cache_Block_Header *free = pcache->headers; free < (pcache->headers + nblocks); free++)
	{
		if(!(free->flags & VALID) && free != ret)
		{
			pcache->pfree = free;
			break;
		}
	}	
	return ret;
}

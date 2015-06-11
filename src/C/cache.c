/*
 * cache.c
 */

#include "strategy.h"

#include <stdio.h>
#include <string.h>

#include "cache.h"
#include "low_cache.h"


void Create_Blocks(struct Cache *pcache)
{
	int ibcache = 0;
	pcache->headers = malloc(pcache->nblocks*pcache->blocksz);
	for(struct Cache_Block_Header *tmp = pcache->headers; tmp < pcache->headers + pcache->nblocks; tmp++)
	{
		struct Cache_Block_Header cbh = {0, 0, 0, NULL};
		cbh.data = malloc(pcache->nrecords*pcache->recordsz+1);
		cbh.ibcache = ibcache++;
		*tmp = cbh;
	}
	
	pcache->pfree = pcache->headers;
}

//! Création du cache.
struct Cache *Cache_Create(const char *fic, unsigned nblocks, unsigned nrecords, size_t recordsz, unsigned nderef)
{
	struct Cache *cache = malloc(sizeof(struct Cache));
	cache->file = (char*)fic;
	cache->fp = fopen(fic, "r+");
	cache->nblocks = nblocks;
	cache->nrecords = nrecords;
	cache->recordsz = recordsz;
	cache->blocksz = sizeof(struct Cache_Block_Header);
	cache->nderef = nderef;
	cache->pstrategy = Strategy_Create(cache);
	
	struct Cache_Instrument instrument = {0, 0, 0, 0, 0};
	
	cache->instrument = instrument;
	
	Create_Blocks(cache);
	
	return cache;
}

//! Fermeture (destruction) du cache.
Cache_Error Cache_Close(struct Cache *pcache)
{
	Cache_Sync(pcache);
	Strategy_Close(pcache);
	
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
		free(h->data);

	free(pcache->headers);
	pcache = NULL;
	
	return CACHE_OK;
}

//! Synchronisation du cache.
Cache_Error Cache_Sync(struct Cache *pcache)
{	
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
	{
		if(h->flags & MODIF)
		{
			h->flags &= ~MODIF;
			fseek(pcache->fp, h->ibfile, SEEK_SET);
			fwrite(h->data, pcache->blocksz, 1, pcache->fp);
		}
	}
	
	return CACHE_OK;
}

//! Invalidation du cache.
Cache_Error Cache_Invalidate(struct Cache *pcache)
{
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
		h->flags = 0;
	
	Strategy_Invalidate(pcache);
	
	return CACHE_OK;
}

//! Lecture  (à travers le cache).
Cache_Error Cache_Read(struct Cache *pcache, int irfile, void *precord)
{
	int ibfile = (int)(irfile/pcache->nrecords);
	struct Cache_Block_Header *cbh = NULL;
	
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
	{
		if(h->ibfile == ibfile && (h->flags & VALID))
		{
			cbh = h;
			break;
		}
	}
	
	if(!cbh)
	{				
		cbh = Strategy_Replace_Block(pcache);
		cbh->ibfile = ibfile;
		cbh->flags = VALID;
		fseek(pcache->fp, irfile, SEEK_SET);
		fread(cbh->data, pcache->recordsz, pcache->nrecords, pcache->fp);
	}
	
	memcpy(precord, ADDR(pcache, irfile, cbh), pcache->recordsz); 
	
	Strategy_Read(pcache, cbh);
	
	return CACHE_OK;
}

//! Écriture (à travers le cache).
Cache_Error Cache_Write(struct Cache *pcache, int irfile, const void *precord)
{	
	int ibfile = (int)(irfile/pcache->nrecords);
	struct Cache_Block_Header *cbh = NULL;
	
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
	{
		if(h->ibfile == ibfile && (h->flags & VALID))
		{
			cbh = h;
			break;
		}
	}
	
	if(!cbh)
	{				
		cbh = Strategy_Replace_Block(pcache);
		cbh->ibfile = ibfile;
		cbh->flags = (VALID|MODIF);
		fseek(pcache->fp, irfile, SEEK_SET);
		fwrite(cbh->data, pcache->recordsz, pcache->nrecords, pcache->fp);
	}
	
	memcpy(ADDR(pcache, irfile, cbh), precord, pcache->recordsz);
	Strategy_Write(pcache, cbh);
	return CACHE_OK;
}


//! Résultat de l'instrumentation.
struct Cache_Instrument *Cache_Get_Instrument(struct Cache *pcache)
{
	return &(pcache->instrument);
}


int main(int argc, char** args)
{	
	struct Cache *c;
	
	if(!(c = Cache_Create("foo", 30, 10, 10, 0)))
	{
		perror("Cache Create");
		exit(-1);
	}
	
	for(struct Cache_Block_Header *h = c->headers; h < (c->headers + c->nblocks); h++)
	{
		printf("'%s'\n", h->data);
	}
	
	printf("CLOSE\n");
	Cache_Close(c);
	return EXIT_SUCCESS;
}

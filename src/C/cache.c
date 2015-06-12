/*
 * cache.c
 */

#include "strategy.h"

#include <stdio.h>
#include <string.h>

#include "cache.h"
#include "low_cache.h"


int CheckSync(struct Cache *pcache)
{
	struct Cache_Instrument instru = pcache->instrument;
	
	return (!((instru.n_reads+instru.n_writes)%NSYNC) ? 1 : 0); 
}


void Create_Blocks(struct Cache *pcache)
{
	int ibcache = 0;
	pcache->headers = malloc(pcache->nblocks*pcache->blocksz);
	for(struct Cache_Block_Header *tmp = pcache->headers; tmp < pcache->headers + pcache->nblocks; tmp++)
	{
		struct Cache_Block_Header cbh = {0, 0, 0, NULL};
		cbh.data = malloc(pcache->blocksz);
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
	
	if(!(cache->fp = fopen(fic, "w+")))
	{
		perror("fopen");
		exit(-1);
	}
	rewind(cache->fp);

	cache->nblocks = nblocks;
	cache->nrecords = nrecords;
	cache->recordsz = recordsz;
	cache->blocksz = recordsz*nrecords;
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
	
	fclose(pcache->fp);
	
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
			fseek(pcache->fp, DADDR(pcache, h->ibfile), SEEK_SET);
			fwrite(h->data, pcache->blocksz, 1, pcache->fp);
		}
	}
	pcache->instrument.n_syncs++;
	
	return CACHE_OK;
}

//! Invalidation du cache.
Cache_Error Cache_Invalidate(struct Cache *pcache)
{	
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
		h->flags &= ~VALID;

	Strategy_Invalidate(pcache);
	
	return CACHE_OK;
}

//! Lecture  (à travers le cache).
Cache_Error Cache_Read(struct Cache *pcache, int irfile, void *precord)
{
	if(CheckSync(pcache))
		Cache_Sync(pcache);

	int ibfile = (int)(irfile/pcache->nrecords);
	struct Cache_Block_Header *cbh = NULL;
	
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
	{
		if(h->ibfile == ibfile && (h->flags & VALID))
		{
			cbh = h;
			pcache->instrument.n_hits++;
			break;
		}
	}
	
	if(!cbh)
	{				
		cbh = Strategy_Replace_Block(pcache);
		if(!cbh)
			cbh = pcache->headers;

		
		if(cbh->flags&MODIF)
		{
			fseek(pcache->fp, DADDR(pcache, cbh->ibfile), SEEK_SET);
			fwrite(cbh->data, pcache->blocksz, 1, pcache->fp);
		}
		
		cbh->ibfile = ibfile;
		cbh->flags = VALID;
		fseek(pcache->fp, DADDR(pcache, ibfile), SEEK_SET);
		fread(cbh->data, pcache->blocksz, 1, pcache->fp);
	}

	memcpy(precord, ADDR(pcache, irfile, cbh), pcache->recordsz); 
	
	pcache->instrument.n_reads++;

	Strategy_Read(pcache, cbh);
	
	return CACHE_OK;
}

//! Écriture (à travers le cache).
Cache_Error Cache_Write(struct Cache *pcache, int irfile, const void *precord)
{	
	if(CheckSync(pcache))
		Cache_Sync(pcache);
		
	int ibfile = (int)(irfile/pcache->nrecords);
	struct Cache_Block_Header *cbh = NULL;
	
	for(struct Cache_Block_Header *h = pcache->headers; h < (pcache->headers + pcache->nblocks); h++)
	{
		if(h->ibfile == ibfile && (h->flags & VALID))
		{
			pcache->instrument.n_hits++;
			cbh = h;
			break;
		}
	}
	if(!cbh)
	{	
		
		cbh = Strategy_Replace_Block(pcache);
		if(!cbh)
			cbh = pcache->headers;
		
		if(cbh->flags&MODIF)
		{
			fseek(pcache->fp, DADDR(pcache, cbh->ibfile), SEEK_SET);
			fwrite(cbh->data, pcache->blocksz, 1, pcache->fp);
		}
				
		cbh->ibfile = ibfile;
		cbh->flags = (VALID|MODIF);
		fseek(pcache->fp, DADDR(pcache, ibfile), SEEK_SET);
		fread(cbh->data, pcache->blocksz, 1, pcache->fp);
	}
	
	memcpy(ADDR(pcache, irfile, cbh), precord, pcache->recordsz);	
	pcache->instrument.n_writes++;
	
	Strategy_Write(pcache, cbh);
	
	return CACHE_OK;
}


//! Résultat de l'instrumentation.
struct Cache_Instrument *Cache_Get_Instrument(struct Cache *pcache)
{
	struct Cache_Instrument *p = malloc(sizeof(struct Cache_Instrument));
	p->n_reads = pcache->instrument.n_reads;	
	p->n_writes = pcache->instrument.n_writes;
	p->n_hits = pcache->instrument.n_hits;
	p->n_syncs = pcache->instrument.n_syncs;
	p->n_deref = pcache->instrument.n_deref;
	
	pcache->instrument.n_syncs = 0;
	pcache->instrument.n_hits = 0;
	pcache->instrument.n_reads = 0;
	pcache->instrument.n_writes = 0;
	pcache->instrument.n_deref = 0;
	
	return p;
}

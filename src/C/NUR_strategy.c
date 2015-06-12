#include "strategy.h"
#include "low_cache.h"
#include <unistd.h>

/*!
 * Flag at the 3rd bit: R flag of the NUR strategy.
 */
#define RBIT 0x4

struct strat {
    int access_cpt;
    int deref_value;
};



static void deref(struct Cache *pcache);

void *Strategy_Create(struct Cache *pcache) 
{

    struct strat *nur = malloc(sizeof(struct strat));
	nur->access_cpt = 0;
    nur->deref_value = pcache->nderef;

    return nur;
}

void Strategy_Close(struct Cache *pcache)
{
    free(pcache->pstrategy);
}

void Strategy_Invalidate(struct Cache *pcache) 
{

	struct strat *nur = (struct strat *)(pcache->pstrategy);

    if (nur->deref_value != -1) {
        nur->access_cpt = 100;
        deref(pcache);
    }

}
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) 
{
    int ib;
    int min;
    struct Cache_Block_Header *best_block = NULL;
    struct  Cache_Block_Header *pbh;
    int rm;

    if ((pbh = Get_Free_Block(pcache)) != NULL) return pbh;
    min = 4;
    for (ib = 0; ib < pcache->nblocks; ib++)
    {
    pbh = &pcache->headers[ib];

    rm = ((pbh->flags & RBIT)>>2) * 2 + (pbh->flags & (MODIF)>>1);
    if (rm == 0) return pbh;
    else if (rm < min) 
    {
        min = rm;
        best_block = pbh;
    }   
    }
    return best_block;    
}

void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
    deref(pcache);
    pbh->flags |= RBIT;
} 
  
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
    deref(pcache);
    pbh->flags |= RBIT;
}

char *Strategy_Name()
{
    return "NUR";
}

static void deref(struct Cache *pcache)
{
    int ib;
    struct strat *nur = (struct strat *)(pcache->pstrategy);

    if (nur->deref_value == -1 || ++(nur->access_cpt) < nur->deref_value) return;

    for (ib = 0; ib < pcache->nblocks; ib++)
    	pcache->headers[ib].flags &= ~RBIT;

    nur->access_cpt = 0;
    ++pcache->instrument.n_deref;
}

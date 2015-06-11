/*
 *	FIFO strategy
 */

#include "strategy.h"
#include "low_cache.h"
#include "cache_list.h"


//! Creation et initialisation de la stratégie (invoqué par la création de cache).
void *Strategy_Create(struct Cache *pcache)
{
	return Cache_List_Create();
}

//! Fermeture de la stratégie.
void Strategy_Close(struct Cache *pcache)
{
	Cache_List_Delete(((struct Cache_List *)(pcache->pstrategy)));
}

//! Fonction "réflexe" lors de l'invalidation du cache.
void Strategy_Invalidate(struct Cache *pcache)
{
	Cache_List_Clear(((struct Cache_List *)pcache->pstrategy));
}
//! Algorithme de remplacement de bloc.
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache)
{
	struct Cache_Block_Header *pbh;
    struct Cache_List *list = ((struct Cache_List *)pcache->pstrategy);

     /* If no invalid block are found,  we take the first of the list */
    if (!(pbh = Get_Free_Block(pcache)))
    {
        pbh = Cache_List_Remove_First(list);        
    }

    Cache_List_Append(list, pbh);
    return pbh; 
}

//! Fonction "réflexe" lors de la lecture.
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pb)
{
	
}

//! Fonction "réflexe" lors de l'écriture.
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pb)
{

}

//! Identification de la stratégie.
char *Strategy_Name()
{
	return "FIFO";
}

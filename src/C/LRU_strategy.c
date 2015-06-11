/*
 *	LRU strategy
 */
 
 #include <assert.h>

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
	Cache_List_Delete((struct Cache_List*)pcache->pstrategy);
}

//! Fonction "réflexe" lors de l'invalidation du cache.
void Strategy_Invalidate(struct Cache *pcache){
	Cache_List_Clear((struct Cache_List *)pcache->pstrategy);
}

//! Algorithme de remplacement de bloc.
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache)
{
    struct Cache_Block_Header *pbh;
    struct Cache_List *list = (struct Cache_List*)pcache->pstrategy, *tmp_list = list; 

    /* On cherche d'abord un bloc invalide */
    if ((pbh = Get_Free_Block(pcache)) != NULL){
		Cache_List_Move_To_End(tmp_list, pbh);
		return pbh;
	}
    while(tmp_list)
    {
		pbh = tmp_list->pheader;
		
		if(!pbh)
			break;
		
		if(pbh && !(VALID & pbh->flags))
		{			
			Cache_List_Move_To_End(list, pbh);
			return pbh;
		}	
		tmp_list = tmp_list->prev;
	}
	
    /* Pas de bloc invalide trouvé, on prend le 1er	*/
    pbh = Cache_List_Remove_First(list);
    Cache_List_Move_To_End(list, pbh);
    return pbh;
}

//! Fonction "réflexe" lors de la lecture.
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pb)
{
	Cache_List_Move_To_End((struct Cache_List*)pcache->pstrategy, pb);
}

//! Fonction "réflexe" lors de l'écriture.
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pb)
{
	Cache_List_Move_To_End((struct Cache_List*)pcache->pstrategy, pb);
}

//! Identification de la stratégie.
char *Strategy_Name()
{
	return "LRU";
}


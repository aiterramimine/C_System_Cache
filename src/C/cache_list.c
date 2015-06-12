#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h> 
#include <strings.h>

#include "cache_list.h"

struct Cache_List *Cache_List_Create()
{
	struct Cache_List *list = malloc(sizeof(struct Cache_List));
	list->next = NULL;
	list->prev = NULL;
	list->pheader = NULL;
	return list;
}

struct Cache_List *Cache_List_Create_With_Header(struct Cache_Block_Header *pbh)
{
	struct Cache_List *pnew_list_with_header = Cache_List_Create();
	pnew_list_with_header->pheader = pbh;
	
	return pnew_list_with_header;
}

void Cache_List_Prepend(struct Cache_List *list, struct Cache_Block_Header *pbh)
{
	if(list->pheader == NULL)
	{
		list->pheader = pbh;
	}	
	else
	{	
		struct Cache_List *new_list = Cache_List_Create();
		
		while(list->prev != NULL)
			list = list->prev;
			
		new_list->pheader = list->pheader;
		list->pheader = pbh;
		
		new_list->next = list->next;
		list->next = new_list;
		new_list->prev = list;
	}
}
/* Ajoutes un noeud a la fin de la liste */
void Cache_List_Append(struct Cache_List *list, struct Cache_Block_Header *pbh)
{
	if(list->pheader == NULL)
	{
		list->pheader = pbh;
	}
	
	else
	{	
		struct Cache_List *pnew_list = Cache_List_Create_With_Header(pbh);
		
		while(list->next != NULL)
			list = list->next;
		
		list->next = pnew_list;
		pnew_list->prev = list;
	}
}

void Cache_List_Clear(struct Cache_List *list)
{
	assert(list != NULL);

	while(list->prev)
		list = list->prev;
	
	struct Cache_List *tmp;
	while(list)
	{
		list->prev = NULL;
		free(list->pheader);
		tmp = list->next;
		list->next = NULL;
		list = tmp;
	}
	
	list = Cache_List_Create();
}

/*! Retrait du premier élément */
struct Cache_Block_Header *Cache_List_Remove_First(struct Cache_List *list)
{
	return Cache_List_Remove(list, list->pheader);
}

/*! Retrait du dernier élément */
struct Cache_Block_Header *Cache_List_Remove_Last(struct Cache_List *list)
{
	if(Cache_List_Is_Empty(list))
		return NULL;
		
	while(list->next)
		list = list->next;
		
	struct Cache_Block_Header *cbh = list->pheader;
	
	list->prev->next = NULL;
	
	free(list);
	
	return cbh;
}


/*! Retrait d'un élément quelconque */
struct Cache_Block_Header *Cache_List_Remove(struct Cache_List *list, struct Cache_Block_Header *pbh)
{
	assert(list);
	
	while(list)
	{
		if(list->pheader == pbh){
			list->prev->next = list->next;
			list->next->prev = list->prev;
			struct Cache_Block_Header *header = list->pheader;
			free(list);			
			return header;
		}
		list = list->next;
	}	
	return NULL;
}
                                             


/*! Test de liste vide */
bool Cache_List_Is_Empty(struct Cache_List *list)
{
	return (list->next || list->pheader) ? false : true;
}

/*! Transférer un élément à la fin */
void Cache_List_Move_To_End(struct Cache_List *list, struct Cache_Block_Header *pbh)
{
	assert(list);
	
	Cache_List_Remove(list, pbh);
	Cache_List_Append(list, pbh);	
}


/*! Transférer un élément  au début */
void Cache_List_Move_To_Begin(struct Cache_List *list, struct Cache_Block_Header *pbh)
{
	assert(list);
	
	Cache_List_Remove(list, pbh);
	Cache_List_Prepend(list, pbh);
}


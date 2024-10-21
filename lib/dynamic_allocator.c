/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("initialize_dynamic_allocator is not implemented yet");

	LIST_INIT(&freeBlocksList);
	struct BlockElement first_free_block;
	struct BlockElement *ptr_first_free_block = (struct BlockElement*) (daStart + 2*sizeof(int));

	int begin_flag = 1, end_flag = 1, begin_size = initSizeOfAllocatedSpace - 2*sizeof(int), end_size = initSizeOfAllocatedSpace - 2*sizeof(int);
	int* ptr_begin_flag = (int*)daStart, *ptr_end_flag = (int*) (initSizeOfAllocatedSpace + daStart - sizeof(int)), *ptr_begin_size = (int*) (daStart + sizeof(int)), *ptr_end_size = (int*) (initSizeOfAllocatedSpace + daStart - 2*sizeof(int));

	*ptr_begin_flag = begin_flag;
	*ptr_begin_size = begin_size;
	*ptr_end_flag = end_flag;
	*ptr_end_size = end_size;

	*ptr_first_free_block = first_free_block;

	LIST_INSERT_HEAD(&freeBlocksList, ptr_first_free_block);

}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
    //TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("set_block_data is not implemented yet");

	    uint32 *address=(uint32*)va;
	    address--;
	    *address= totalSize + isAllocated;
	    void *ptr_move = (void*)va;
	    ptr_move += (totalSize - 2*sizeof(uint32));
	    address = (uint32*)ptr_move;
	    *address = totalSize + isAllocated;
}


//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
//	panic("alloc_block_FF is not implemented yet");

	if(size == 0)
	    return NULL;

	struct BlockElement *element;
	LIST_FOREACH(element, &(freeBlocksList))
	{
	    if(get_block_size(element) >= size + 2*sizeof(int))
	    {
	        if(get_block_size(element) - size - 2*sizeof(int) >= 16) {
	            struct BlockElement* new_free_block = (struct BlockElement*)((void*)element + size + 2*sizeof(int));
	            LIST_INSERT_AFTER(&(freeBlocksList), element, new_free_block);
	            set_block_data(new_free_block, get_block_size(element) - size - 2*sizeof(int), 0);
	            set_block_data(element, size + 2*sizeof(int), 1);
	            LIST_REMOVE(&(freeBlocksList), element);
	        }
	        else
	        {
	            set_block_data(element, get_block_size(element), 1);
	            LIST_REMOVE(&(freeBlocksList), element);
	        }

	        return (void*)element;
	    }
	}

	if(sbrk(LIST_SIZE(&(freeBlocksList))) == (void*)-1)
	    return NULL;

	return sbrk(LIST_SIZE(&(freeBlocksList)));

}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_BF is not implemented yet");

	if(size == 0)
		    return NULL;

	int bestSizeDiff = -1;
	void* address;

	bool possibleAllocation = 0;
	struct BlockElement *element;
	LIST_FOREACH(element, &(freeBlocksList))
	{
		if(get_block_size(element) >= size + 2*sizeof(int))
		{
			possibleAllocation = 1;
			if(get_block_size(element) - size - 2*sizeof(int) < bestSizeDiff || bestSizeDiff == -1) {
				bestSizeDiff = get_block_size(element) - size - 2*sizeof(int);
				address = (void*) element;
			}
		}
	}

	if(possibleAllocation) {
		if(get_block_size(address) - size - 2*sizeof(int) >= 16) {
			struct BlockElement* new_free_block = (struct BlockElement*)((void*)address + size + 2*sizeof(int));
			LIST_INSERT_AFTER(&(freeBlocksList), (struct BlockElement *)address, new_free_block);
			set_block_data(new_free_block, get_block_size(address) - size - 2*sizeof(int), 0);
			set_block_data(address, size + 2 * sizeof(int), 1);
			LIST_REMOVE(&(freeBlocksList), (struct BlockElement *)address);
		}
		else
		{
			set_block_data(address, get_block_size(address), 1);
			LIST_REMOVE(&(freeBlocksList), (struct BlockElement *)address);
		}

		return (void*)address;
	}

	if(sbrk(LIST_SIZE(&(freeBlocksList))) == (void*)-1)
		return NULL;

	return sbrk(LIST_SIZE(&(freeBlocksList)));
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_block is not implemented yet");

	if(va == NULL)
		return;

	int* before_footer = (int*) (va - 2*sizeof(int));
	int* after_header = (int*) (va + get_block_size(va) - sizeof(int));
	int* header = (int*) (va - sizeof(int));
	int* footer = (int*) (va + get_block_size(va) - 2*sizeof(int));

	struct BlockElement *element;
	struct BlockElement *it;

	bool first = 0;

	LIST_FOREACH(it, &(freeBlocksList))
	{
		element = it;
		if((void*) it > va)
		{
			first = 1;
			break;
		}

	}

	if(*before_footer % 2 != 0 && *after_header % 2 != 0)
	{
		set_block_data(va, get_block_size(va), 0);
		if(!LIST_SIZE(&(freeBlocksList)) || first) {
			LIST_INSERT_HEAD(&(freeBlocksList),(struct BlockElement*) va);
		}
		else{
			LIST_INSERT_AFTER(&(freeBlocksList), element, (struct BlockElement*) va);
		}
	}
	else if(*before_footer % 2 == 0 && *after_header % 2 != 0)
	{
		int newSize = *before_footer + get_block_size(va);
		void* ptr_prev_element = va - *before_footer;
		set_block_data(ptr_prev_element, newSize, 0);

		before_footer = NULL;
		header = NULL;
	}

	else if(*before_footer % 2 != 0 && *after_header % 2 == 0)
	{
		struct BlockElement moved_free_block;
		struct BlockElement* ptr_blockelement = (struct BlockElement*)(va + get_block_size(va));
		struct BlockElement* ptr_moved_free_block = (struct BlockElement*) va;
		*ptr_moved_free_block = moved_free_block;
		int newSize = *after_header + get_block_size(va);
		set_block_data(va, newSize, 0);
		LIST_INSERT_BEFORE(&(freeBlocksList),ptr_blockelement,ptr_moved_free_block);
		LIST_REMOVE(&(freeBlocksList),ptr_blockelement);

		after_header = NULL;
		footer = NULL;

		int *ptr = (int*) ptr_blockelement;

		for(int i = 0 ; i < 2 ; i++) {
			*ptr = (int)NULL;
		}

	}
	else {
		int newSize = *after_header + *before_footer + get_block_size(va);
		void* ptr_prev = va - *before_footer;
		struct BlockElement* ptr_next_blockelement = (struct BlockElement*)(va + *after_header);

		set_block_data(ptr_prev, newSize, 0);
		LIST_REMOVE(&(freeBlocksList),ptr_next_blockelement);

		after_header = NULL;
		footer = NULL;
		before_footer = NULL;
		header = NULL;
		ptr_next_blockelement = NULL;

	}

}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("realloc_block_FF is not implemented yet");
	//Your Code is Here...
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

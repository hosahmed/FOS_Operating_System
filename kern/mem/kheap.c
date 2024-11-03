#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	// Write your code here, remove the panic and write your code

	segmentBreak = start = ROUNDDOWN(daStart, PAGE_SIZE);
    initSizeToAllocate = ROUNDUP(initSizeToAllocate, PAGE_SIZE);
	uint32 noOfPagesToAllocate = initSizeToAllocate / 4096; /*4KB*/
	segmentBreak += initSizeToAllocate;
	hardLimit = daLimit;
	uint32 iterator = start;

    if(start + initSizeToAllocate > daLimit)
    	panic("Initial size exceeds the given limit");


	for (int i = 0; i < noOfPagesToAllocate; i++ ){
		struct FrameInfo *ptr_frame_info;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, iterator, PERM_USER | PERM_WRITEABLE);
		iterator += 4096;
	}

	initialize_dynamic_allocator(start, initSizeToAllocate);
	return 0;
}

void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */
	//MS2: COMMENT THIS LINE BEFORE START CODING==========
	//return (void*)-1 ;
	//====================================================

	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	// Write your code here, remove the panic and write your code
	//panic("sbrk() is not implemented yet...!!");

	if(!numOfPages)
		return (void*)segmentBreak;

	uint32 oldSegBreak = segmentBreak;
	uint32 iterator = segmentBreak;

	if(hardLimit < segmentBreak + 4096*numOfPages || !LIST_SIZE(&MemFrameLists.free_frame_list))
		return (void*)-1;


	for (int i = 0; i < numOfPages; i++){
		struct FrameInfo *ptr_frame_info;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, iterator, PERM_USER | PERM_WRITEABLE);
		iterator += 4096;
	}

	uint32* last_footer = (uint32*)(segmentBreak - 2*sizeof(int));
	uint32 last_block_size = *last_footer - (*last_footer % 2 == 0 ? 0 : 1);

//	cprintf("\n\nlast block size = %d\n\n", last_block_size);

	void* last_block = (void*)(segmentBreak - last_block_size);

	if(is_free_block(last_block)) {
		set_block_data(last_block, get_block_size(last_block) + 4096*numOfPages,0);
	} else {
		set_block_data((void*)oldSegBreak, 4096*numOfPages,0);
		LIST_INSERT_TAIL(&(freeBlocksList),(struct BlockElement*)(void*)oldSegBreak);
	}

	uint32* endBlock = (uint32*)(iterator - sizeof(int));

	*endBlock = 1;

	segmentBreak = iterator;

	return (void*)oldSegBreak;
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	if(isKHeapPlacementStrategyFIRSTFIT()) {
		if(size <= DYN_ALLOC_MAX_BLOCK_SIZE) //2048
		{
			return alloc_block_FF(size);
		}
		else
		{
			size = ROUNDUP(size, PAGE_SIZE);
			uint32 noOfPagesToAllocate = size / PAGE_SIZE;//to see how many free frame needed
			uint32 iterator = hardLimit+PAGE_SIZE;
			uint32 *ptr_table = NULL;
			bool canAllock = 0;
			int countOfContinuesFrame=0;
			while(iterator!= KERNEL_HEAP_MAX)
			{

				if(get_frame_info(ptr_page_directory, iterator, &ptr_table)==NULL)
				{
					countOfContinuesFrame++;
					if(countOfContinuesFrame==noOfPagesToAllocate)
					{
						canAllock=1;
						break;
					}
				}
				else
				{
					countOfContinuesFrame=0;
				}

				iterator += PAGE_SIZE;
			}

			if(canAllock)
			{
				iterator -= (PAGE_SIZE*(noOfPagesToAllocate-1));
				for (int i = 0; i < noOfPagesToAllocate; i++ )
				{
					struct FrameInfo *ptr_frame_info;
					allocate_frame(&ptr_frame_info);
					map_frame(ptr_page_directory, ptr_frame_info, iterator, PERM_WRITEABLE);
					iterator += PAGE_SIZE;
				}
				uint32 *ptrFrames =  (uint32*)(iterator - (PAGE_SIZE*noOfPagesToAllocate));
				*ptrFrames = noOfPagesToAllocate;
				cprintf("\nPages To Allocate = %d\n", noOfPagesToAllocate);
				return (void*)(iterator - (PAGE_SIZE*noOfPagesToAllocate));
			}

		}
	}

	return NULL;
	/*for fast allock(bonus) we will use a LinkedList of a Struct
	 * and that LinkedList contain the all the the full Frames,
	 * and that Struct contain the size and the start address of those Frames
	 */

}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	uint32 address = (uint32)virtual_address;

	if(address > start && address < hardLimit) {
		free_block(virtual_address);
	} else if(address >= hardLimit + PAGE_SIZE && address < KERNEL_HEAP_MAX) {
		uint32 noOfFrames = *((uint32*)virtual_address);
		cprintf("\nNo of Frames = %d\n", noOfFrames);
		uint32 iterator = address;

		for (int i = 0; i < noOfFrames; i++)
		{
			unmap_frame(ptr_page_directory, iterator);
			iterator += PAGE_SIZE;
		}
	} else {
		panic("invalid address");
	}

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}

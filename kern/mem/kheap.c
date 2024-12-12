#include "kheap.h"
#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//////////////////////////////////////////////////////
// Fast Page Allocator Data Structures

struct PageBlock {
    uint32 va;
    uint32 size;
};

struct FreeBlock {
	uint32 va;
    uint32 size;
};

struct
{
	struct PageBlock allocated_blocks[NUM_OF_KHEAP_PAGES];
	struct FreeBlock free_blocks[NUM_OF_KHEAP_PAGES/2];
	uint32 block_count;
	uint32 free_count;
	struct spinlock arrayslock; //Use it to protect the arrays in the kernel
} AllArrays;

//////////////////////////////////////////////////////

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
		ptr_frame_info->va = iterator;
		iterator += 4096;
	}

	init_spinlock(&(AllArrays.arrayslock), "kern_lock");
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
		ptr_frame_info->va = iterator;
		iterator += 4096;
	}

	segmentBreak = iterator;

	return (void*)oldSegBreak;
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	if(!holding_spinlock(&AllArrays.arrayslock))
	{
		acquire_spinlock(&(AllArrays.arrayslock));
	}

	if (isKHeapPlacementStrategyFIRSTFIT())
    {
        if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
        {
        	void* va = alloc_block_FF(size);
        	if(holding_spinlock(&AllArrays.arrayslock))
			{
				release_spinlock(&(AllArrays.arrayslock));
			}
        	return va;
        }
        else
        {
        	size = ROUNDUP(size, PAGE_SIZE);
            uint32 noOfPagesToAllocate = size / PAGE_SIZE;
            uint32 allocationAddress;
            bool canAllocate = 0;

            if (AllArrays.block_count == 0)
            {
                if (size <= KERNEL_HEAP_MAX - hardLimit - PAGE_SIZE)
                {
                    allocationAddress = hardLimit + PAGE_SIZE;
                    AllArrays.allocated_blocks[0].va = allocationAddress;
                    AllArrays.allocated_blocks[0].size = size;
                    AllArrays.block_count = 1;

                    if (size < KERNEL_HEAP_MAX - hardLimit - PAGE_SIZE)
                    {
                        AllArrays.free_blocks[0].va = allocationAddress + size;
                        AllArrays.free_blocks[0].size = (KERNEL_HEAP_MAX - hardLimit - PAGE_SIZE) - size;
                        AllArrays.free_count = 1;
                    }
                    else
                    {
                        AllArrays.free_count = 0;
                    }

                    for (uint32 i = 0; i < noOfPagesToAllocate; i++) {
                        struct FrameInfo *ptr_frame_info;
                        allocate_frame(&ptr_frame_info);
                        map_frame(ptr_page_directory, ptr_frame_info, allocationAddress, PERM_WRITEABLE);
                        ptr_frame_info->va = allocationAddress;
                        allocationAddress += PAGE_SIZE;
                    }
                    void* va = (void*)(AllArrays.allocated_blocks[0].va);
                    if(holding_spinlock(&AllArrays.arrayslock))
					{
						release_spinlock(&(AllArrays.arrayslock));
					}
                    return va;
                }
                else
                {
                	if(holding_spinlock(&AllArrays.arrayslock))
					{
						release_spinlock(&(AllArrays.arrayslock));
					}
                    return NULL;
                }
            }

            for (uint32 i = 0; i < AllArrays.free_count; i++)
            {
                if (AllArrays.free_blocks[i].size >= size)
                {
                    allocationAddress = AllArrays.free_blocks[i].va;

                    if (AllArrays.free_blocks[i].size == size)
                    {
                        memmove(&AllArrays.free_blocks[i], &AllArrays.free_blocks[i + 1], (AllArrays.free_count - i - 1) * sizeof(struct FreeBlock));
                        AllArrays.free_count--;
                    }
                    else
                    {
                        AllArrays.free_blocks[i].size -= size;
                        AllArrays.free_blocks[i].va += size;
                    }

                    canAllocate = 1;
                    break;
                }
            }

            if (!canAllocate)
            {
            	if(holding_spinlock(&AllArrays.arrayslock))
            	{
            		release_spinlock(&(AllArrays.arrayslock));
            	}
            	return NULL;
            }

            uint32 left = 0;
            uint32 right = AllArrays.block_count;

            while (left < right)
            {
                uint32 mid = left + (right - left) / 2 ;
                if (AllArrays.allocated_blocks[mid].va < allocationAddress)
                {
                	left = mid + 1;
                }
                else
                {
                	right = mid;
                }
            }

            uint32 index = left;
            memmove(&AllArrays.allocated_blocks[index + 1], &AllArrays.allocated_blocks[index], (AllArrays.block_count - index) * sizeof(struct PageBlock));
            AllArrays.allocated_blocks[index].va = allocationAddress;
            AllArrays.allocated_blocks[index].size = size;
            AllArrays.block_count++;

            for (uint32 i = 0 ; i < noOfPagesToAllocate; i++)
            {
                struct FrameInfo *ptr_frame_info;
                allocate_frame(&ptr_frame_info);
                map_frame(ptr_page_directory, ptr_frame_info, allocationAddress, PERM_WRITEABLE);
                ptr_frame_info->va = allocationAddress;
                allocationAddress += PAGE_SIZE;
            }
            void* va = (void*)(allocationAddress - (PAGE_SIZE * noOfPagesToAllocate));
            if(holding_spinlock(&AllArrays.arrayslock))
			{
				release_spinlock(&(AllArrays.arrayslock));
			}
            return va;
        }
    }
	if(holding_spinlock(&AllArrays.arrayslock))
	{
		release_spinlock(&(AllArrays.arrayslock));
	}
    return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	if(!holding_spinlock(&AllArrays.arrayslock))
	{
		acquire_spinlock(&(AllArrays.arrayslock));
	}
    uint32 address = (uint32)virtual_address;

    if (address > start && address < hardLimit)
    {
        free_block(virtual_address);
        if(holding_spinlock(&AllArrays.arrayslock))
		{
			release_spinlock(&(AllArrays.arrayslock));
		}
        return;
    }
    else if (address >= hardLimit + PAGE_SIZE && address < KERNEL_HEAP_MAX)
    {
        uint32 iterator = address;
        uint32 noOfFrames = 0;
        uint32 size = 0;
        int index = -1;

        int left = 0;
        int right = AllArrays.block_count - 1;
        while (left <= right)
        {
            int mid = left + (right - left) / 2;

            if (AllArrays.allocated_blocks[mid].va == address)
            {
                noOfFrames = AllArrays.allocated_blocks[mid].size / PAGE_SIZE;
                size = AllArrays.allocated_blocks[mid].size;
                index = mid;

                memmove(&AllArrays.allocated_blocks[mid], &AllArrays.allocated_blocks[mid + 1],
                        (AllArrays.block_count - mid - 1) * sizeof(struct PageBlock));
                AllArrays.block_count--;
                break;
            }
            else if (AllArrays.allocated_blocks[mid].va < address)
            {
                left = mid + 1;
            }
            else
            {
                right = mid - 1;
            }
        }

        if (index == -1)
        {
        	if(holding_spinlock(&AllArrays.arrayslock))
			{
				release_spinlock(&(AllArrays.arrayslock));
			}
            return;
        }

        struct FreeBlock newFreeBlock;
        newFreeBlock.va = address;
        newFreeBlock.size = size;

        left = 0;
		right = AllArrays.free_count - 1;
		while (left <= right)
		{
			int mid = left + (right - left) / 2;

			if (AllArrays.free_blocks[mid].va < newFreeBlock.va)
			{
				left = mid + 1;
			}
			else
			{
				right = mid - 1;
			}
		}

		int insertIndex = left;

        if (AllArrays.free_count > 0 && insertIndex < AllArrays.free_count)
        {
            memmove(&AllArrays.free_blocks[insertIndex + 1], &AllArrays.free_blocks[insertIndex],
                    (AllArrays.free_count - insertIndex) * sizeof(struct FreeBlock));
        }
        AllArrays.free_blocks[insertIndex] = newFreeBlock;
        AllArrays.free_count++;

        if (insertIndex > 0)
        {
            struct FreeBlock* prevBlock = &AllArrays.free_blocks[insertIndex - 1];
            if (prevBlock->va + prevBlock->size == newFreeBlock.va)
            {
                prevBlock->size += newFreeBlock.size;
                memmove(&AllArrays.free_blocks[insertIndex], &AllArrays.free_blocks[insertIndex + 1],
                        (AllArrays.free_count - insertIndex - 1) * sizeof(struct FreeBlock));
                AllArrays.free_count--;
                insertIndex--;
            }
        }

        if (insertIndex < AllArrays.free_count - 1)
        {
            struct FreeBlock* nextBlock = &AllArrays.free_blocks[insertIndex + 1];
            if (newFreeBlock.va + newFreeBlock.size == nextBlock->va)
            {
                AllArrays.free_blocks[insertIndex].size += nextBlock->size;
                memmove(&AllArrays.free_blocks[insertIndex + 1], &AllArrays.free_blocks[insertIndex + 2],
                        (AllArrays.free_count - insertIndex - 2) * sizeof(struct FreeBlock));
                AllArrays.free_count--;
            }
        }

        for (uint32 i = 0; i < noOfFrames; i++)
        {
            unmap_frame(ptr_page_directory, iterator);
            iterator += PAGE_SIZE;
        }
    }
    else
    {
    	if(holding_spinlock(&AllArrays.arrayslock))
		{
			release_spinlock(&(AllArrays.arrayslock));
		}
        return;
    }
    if(holding_spinlock(&AllArrays.arrayslock))
	{
		release_spinlock(&(AllArrays.arrayslock));
	}
    //you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code

	uint32 *ptr_table ;
	int w=get_page_table(ptr_page_directory, virtual_address, &ptr_table);

	if (w==TABLE_NOT_EXIST)
	{
		return 0;
	}
	if(((ptr_table[PTX(virtual_address)] & PERM_PRESENT)))
	{
	    return (unsigned int) ((ptr_table[PTX(virtual_address)] ) & 0xFFFFF000) +(virtual_address & 0x00000FFF);
	}
	else
	{
		return 0;
	}
	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code

	//panic("kheap_virtual_address() is not implemented yet...!!");

	unsigned int obtained_offset=PGOFF(physical_address);

	struct FrameInfo* frame = to_frame_info(physical_address);
	if(frame->va == 0)
	{
		return 0;
	}

	else
	{
		return frame->va + obtained_offset;
	}

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
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	if(!holding_spinlock(&AllArrays.arrayslock))
	{
		acquire_spinlock(&(AllArrays.arrayslock));
	}
	// case 1
	if(virtual_address == NULL && new_size != 0)
	{
		void* va = kmalloc(new_size);
		if(holding_spinlock(&AllArrays.arrayslock))
		{
			release_spinlock(&(AllArrays.arrayslock));
		}
		return va;
	}

	// case 2
	if(virtual_address != NULL && new_size == 0)
	{
		kfree(virtual_address);
		if(holding_spinlock(&AllArrays.arrayslock))
		{
			release_spinlock(&(AllArrays.arrayslock));
		}
		return NULL;
	}

	// case 3
	if(virtual_address == NULL && new_size == 0)
	{
		if(holding_spinlock(&AllArrays.arrayslock))
		{
			release_spinlock(&(AllArrays.arrayslock));
		}
		return NULL;
	}

	uint32 address = (uint32)virtual_address;

	// va is in block allocator
	if(address > start && address < hardLimit)
	{
		// block -> block (case 4)
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			if(!is_free_block(virtual_address))
			{
				void* va = realloc_block_FF(virtual_address, new_size);
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return va;
			}
			else
			{
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return NULL;
			}
		}
		// block -> page (case 5)
		else
		{
			if(is_free_block(virtual_address))
			{
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return NULL;
			}
			void* newVA = kmalloc(new_size);
			if(newVA)
			{
				memcpy(newVA, virtual_address, get_block_size(virtual_address) - 8);
				kfree(virtual_address);
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return newVA;
			}
			else
			{
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return NULL;
			}
		}
	}

	// va is in page allocator
	else if(address >= hardLimit + PAGE_SIZE && address < KERNEL_HEAP_MAX)
	{
		// page -> block (case 6)
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			void* newVA = kmalloc(new_size);
			if(newVA)
			{
				memcpy(newVA, virtual_address, new_size);
				kfree(virtual_address);
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return newVA;
			}
			else
			{
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return NULL;
			}
		}
		// page -> page (case 7)
		else
		{
			new_size = ROUNDUP(new_size, PAGE_SIZE);

			if(new_size > KERNEL_HEAP_MAX - hardLimit - PAGE_SIZE)
			{
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return NULL;
			}

			int left = 0;
			int right = AllArrays.block_count - 1;
			int allocatedBlockIndex = -1;
			uint32 nextBlockVA;

			while (left <= right)
			{
				int mid = left + (right - left) / 2;

				if (AllArrays.allocated_blocks[mid].va == address)
				{
					allocatedBlockIndex = mid;
					nextBlockVA = AllArrays.allocated_blocks[mid].va + AllArrays.allocated_blocks[mid].size;
					break;
				}
				else if (AllArrays.allocated_blocks[mid].va < address)
				{
					left = mid + 1;
				}
				else
				{
					right = mid - 1;
				}
			}

			if(allocatedBlockIndex == -1)
			{
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return NULL;
			}

			// special case (there is no next block then reallocate and if done free the old heap space)
			if(nextBlockVA == KERNEL_HEAP_MAX)
			{
				if(new_size > AllArrays.allocated_blocks[allocatedBlockIndex].size)
				{
					int copySize = AllArrays.allocated_blocks[allocatedBlockIndex].size;
					void* newVA = kmalloc(new_size);
					if(newVA)
					{
						memcpy(newVA, virtual_address, copySize);
						kfree(virtual_address);
						if(holding_spinlock(&AllArrays.arrayslock))
						{
							release_spinlock(&(AllArrays.arrayslock));
						}
						return newVA;
					}
					else
					{
						if(holding_spinlock(&AllArrays.arrayslock))
						{
							release_spinlock(&(AllArrays.arrayslock));
						}
						return NULL;
					}
				}
				else if(new_size < AllArrays.allocated_blocks[allocatedBlockIndex].size)
				{
					int noOfFramesToDellaocate = (AllArrays.allocated_blocks[allocatedBlockIndex].size - new_size) / PAGE_SIZE;

					struct FreeBlock newFreeBlock;
					newFreeBlock.va = AllArrays.allocated_blocks[allocatedBlockIndex].va + new_size;
					newFreeBlock.size = AllArrays.allocated_blocks[allocatedBlockIndex].size - new_size;

					left = 0;
					right = AllArrays.free_count - 1;
					while (left <= right)
					{
						int mid = left + (right - left) / 2;

						if (AllArrays.free_blocks[mid].va < newFreeBlock.va)
						{
							left = mid + 1;
						}
						else
						{
							right = mid - 1;
						}
					}

					int insertIndex = left;

					if (AllArrays.free_count > 0 && insertIndex < AllArrays.free_count)
					{
						memmove(&AllArrays.free_blocks[insertIndex + 1], &AllArrays.free_blocks[insertIndex],
								(AllArrays.free_count - insertIndex) * sizeof(struct FreeBlock));
					}
					AllArrays.free_blocks[insertIndex] = newFreeBlock;
					AllArrays.free_count++;

					AllArrays.allocated_blocks[allocatedBlockIndex].size = new_size;

					uint32 iterator = AllArrays.allocated_blocks[allocatedBlockIndex].va + AllArrays.allocated_blocks[allocatedBlockIndex].size;

					for (uint32 i = 0; i < noOfFramesToDellaocate; i++)
					{
						unmap_frame(ptr_page_directory, iterator);
						iterator += PAGE_SIZE;
					}
				}
				void* va = (void*)AllArrays.allocated_blocks[allocatedBlockIndex].va;
				if(holding_spinlock(&AllArrays.arrayslock))
				{
					release_spinlock(&(AllArrays.arrayslock));
				}
				return va;
			}


			// next block is allocated (case 7.1)
			else if(allocatedBlockIndex < AllArrays.block_count - 1 && AllArrays.allocated_blocks[allocatedBlockIndex+1].va == AllArrays.allocated_blocks[allocatedBlockIndex].va + AllArrays.allocated_blocks[allocatedBlockIndex].size)
			{
				// next block is allocated and size is equal do nothing(case 7.1.1)
				if(AllArrays.allocated_blocks[allocatedBlockIndex].size == new_size)
				{
					void* va = (void*)AllArrays.allocated_blocks[allocatedBlockIndex].va;
					if(holding_spinlock(&AllArrays.arrayslock))
					{
						release_spinlock(&(AllArrays.arrayslock));
					}
					return va;
				}
				// next block is allocated and size is increased (case 7.1.2)
				if(AllArrays.allocated_blocks[allocatedBlockIndex].size < new_size)
				{
					int copySize = AllArrays.allocated_blocks[allocatedBlockIndex].size;
					void* newVA = kmalloc(new_size);
					if(newVA)
					{
						memcpy(newVA, virtual_address, copySize);
						kfree(virtual_address);
						if(holding_spinlock(&AllArrays.arrayslock))
						{
							release_spinlock(&(AllArrays.arrayslock));
						}
						return newVA;
					}
					else
					{
						if(holding_spinlock(&AllArrays.arrayslock))
						{
							release_spinlock(&(AllArrays.arrayslock));
						}
						return NULL;
					}
				}
				// next block is allocated and size is decreased (case 7.1.3)
				// then add a new free block in between after removing the extra space
				else
				{
					int noOfFramesToDellaocate = (AllArrays.allocated_blocks[allocatedBlockIndex].size - new_size) / PAGE_SIZE;

					struct FreeBlock newFreeBlock;
					newFreeBlock.va = AllArrays.allocated_blocks[allocatedBlockIndex].va + new_size;
					newFreeBlock.size = AllArrays.allocated_blocks[allocatedBlockIndex].size - new_size;

					left = 0;
					right = AllArrays.free_count - 1;
					while (left <= right)
					{
						int mid = left + (right - left) / 2;

						if (AllArrays.free_blocks[mid].va < newFreeBlock.va)
						{
							left = mid + 1;
						}
						else
						{
							right = mid - 1;
						}
					}

					int insertIndex = left;

					if (AllArrays.free_count > 0 && insertIndex < AllArrays.free_count)
					{
						memmove(&AllArrays.free_blocks[insertIndex + 1], &AllArrays.free_blocks[insertIndex],
								(AllArrays.free_count - insertIndex) * sizeof(struct FreeBlock));
					}
					AllArrays.free_blocks[insertIndex] = newFreeBlock;
					AllArrays.free_count++;

					AllArrays.allocated_blocks[allocatedBlockIndex].size = new_size;

					uint32 iterator = AllArrays.allocated_blocks[allocatedBlockIndex].va + AllArrays.allocated_blocks[allocatedBlockIndex].size;

					for (uint32 i = 0; i < noOfFramesToDellaocate; i++)
					{
						unmap_frame(ptr_page_directory, iterator);
						iterator += PAGE_SIZE;
					}
					void* va = (void*)AllArrays.allocated_blocks[allocatedBlockIndex].va;
					if(holding_spinlock(&AllArrays.arrayslock))
					{
						release_spinlock(&(AllArrays.arrayslock));
					}
					return va;
				}
			}
			// next block is free (case 7.2)
			else
			{
				int left = 0;
				int right = AllArrays.block_count - 1;
				int freeBlockIndex = -1;
				while (left <= right)
				{
					int mid = left + (right - left) / 2;

					if (AllArrays.free_blocks[mid].va == nextBlockVA)
					{
						freeBlockIndex = mid;
						break;
					}
					else if (AllArrays.free_blocks[mid].va < nextBlockVA)
					{
						left = mid + 1;
					}
					else
					{
						right = mid - 1;
					}
				}
				// next block is free and size is equal do nothing (case 7.2.1)
				if(AllArrays.allocated_blocks[allocatedBlockIndex].size == new_size)
				{
					void* va = (void*)AllArrays.allocated_blocks[allocatedBlockIndex].va;
					if(holding_spinlock(&AllArrays.arrayslock))
					{
						release_spinlock(&(AllArrays.arrayslock));
					}
					return va;
				}
				// next block is free and size is increased (case 7.2.2)
				if(AllArrays.allocated_blocks[allocatedBlockIndex].size < new_size)
				{
					int noOfFramesToAllocate = (new_size - AllArrays.allocated_blocks[allocatedBlockIndex].size) / PAGE_SIZE;
					uint32 iterator = AllArrays.allocated_blocks[allocatedBlockIndex].va + AllArrays.allocated_blocks[allocatedBlockIndex].size;
					// can fit in
					if(new_size <= AllArrays.allocated_blocks[allocatedBlockIndex].size + AllArrays.free_blocks[freeBlockIndex].size)
					{
						if (AllArrays.free_blocks[freeBlockIndex].size == new_size - AllArrays.allocated_blocks[allocatedBlockIndex].size)
						{
							memmove(&AllArrays.free_blocks[freeBlockIndex], &AllArrays.free_blocks[freeBlockIndex + 1], (AllArrays.free_count - freeBlockIndex - 1) * sizeof(struct FreeBlock));
							AllArrays.free_count--;
						}
						else
						{
							AllArrays.free_blocks[freeBlockIndex].size -= (new_size - AllArrays.allocated_blocks[allocatedBlockIndex].size);
							AllArrays.free_blocks[freeBlockIndex].va += (new_size - AllArrays.allocated_blocks[allocatedBlockIndex].size);
						}

						AllArrays.allocated_blocks[allocatedBlockIndex].size = new_size;

						for (uint32 i = 0 ; i < noOfFramesToAllocate; i++)
						{
							struct FrameInfo *ptr_frame_info;
							allocate_frame(&ptr_frame_info);
							map_frame(ptr_page_directory, ptr_frame_info, iterator, PERM_WRITEABLE);
							iterator += PAGE_SIZE;
						}
						void* va = (void*)AllArrays.allocated_blocks[allocatedBlockIndex].va;
						if(holding_spinlock(&AllArrays.arrayslock))
						{
							release_spinlock(&(AllArrays.arrayslock));
						}
						return va;
					}
					// cannot fit in
					else
					{
						int copySize = AllArrays.allocated_blocks[allocatedBlockIndex].size;
						void* newVA = kmalloc(new_size);
						if(newVA)
						{
							memcpy(newVA, virtual_address, copySize);
							kfree(virtual_address);
							if(holding_spinlock(&AllArrays.arrayslock))
							{
								release_spinlock(&(AllArrays.arrayslock));
							}
							return newVA;
						}
						else
						{
							if(holding_spinlock(&AllArrays.arrayslock))
							{
								release_spinlock(&(AllArrays.arrayslock));
							}
							return NULL;
						}
					}
				}
				// next block is free and size is decreased (case 7.2.3)
				else
				{
					int noOfFramesToDellaocate = (AllArrays.allocated_blocks[allocatedBlockIndex].size - new_size) / PAGE_SIZE;

					AllArrays.free_blocks[freeBlockIndex].size += (AllArrays.allocated_blocks[allocatedBlockIndex].size - new_size);
					AllArrays.free_blocks[freeBlockIndex].va -= (AllArrays.allocated_blocks[allocatedBlockIndex].size - new_size);

					AllArrays.allocated_blocks[allocatedBlockIndex].size = new_size;

					uint32 iterator = AllArrays.allocated_blocks[allocatedBlockIndex].va + AllArrays.allocated_blocks[allocatedBlockIndex].size;


					for (uint32 i = 0; i < noOfFramesToDellaocate; i++)
					{
						unmap_frame(ptr_page_directory, iterator);
						iterator += PAGE_SIZE;
					}
					void* va = (void*)AllArrays.allocated_blocks[allocatedBlockIndex].va;
					if(holding_spinlock(&AllArrays.arrayslock))
					{
						release_spinlock(&(AllArrays.arrayslock));
					}
					return va;
				}
			}
		}
	}
	if(holding_spinlock(&AllArrays.arrayslock))
	{
		release_spinlock(&(AllArrays.arrayslock));
	}
	return virtual_address;
}

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
		frames_virtual_addresses[(kheap_physical_address(iterator)/4096)]=iterator;
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
		frames_virtual_addresses[(kheap_physical_address(iterator)/4096)]=iterator;
		iterator += 4096;
	}

	uint32* last_footer = (uint32*)(segmentBreak - 2*sizeof(int));
	uint32 last_block_size = *last_footer - (*last_footer % 2 == 0 ? 0 : 1);
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
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	if (isKHeapPlacementStrategyFIRSTFIT())
    {
        if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
        {
            return alloc_block_FF(size);
        }
        else
        {
            size = ROUNDUP(size, PAGE_SIZE);
            uint32 noOfPagesToAllocate = size / PAGE_SIZE;
            uint32 allocationAddress;
            bool canAllocate = 0;

            if (block_count == 0)
            {
                if (size <= KERNEL_HEAP_MAX - hardLimit - PAGE_SIZE)
                {
                    allocationAddress = hardLimit + PAGE_SIZE;
                    allocated_blocks[0].va = allocationAddress;
                    allocated_blocks[0].size = size;
                    block_count = 1;

                    if (size < KERNEL_HEAP_MAX - hardLimit - PAGE_SIZE)
                    {
                        free_blocks[0].va = allocationAddress + size;
                        free_blocks[0].size = (KERNEL_HEAP_MAX - hardLimit - PAGE_SIZE) - size;
                        free_count = 1;
                    }
                    else
                    {
                        free_count = 0;
                    }

                    for (uint32 i = 0; i < noOfPagesToAllocate; i++) {
                        struct FrameInfo *ptr_frame_info;
                        allocate_frame(&ptr_frame_info);
                        map_frame(ptr_page_directory, ptr_frame_info, allocationAddress, PERM_WRITEABLE);
                        frames_virtual_addresses[(kheap_physical_address(allocationAddress)/4096)]=allocationAddress;
                        allocationAddress += PAGE_SIZE;
                    }

                    return (void*)(allocated_blocks[0].va);
                }
                else
                {
                    return NULL;
                }
            }

            for (uint32 i = 0; i < free_count; i++)
            {
                if (free_blocks[i].size >= size)
                {
                    allocationAddress = free_blocks[i].va;

                    if (free_blocks[i].size == size)
                    {
                        memmove(&free_blocks[i], &free_blocks[i + 1], (free_count - i - 1) * sizeof(struct FreeBlock));
                        free_count--;
                    }
                    else
                    {
                        free_blocks[i].size -= size;
                        free_blocks[i].va += size;
                    }

                    canAllocate = 1;
                    break;
                }
            }

            if (!canAllocate)
            {
            	return NULL;
            }

            uint32 left = 0;
            uint32 right = block_count;

            while (left < right)
            {
                uint32 mid = left + (right - left) / 2 ;
                if (allocated_blocks[mid].va < allocationAddress)
                {
                	left = mid + 1;
                }
                else
                {
                	right = mid;
                }
            }

            uint32 index = left;
            memmove(&allocated_blocks[index + 1], &allocated_blocks[index], (block_count - index) * sizeof(struct PageBlock));
            allocated_blocks[index].va = allocationAddress;
            allocated_blocks[index].size = size;
            block_count++;

            for (uint32 i = 0 ; i < noOfPagesToAllocate; i++)
            {
                struct FrameInfo *ptr_frame_info;
                allocate_frame(&ptr_frame_info);
                map_frame(ptr_page_directory, ptr_frame_info, allocationAddress, PERM_WRITEABLE);
                frames_virtual_addresses[(kheap_physical_address(allocationAddress)/4096)]=allocationAddress;
                allocationAddress += PAGE_SIZE;
            }

            return (void*)(allocationAddress - (PAGE_SIZE * noOfPagesToAllocate));
        }
    }

    return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code

    uint32 address = (uint32)virtual_address;

    if (address > start && address < hardLimit)
    {
        free_block(virtual_address);
        frames_virtual_addresses[(kheap_physical_address(address)/4096)]=0;
        return;
    }
    else if (address >= hardLimit + PAGE_SIZE && address < KERNEL_HEAP_MAX)
    {
        uint32 iterator = address;
        uint32 noOfFrames = 0;
        uint32 size = 0;
        int index = -1;

        int left = 0;
        int right = block_count - 1;
        while (left <= right)
        {
            int mid = left + (right - left) / 2;

            if (allocated_blocks[mid].va == address)
            {
                noOfFrames = allocated_blocks[mid].size / PAGE_SIZE;
                size = allocated_blocks[mid].size;
                index = mid;

                memmove(&allocated_blocks[mid], &allocated_blocks[mid + 1],
                        (block_count - mid - 1) * sizeof(struct PageBlock));
                block_count--;
                break;
            }
            else if (allocated_blocks[mid].va < address)
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
            panic("Invalid address: Block not found in allocated_blocks");
            return;
        }

        struct FreeBlock newFreeBlock;
        newFreeBlock.va = address;
        newFreeBlock.size = size;

        left = 0;
		right = free_count - 1;
		while (left <= right)
		{
			int mid = left + (right - left) / 2;

			if (free_blocks[mid].va < newFreeBlock.va)
			{
				left = mid + 1;
			}
			else
			{
				right = mid - 1;
			}
		}

		int insertIndex = left;

        if (free_count > 0 && insertIndex < free_count)
        {
            memmove(&free_blocks[insertIndex + 1], &free_blocks[insertIndex],
                    (free_count - insertIndex) * sizeof(struct FreeBlock));
        }
        free_blocks[insertIndex] = newFreeBlock;
        free_count++;

        if (insertIndex > 0)
        {
            struct FreeBlock* prevBlock = &free_blocks[insertIndex - 1];
            if (prevBlock->va + prevBlock->size == newFreeBlock.va)
            {
                prevBlock->size += newFreeBlock.size;
                memmove(&free_blocks[insertIndex], &free_blocks[insertIndex + 1],
                        (free_count - insertIndex - 1) * sizeof(struct FreeBlock));
                free_count--;
                insertIndex--;
            }
        }

        if (insertIndex < free_count - 1)
        {
            struct FreeBlock* nextBlock = &free_blocks[insertIndex + 1];
            if (newFreeBlock.va + newFreeBlock.size == nextBlock->va)
            {
                free_blocks[insertIndex].size += nextBlock->size;
                memmove(&free_blocks[insertIndex + 1], &free_blocks[insertIndex + 2],
                        (free_count - insertIndex - 2) * sizeof(struct FreeBlock));
                free_count--;
            }
        }

        for (uint32 i = 0; i < noOfFrames; i++)
        {
        	frames_virtual_addresses[(kheap_physical_address(iterator)/4096)]=0;
            unmap_frame(ptr_page_directory, iterator);
            iterator += PAGE_SIZE;
        }
    }
    else
    {
        panic("invalid address");
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
		return 0;

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

	if(frames_virtual_addresses[(physical_address/4096)] == 0)
		return 0;
	else
		return frames_virtual_addresses[(physical_address/4096)]+obtained_offset;

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

	// case 1
	if(virtual_address == NULL)
	{
		return kmalloc(new_size);
	}

	// case 2
	if(new_size == 0)
	{
		kfree(virtual_address);
		return NULL;
	}

	// case 3
	if(virtual_address == NULL && new_size == 0)
	{
		return NULL;
	}

	uint32 address = (uint32)virtual_address;

	// va is in block allocator
	if(address > start && address < hardLimit)
	{
		// block -> block (case 4)
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			realloc_block_FF(virtual_address, new_size);
		}
		// block -> heap (case 5)
		else
		{
			void* newVA = kmalloc(new_size);
			if(newVA)
			{
				memcpy(newVA, virtual_address, get_block_size(virtual_address) - 8);
				kfree(virtual_address);
			}
		}
	}

	// va is in page allocator
	else if(address >= hardLimit + PAGE_SIZE && address < KERNEL_HEAP_MAX)
	{
		// heap -> block (case 6)
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			void* newVA = kmalloc(new_size);
			if(newVA)
			{
				memcpy(newVA, virtual_address, new_size);
				kfree(virtual_address);
			}
		}
		// heap -> heap (case 7)
		else
		{
			new_size = ROUNDUP(new_size, PAGE_SIZE);

			int left = 0;
			int right = block_count - 1;
			int allocatedBlockIndex = -1;
			uint32 nextBlockVA;

			while (left <= right)
			{
				int mid = left + (right - left) / 2;

				if (allocated_blocks[mid].va == address)
				{
					allocatedBlockIndex = mid;
					nextBlockVA = allocated_blocks[mid].va + allocated_blocks[mid].size;
					break;
				}
				else if (allocated_blocks[mid].va < address)
				{
					left = mid + 1;
				}
				else
				{
					right = mid - 1;
				}
			}

			// special case (there is no next block then reallocate and if done free the old heap space)
			if(nextBlockVA == KERNEL_HEAP_MAX)
			{
				void* newVA = kmalloc(new_size);
				if(newVA)
				{
					memcpy(newVA, virtual_address, (allocated_blocks[allocatedBlockIndex].size > new_size ? new_size : allocated_blocks[allocatedBlockIndex].size));
					kfree(virtual_address);
				}
			}


			// next block is allocated (case 7.1)
			if(allocatedBlockIndex < block_count - 1 && allocated_blocks[allocatedBlockIndex+1].va == allocated_blocks[allocatedBlockIndex].va + allocated_blocks[allocatedBlockIndex].size)
			{
				// next block is allocated and size is equal (case 7.1.1)
				if(allocated_blocks[allocatedBlockIndex].size == new_size) {
					return NULL;
				}
				// next block is allocated and size is increased (case 7.1.2)
				else if(allocated_blocks[allocatedBlockIndex].size < new_size)
				{
					void* newVA = kmalloc(new_size);
					if(newVA)
					{
						memcpy(newVA, virtual_address, allocated_blocks[allocatedBlockIndex].size);
						kfree(virtual_address);
					}
				}
				// next block is allocated and size is decreased (case 7.1.3)
				// then add a new free block in between after removing the extra space
				else
				{
					int noOfFramesToDellaocate = (allocated_blocks[allocatedBlockIndex].size - new_size) / PAGE_SIZE;

					struct FreeBlock newFreeBlock;
					newFreeBlock.va = allocated_blocks[allocatedBlockIndex].va + new_size;
					newFreeBlock.size = allocated_blocks[allocatedBlockIndex].size - new_size;

					left = 0;
					right = free_count - 1;
					while (left <= right)
					{
						int mid = left + (right - left) / 2;

						if (free_blocks[mid].va < newFreeBlock.va)
						{
							left = mid + 1;
						}
						else
						{
							right = mid - 1;
						}
					}

					int insertIndex = left;

					if (free_count > 0 && insertIndex < free_count)
					{
						memmove(&free_blocks[insertIndex + 1], &free_blocks[insertIndex],
								(free_count - insertIndex) * sizeof(struct FreeBlock));
					}
					free_blocks[insertIndex] = newFreeBlock;
					free_count++;

					allocated_blocks[allocatedBlockIndex].size = new_size;

					uint32 iterator = allocated_blocks[allocatedBlockIndex].va + allocated_blocks[allocatedBlockIndex].size;

					for (uint32 i = 0; i < noOfFramesToDellaocate; i++)
					{
						frames_virtual_addresses[(kheap_physical_address(iterator)/4096)]=0;
						unmap_frame(ptr_page_directory, iterator);
						iterator += PAGE_SIZE;
					}

					return (void*) allocated_blocks[allocatedBlockIndex].va;
				}
			}
			// next block is free (case 7.2)
			else
			{
				int left = 0;
				int right = block_count - 1;
				int freeBlockIndex = -1;
				while (left <= right)
				{
					int mid = left + (right - left) / 2;

					if (free_blocks[mid].va == nextBlockVA)
					{
						freeBlockIndex = mid;
						break;
					}
					else if (free_blocks[mid].va < nextBlockVA)
					{
						left = mid + 1;
					}
					else
					{
						right = mid - 1;
					}
				}
				// next block is free and size is equal (case 7.2.1)
				if(allocated_blocks[allocatedBlockIndex].size == new_size)
				{
					return (void*) allocated_blocks[allocatedBlockIndex].va;
				}
				// next block is free and size is increased (case 7.2.2)
				else if(allocated_blocks[allocatedBlockIndex].size < new_size)
				{
					int noOfFramesToAllocate = new_size - allocated_blocks[allocatedBlockIndex].size;
					uint32 iterator = allocated_blocks[allocatedBlockIndex].va + allocated_blocks[allocatedBlockIndex].size;
					// can fit in
					if(new_size <= allocated_blocks[allocatedBlockIndex].size + free_blocks[freeBlockIndex].size)
					{
						if (free_blocks[freeBlockIndex].size == new_size - allocated_blocks[allocatedBlockIndex].size)
						{
							memmove(&free_blocks[freeBlockIndex], &free_blocks[freeBlockIndex + 1], (free_count - freeBlockIndex - 1) * sizeof(struct FreeBlock));
							free_count--;
						}
						else
						{
							free_blocks[freeBlockIndex].size -= (new_size - allocated_blocks[allocatedBlockIndex].size);
							free_blocks[freeBlockIndex].va += (new_size - allocated_blocks[allocatedBlockIndex].size);
						}

						allocated_blocks[allocatedBlockIndex].size = new_size;

						for (uint32 i = 0 ; i < noOfFramesToAllocate; i++)
						{
							struct FrameInfo *ptr_frame_info;
							allocate_frame(&ptr_frame_info);
							map_frame(ptr_page_directory, ptr_frame_info, iterator, PERM_WRITEABLE);
							frames_virtual_addresses[(kheap_physical_address(iterator)/4096)]=iterator;
							iterator += PAGE_SIZE;
						}
					}
					// cannot fit
					else
					{
						void* newVA = kmalloc(new_size);
						if(newVA)
						{
							memcpy(newVA, virtual_address, allocated_blocks[allocatedBlockIndex].size);
							kfree(virtual_address);
						}
					}
				}
				// next block is free and size is decreased (case 7.2.3)
				else
				{
					int noOfFramesToDellaocate = (allocated_blocks[allocatedBlockIndex].size - new_size) / PAGE_SIZE;

					free_blocks[freeBlockIndex].size += (allocated_blocks[allocatedBlockIndex].size - new_size);
					free_blocks[freeBlockIndex].va -= (allocated_blocks[allocatedBlockIndex].size - new_size);

					allocated_blocks[allocatedBlockIndex].size = new_size;

					uint32 iterator = allocated_blocks[allocatedBlockIndex].va + allocated_blocks[allocatedBlockIndex].size;


					for (uint32 i = 0; i < noOfFramesToDellaocate; i++)
					{
						frames_virtual_addresses[(kheap_physical_address(iterator)/4096)]=0;
						unmap_frame(ptr_page_directory, iterator);
						iterator += PAGE_SIZE;
					}

				}
				return (void*) allocated_blocks[allocatedBlockIndex].va;
			}
		}
	}

	return NULL;
}

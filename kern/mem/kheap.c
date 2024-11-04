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

void* kmalloc(unsigned int size) {
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
                        if (allocate_frame(&ptr_frame_info) != 0) {
                            return NULL;
                        }
                        map_frame(ptr_page_directory, ptr_frame_info, allocationAddress, PERM_WRITEABLE);
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

            uint32 low = 0;
            uint32 high = block_count;

            while (low < high)
            {
                uint32 mid = low + (high - low) / 2 ;
                if (allocated_blocks[mid].va < allocationAddress)
                {
                    low = mid + 1;
                }
                else
                {
                    high = mid;
                }
            }

            uint32 index = low;
            memmove(&allocated_blocks[index + 1], &allocated_blocks[index], (block_count - index) * sizeof(struct PageBlock));
            allocated_blocks[index].va = allocationAddress;
            allocated_blocks[index].size = size;
            block_count++;

            for (uint32 i = 0 ; i < noOfPagesToAllocate; i++)
            {
                struct FrameInfo *ptr_frame_info;
                allocate_frame(&ptr_frame_info);
                map_frame(ptr_page_directory, ptr_frame_info, allocationAddress, PERM_WRITEABLE);
                allocationAddress += PAGE_SIZE;
            }

            return (void*)(allocationAddress - (PAGE_SIZE * noOfPagesToAllocate)); // Return the starting address of the allocated memory
        }
    }

    return NULL;
}

void kfree(void* virtual_address) {
    uint32 address = (uint32)virtual_address;

    if (address > start && address < hardLimit)
    {
        free_block(virtual_address);
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

        int insertIndex = 0;
        while (insertIndex < free_count && free_blocks[insertIndex].va < newFreeBlock.va)
        {
            insertIndex++;
        }

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
            unmap_frame(ptr_page_directory, iterator);
            iterator += PAGE_SIZE;
        }
    }
    else
    {
        panic("invalid address");
    }
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

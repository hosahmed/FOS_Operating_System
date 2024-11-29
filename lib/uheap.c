#include <inc/lib.h>

//////////////////////////////////////////////////////
// Fast Page Allocator Structs

struct EnvPageBlock {
	uint32 va;
	uint32 size;
};

struct EnvFreeBlock {
	uint32 va;
	uint32 size;
};

//////////////////////////////////////////////////////

//////////////////////////////////////////////////////
// Fast Page Allocator Data Structures

struct EnvPageBlock env_allocated_blocks[NUM_OF_UHEAP_PAGES];
struct EnvFreeBlock env_free_blocks[NUM_OF_UHEAP_PAGES];
uint32 sharedObjectsIDs[NUM_OF_UHEAP_PAGES];
uint32 env_block_count;
uint32 env_free_count;

//////////////////////////////////////////////////////

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code

	if (sys_isUHeapPlacementStrategyFIRSTFIT())
	{
		if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			sys_allocate_user_mem(myEnv->segment_break, size);
			return (alloc_block_FF(size));
		}
		else
		{

			size = ROUNDUP(size, PAGE_SIZE);
			uint32 noOfPagesToAllocate = size / PAGE_SIZE;
			uint32 allocationAddress;
			bool canAllocate = 0;

			if (env_block_count == 0)
			{
				if (size <= USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE)
				{
					allocationAddress = myEnv->hard_limit + PAGE_SIZE;
					env_allocated_blocks[0].va = allocationAddress;
					env_allocated_blocks[0].size = size;
					env_block_count = 1;

					if (size < USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE)
					{
						env_free_blocks[0].va = allocationAddress + size;
						env_free_blocks[0].size = (USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE) - size;
						env_free_count = 1;
					}
					else
					{
						env_free_count = 0;
					}

					sys_allocate_user_mem(allocationAddress, size);

					return (void*) allocationAddress;
				}
				else
				{
					return NULL;
				}
			}

			for (uint32 i = 0; i < env_free_count; i++)
			{
				if (env_free_blocks[i].size >= size)
				{
					allocationAddress = env_free_blocks[i].va;

					if (env_free_blocks[i].size == size)
					{
						for (int j = i; j < env_free_count - 1; j++) {
							env_free_blocks[j] = env_free_blocks[j + 1];
						}
						env_free_count--;
					}
					else
					{
						env_free_blocks[i].size -= size;
						env_free_blocks[i].va += size;
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
			uint32 right = env_block_count;

			while (left < right)
			{
				uint32 mid = left + (right - left) / 2 ;
				if (env_allocated_blocks[mid].va < allocationAddress)
				{
					left = mid + 1;
				}
				else
				{
					right = mid;
				}
			}

			uint32 index = left;
			for (int j = env_block_count; j > index; j--) {
				env_allocated_blocks[j] = env_allocated_blocks[j - 1];
			}
			env_allocated_blocks[index].va = allocationAddress;
			env_allocated_blocks[index].size = size;
			env_block_count++;

			// MARKING
			sys_allocate_user_mem(allocationAddress, size);

			return (void*) allocationAddress;
		}
	}

	return NULL;
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code

	uint32 address = (uint32)virtual_address;

	if (address > myEnv->start && address < myEnv->hard_limit)
	{
		free_block(virtual_address);
		return;
	}
	else if (address >= myEnv->hard_limit + PAGE_SIZE && address < USER_HEAP_MAX)
	{
		uint32 iterator = address;
		uint32 noOfFrames = 0;
		uint32 size = 0;
		int index = -1;

		int left = 0;
		int right = env_block_count - 1;
		while (left <= right)
		{
			int mid = left + (right - left) / 2;

			if (env_allocated_blocks[mid].va == address)
			{
				noOfFrames = env_allocated_blocks[mid].size / PAGE_SIZE;
				size = env_allocated_blocks[mid].size;
				index = mid;

				memmove(&env_allocated_blocks[mid], &env_allocated_blocks[mid + 1],
						(env_block_count - mid - 1) * sizeof(struct EnvPageBlock));
				env_block_count--;
				break;
			}
			else if (env_allocated_blocks[mid].va < address)
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

		struct EnvFreeBlock newFreeBlock;
		newFreeBlock.va = address;
		newFreeBlock.size = size;

		left = 0;
		right = env_free_count - 1;
		while (left <= right)
		{
			int mid = left + (right - left) / 2;

			if (env_free_blocks[mid].va < newFreeBlock.va)
			{
				left = mid + 1;
			}
			else
			{
				right = mid - 1;
			}
		}

		int insertIndex = left;

		if (env_free_count > 0 && insertIndex < env_free_count)
		{
			memmove(&env_free_blocks[insertIndex + 1], &env_free_blocks[insertIndex],
					(env_free_count - insertIndex) * sizeof(struct EnvFreeBlock));
		}
		env_free_blocks[insertIndex] = newFreeBlock;
		env_free_count++;

		if (insertIndex > 0)
		{
			struct EnvFreeBlock* prevBlock = &env_free_blocks[insertIndex - 1];
			if (prevBlock->va + prevBlock->size == newFreeBlock.va)
			{
				prevBlock->size += newFreeBlock.size;
				memmove(&env_free_blocks[insertIndex], &env_free_blocks[insertIndex + 1],
						(env_free_count - insertIndex - 1) * sizeof(struct EnvFreeBlock));
				env_free_count--;
				insertIndex--;
			}
		}

		if (insertIndex < env_free_count - 1)
		{
			struct EnvFreeBlock* nextBlock = &env_free_blocks[insertIndex + 1];
			if (newFreeBlock.va + newFreeBlock.size == nextBlock->va)
			{
				env_free_blocks[insertIndex].size += nextBlock->size;
				memmove(&env_free_blocks[insertIndex + 1], &env_free_blocks[insertIndex + 2],
						(env_free_count - insertIndex - 2) * sizeof(struct EnvFreeBlock));
				env_free_count--;
			}
		}

	        sys_free_user_mem(address, size);

	    }
	    else
	    {
	        panic("invalid address");
	    }
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
	// Write your code here, remove the panic and write your code
	//panic("smalloc() is not implemented yet...!!");

	if(sys_isUHeapPlacementStrategyFIRSTFIT())
	{
		size = ROUNDUP(size, PAGE_SIZE);
		uint32 noOfPagesToAllocate = size / PAGE_SIZE;
		uint32 allocationAddress;
		bool canAllocate = 0;

		if (env_block_count == 0)
		{
			if (size <= USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE)
			{
				allocationAddress = myEnv->hard_limit + PAGE_SIZE;
				env_allocated_blocks[0].va = allocationAddress;
				env_allocated_blocks[0].size = size;
				env_block_count = 1;

				if (size < USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE)
				{
					env_free_blocks[0].va = allocationAddress + size;
					env_free_blocks[0].size = (USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE) - size;
					env_free_count = 1;
				}
				else
				{
					env_free_count = 0;
				}

				int Tst = sys_createSharedObject(sharedVarName, size,  isWritable,(void*) allocationAddress);

				if(Tst==E_SHARED_MEM_EXISTS || Tst==E_NO_SHARE)
				{
					return NULL;
				}
				sharedObjectsIDs[allocationAddress/PAGE_SIZE]=Tst;
				return (void*) allocationAddress;
			}
			else
			{
				return NULL;
			}
		}

		for (uint32 i = 0; i < env_free_count; i++)
		{
			if (env_free_blocks[i].size >= size)
			{
				allocationAddress = env_free_blocks[i].va;

				if (env_free_blocks[i].size == size)
				{
					for (int j = i; j < env_free_count - 1; j++) {
						env_free_blocks[j] = env_free_blocks[j + 1];
					}
					env_free_count--;
				}
				else
				{
					env_free_blocks[i].size -= size;
					env_free_blocks[i].va += size;
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
		uint32 right = env_block_count;

		while (left < right)
		{
			uint32 mid = left + (right - left) / 2 ;
			if (env_allocated_blocks[mid].va < allocationAddress)
			{
				left = mid + 1;
			}
			else
			{
				right = mid;
			}
		}

		uint32 index = left;
		for (int j = env_block_count; j > index; j--) {
			env_allocated_blocks[j] = env_allocated_blocks[j - 1];
		}
		env_allocated_blocks[index].va = allocationAddress;
		env_allocated_blocks[index].size = size;
		env_block_count++;
		int Tst = sys_createSharedObject(sharedVarName, size,  isWritable,(void*) allocationAddress);
		if(Tst==E_SHARED_MEM_EXISTS || Tst==E_NO_SHARE)
		{
			return NULL;
		}
		sharedObjectsIDs[allocationAddress/PAGE_SIZE]=Tst;

		return (void*)allocationAddress;
	}
	return NULL;

}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	// Write your code here, remove the panic and write your code
	//panic("sget() is not implemented yet...!!");
	int size = sys_getSizeOfSharedObject(ownerEnvID,sharedVarName);
	if(size==E_SHARED_MEM_NOT_EXISTS)
	{
		return NULL;
	}

	if(sys_isUHeapPlacementStrategyFIRSTFIT())
	{
		size = ROUNDUP(size, PAGE_SIZE);
		uint32 noOfPagesToAllocate = size / PAGE_SIZE;
		uint32 allocationAddress;
		bool canAllocate = 0;

		if (env_block_count == 0)
		{
			allocationAddress = myEnv->hard_limit + PAGE_SIZE;
			env_allocated_blocks[0].va = allocationAddress;
			env_allocated_blocks[0].size = size;
			env_block_count = 1;

			if (size < USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE)
			{
				env_free_blocks[0].va = allocationAddress + size;
				env_free_blocks[0].size = (USER_HEAP_MAX - myEnv->hard_limit - PAGE_SIZE) - size;
				env_free_count = 1;
			}
			else
			{
				env_free_count = 0;
			}

			int Tst = sys_getSharedObject(ownerEnvID, sharedVarName,(void*)allocationAddress);

			//check the return of Tst
			if(Tst==E_SHARED_MEM_NOT_EXISTS )
			{
				cprintf("\nOBJECT NOT FOUND!!!!!!!!!\n");
				return NULL;
			}
			sharedObjectsIDs[allocationAddress/PAGE_SIZE]=Tst;
			return (void*) allocationAddress;
		}

		for (uint32 i = 0; i < env_free_count; i++)
		{
			if (env_free_blocks[i].size >= size)
			{
				allocationAddress = env_free_blocks[i].va;

				if (env_free_blocks[i].size == size)
				{
					for (int j = i; j < env_free_count - 1; j++) {
						env_free_blocks[j] = env_free_blocks[j + 1];
					}
					env_free_count--;
				}
				else
				{
					env_free_blocks[i].size -= size;
					env_free_blocks[i].va += size;
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
		uint32 right = env_block_count;

		while (left < right)
		{
			uint32 mid = left + (right - left) / 2 ;
			if (env_allocated_blocks[mid].va < allocationAddress)
			{
				left = mid + 1;
			}
			else
			{
				right = mid;
			}
		}

		uint32 index = left;
		for (int j = env_block_count; j > index; j--) {
			env_allocated_blocks[j] = env_allocated_blocks[j - 1];
		}
		env_allocated_blocks[index].va = allocationAddress;
		env_allocated_blocks[index].size = size;
		env_block_count++;

		int Tst = sys_getSharedObject(ownerEnvID, sharedVarName,(void*)allocationAddress);

		if(Tst==E_SHARED_MEM_NOT_EXISTS)
		{
			return NULL;
		}
		sharedObjectsIDs[allocationAddress/PAGE_SIZE]=Tst;
		return (void*)allocationAddress;
	}

	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	//panic("sfree() is not implemented yet...!!");

	uint32 address = (uint32)virtual_address;

	uint32 iterator = address;
	uint32 size = 0;
	int index = -1;

	int left = 0;
	int right = env_block_count - 1;
	while (left <= right)
	{
		int mid = left + (right - left) / 2;

		if (env_allocated_blocks[mid].va == address)
		{
			size = env_allocated_blocks[mid].size;
			index = mid;

			memmove(&env_allocated_blocks[mid], &env_allocated_blocks[mid + 1],
					(env_block_count - mid - 1) * sizeof(struct EnvPageBlock));
			env_block_count--;
			break;
		}
		else if (env_allocated_blocks[mid].va < address)
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

	struct EnvFreeBlock newFreeBlock;
	newFreeBlock.va = address;
	newFreeBlock.size = size;

	left = 0;
	right = env_free_count - 1;
	while (left <= right)
	{
		int mid = left + (right - left) / 2;

		if (env_free_blocks[mid].va < newFreeBlock.va)
		{
			left = mid + 1;
		}
		else
		{
			right = mid - 1;
		}
	}

	int insertIndex = left;

	if (env_free_count > 0 && insertIndex < env_free_count)
	{
		memmove(&env_free_blocks[insertIndex + 1], &env_free_blocks[insertIndex],
				(env_free_count - insertIndex) * sizeof(struct EnvFreeBlock));
	}
	env_free_blocks[insertIndex] = newFreeBlock;
	env_free_count++;

	if (insertIndex > 0)
	{
		struct EnvFreeBlock* prevBlock = &env_free_blocks[insertIndex - 1];
		if (prevBlock->va + prevBlock->size == newFreeBlock.va)
		{
			prevBlock->size += newFreeBlock.size;
			memmove(&env_free_blocks[insertIndex], &env_free_blocks[insertIndex + 1],
					(env_free_count - insertIndex - 1) * sizeof(struct EnvFreeBlock));
			env_free_count--;
			insertIndex--;
		}
	}

	if (insertIndex < env_free_count - 1)
	{
		struct EnvFreeBlock* nextBlock = &env_free_blocks[insertIndex + 1];
		if (newFreeBlock.va + newFreeBlock.size == nextBlock->va)
		{
			env_free_blocks[insertIndex].size += nextBlock->size;
			memmove(&env_free_blocks[insertIndex + 1], &env_free_blocks[insertIndex + 2],
					(env_free_count - insertIndex - 2) * sizeof(struct EnvFreeBlock));
			env_free_count--;
		}
	}

	uint32 sharedVarID=sharedObjectsIDs[((uint32)virtual_address)/PAGE_SIZE];
	sys_freeSharedObject(sharedVarID,virtual_address);
	sharedObjectsIDs[((uint32)virtual_address)/PAGE_SIZE] = 0;

}


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}

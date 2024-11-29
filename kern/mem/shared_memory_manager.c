#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");


	//need to be review
	struct FrameInfo** ArrSharedFrames = kmalloc(sizeof(int)*numOfFrames);

	if(ArrSharedFrames==NULL)
	{
		return NULL;
	}
	for (int i=0;i<numOfFrames;i++)
	{
		ArrSharedFrames[i]=0;
	}


	return ArrSharedFrames;

}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");



	//need to be review
	uint32 RoundUpsize= ROUNDUP(size, PAGE_SIZE);

	struct Share* sharedObject = kmalloc(sizeof(struct Share));

	if(sharedObject== NULL)
	{
		return NULL;
	}

	sharedObject->ID= (uint32)sharedObject & 0x7FFFFFFF;
	sharedObject->framesStorage = create_frames_storage(RoundUpsize/PAGE_SIZE);
	sharedObject->isWritable = isWritable;

	if(sharedObject->framesStorage == NULL)
	{
		kfree((void*)sharedObject);
		return NULL;
	}

	for(int i=0;i<strlen(shareName) ;i++)
	{
		sharedObject->name[i]=shareName[i];
	}

	sharedObject->ownerID = ownerID;
	sharedObject->references = 1;
	sharedObject->size = size;


	return sharedObject;

}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name)
{
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");

	// check if the time of this code consider long or smoll
	// may need lock in this func bec of search at the time of deleting
	acquire_spinlock(&(AllShares.shareslock));
	struct Share* sharedObjectInList;
	LIST_FOREACH(sharedObjectInList,&(AllShares.shares_list))
	{
		if(sharedObjectInList->ownerID == ownerID && strcmp(sharedObjectInList->name,name)==0 && strlen(sharedObjectInList->name) == strlen(name))
		{
			release_spinlock(&(AllShares.shareslock));
			return sharedObjectInList;
		}
	}

	release_spinlock(&(AllShares.shareslock));

	return NULL;
}

//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	size = ROUNDUP(size, PAGE_SIZE);
	struct Env* myenv = get_cpu_proc();

	if(get_share(ownerID,shareName)!=NULL)
	{
		return E_SHARED_MEM_EXISTS;
	}

	if(myenv->counterForSharedObj == 100)
	{
		return E_NO_SHARE;
	}

	struct Share* SharedObj = create_share(ownerID,shareName,size,isWritable);

	if(SharedObj== NULL)
	{
		return E_NO_SHARE;
	}

	acquire_spinlock(&(AllShares.shareslock));
	LIST_INSERT_TAIL(&(AllShares.shares_list),SharedObj);
	release_spinlock(&(AllShares.shareslock));

	uint32 noOfPagesToAllocate = size/PAGE_SIZE;

	// Allocating
	for (uint32 i = 0 ; i < noOfPagesToAllocate; i++)
	{
		struct FrameInfo *ptr_frame_info;
		allocate_frame(&ptr_frame_info);
		uint32* ptr_page_table = NULL;
		map_frame(myenv->env_page_directory, ptr_frame_info, (uint32)virtual_address, PERM_WRITEABLE | PERM_USER);
		SharedObj->framesStorage[i] = ptr_frame_info;
		virtual_address += PAGE_SIZE;
	}
	myenv->counterForSharedObj++;

	return SharedObj->ID;

}


//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here...


	struct Env* myenv = get_cpu_proc();//The calling environment
	struct Share* SharedObj= get_share(ownerID,shareName);

	if(SharedObj==NULL)
	{
		return E_SHARED_MEM_NOT_EXISTS;
	}
	SharedObj->references+=1;
	int sizeRoundUp= ROUNDUP(SharedObj->size,PAGE_SIZE);
	int nbOfFrame = sizeRoundUp/PAGE_SIZE;

	bool checkWritable;

	if(SharedObj->isWritable==1)
	{
		checkWritable=1;
	}
	else
	{
		checkWritable=0;
	}

	for(int i=0;i<nbOfFrame;i++)
	{
		if(checkWritable)
		{
			map_frame(myenv->env_page_directory, SharedObj->framesStorage[i], (uint32)virtual_address, PERM_WRITEABLE | PERM_USER);
		}
		else
		{
			map_frame(myenv->env_page_directory, SharedObj->framesStorage[i], (uint32)virtual_address, PERM_USER);
		}

		virtual_address+=PAGE_SIZE;
	}

	return SharedObj->ID;

}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================



//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_share is not implemented yet");
	acquire_spinlock(&(AllShares.shareslock));
	LIST_REMOVE(&(AllShares.shares_list), ptrShare);
	release_spinlock(&(AllShares.shareslock));

	kfree(ptrShare->framesStorage);
	kfree(ptrShare);
}
//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("freeSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* current_env = get_cpu_proc();
	struct Share* sharedObjectInList,*sharedObjectToFree;

	acquire_spinlock(&(AllShares.shareslock));
	LIST_FOREACH(sharedObjectInList,&(AllShares.shares_list))
	{
		if(sharedObjectInList->ID == sharedObjectID)
		{
			sharedObjectToFree=sharedObjectInList;
		}
	}
	release_spinlock(&(AllShares.shareslock));

	uint32 iterator = (uint32)startVA;

	uint32 noOfFrames = ROUNDUP(sharedObjectToFree->size, PAGE_SIZE)/PAGE_SIZE;

	uint32 *ptr_page_table;

	for (uint32 i = 0; i < noOfFrames; i++)
	{
		get_page_table(current_env->env_page_directory, iterator, &ptr_page_table);

		unmap_frame(current_env->env_page_directory, iterator);

		uint32 empty = 1;
		for (int j = 0; j < 1024; j++) {
			if (ptr_page_table[j] != 0) {
				empty = 0;
				break;
			}
		}
		if (empty == 1) {
			current_env->env_page_directory[PDX(iterator)] = 0;
			unmap_frame(current_env->env_page_directory, (uint32)ptr_page_table);
		}

		iterator += PAGE_SIZE;
	}

	sharedObjectToFree->references--;

	if(sharedObjectToFree->references == 0)
	{
		free_share(sharedObjectToFree);
	}
	return 0;
}

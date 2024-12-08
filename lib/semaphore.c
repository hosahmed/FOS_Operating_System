// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_semaphore is not implemented yet");
	//CHECK FOR WRITABLE
	uint32 size =sizeof(struct semaphore)+sizeof(struct __semdata);

	struct semaphore created_semaphore;
	struct __semdata* created_semdata = smalloc(semaphoreName, sizeof(struct __semdata), 1);

	created_semdata->count=value;
	created_semdata->lock=0;
	strcpy(created_semdata->name,semaphoreName);

	sys_init_queue(&(created_semdata->queue));

	created_semaphore.semdata = created_semdata;

	return created_semaphore;
}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_semaphore is not implemented yet");
	//Your Code is Here...
//	struct semaphore *retrieved_semaphore=sget(ownerEnvID,semaphoreName);

	struct semaphore *retrieved_semaphore=sget(ownerEnvID,semaphoreName);

	return *retrieved_semaphore;

}

void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("wait_semaphore is not implemented yet");
	uint32 keyw = 1;
	//struct Env *current_env=(struct Env *)get_cpu_proc();

	do
	{
		xchg(&keyw, sem.semdata->lock);
	}while (keyw != 0);

	sem.semdata->count--;

	if (sem.semdata->count < 0)
	{
		//place this process in s.queue
		sys_enqueue(&(sem.semdata->queue),(struct Env *)myEnv);
		//block this process (must also set s.lock = 0)
		myEnv->env_status = ENV_BLOCKED;
		sem.semdata->lock = 0;
	}
	sem.semdata->lock = 0;
}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("signal_semaphore is not implemented yet");
	//Your Code is Here...
	uint32 keys = 1;
	do
	{
		xchg(&keys, sem.semdata->lock);
	}while (keys != 0);

	sem.semdata->count++;

	if (sem.semdata->count <= 0)
	{
		struct Env* retrieved_process = sys_dequeue(&(sem.semdata->queue));

	}
	sem.semdata->lock = 0;
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}

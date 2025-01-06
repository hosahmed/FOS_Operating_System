 


## About The Project  

The OS'24 Project offers an in-depth, practical approach to implementing an operating system, focusing on essential concepts such as memory management, process scheduling, synchronization, and fault handling. It prioritizes modular design and equips students with a solid grounding in systems programming.

---  

## Modules  

### Command Prompt  
- A simple command-line interface to execute kernel-level commands.  
- Supports commands like `kernel_info`, `nclock`, and others for testing and interaction.  

### System Calls  
- Implemented system call interface to link user programs with kernel functions. 
- Validates user-provided pointers and ensures proper handling of invalid memory access.  
- Provides key system calls like read, write, open, and close for file management.

### Dynamic Allocator  
- **Kernel Heap**: Implemented block and page-level allocators for dynamic memory allocation.  
  - **First-Fit Strategy**: Used for both block and page allocations.  
  - Supports functions like `kmalloc`, `kfree`.  
- **User Heap**: Memory allocation for user programs with lazy allocation using system calls.  

### Memory Manager  
- **Page Allocator**: Handles memory on a page-level granularity for efficient management.  
- **Shared Memory**: Allows multiple processes to share memory regions for interprocess communication.  

### Fault Handler  
- **Page Fault Handling**: Implements lazy allocation and replacement policies to manage memory faults efficiently.  
- **Nth Chance Clock Replacement**: Optimized page replacement algorithm for balancing performance and memory utilization.  

### (Locks & Semaphores)  
- **Locks**:  
  - Implemented spinlocks for short critical sections with busy-waiting.  
  - Designed sleeplocks to handle longer critical sections by blocking threads and avoiding CPU wastage.  
  - Ensures safe access to shared kernel resources.  
- **Semaphores**:  
  - User-level semaphores to synchronize processes.  
  - Handles common issues like deadlocks and priority inversion with proper locking mechanisms.  

### CPU Scheduler  
- **Priority Round-Robin Scheduler**:  
  - Preemptive scheduling with multiple priority levels.  
  - Prevents starvation by promoting processes based on their waiting time.



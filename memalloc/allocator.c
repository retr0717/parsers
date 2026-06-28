#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>

#define MMAP_THRESHOLD (128*1024)//threshold.
pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;

void *memalloc(size_t);
void mfree(void*);
void printlist();
void coalesce();
void *mcalloc(size_t, size_t);
void *mrealloc(void*, size_t);

//block header.
typedef struct block{
	size_t size;
	int free;
	int is_mmap;
	struct block *next;
} block_t;

//linked list to keep track of blocks.
block_t *head = NULL;

int main()
{
	//allocate.
	void *p1 = memalloc(200);
	printf("Allocated : %p\n", p1);
	printlist();
	//free the mem.
	mfree(p1);

	//allocate another.
	void *p2 = memalloc(80);
	printf("Allocate 2 : %p\n", p2);//should be same addr as p1
	printlist();
	mfree(p2);

	//allocate 3.
	void *p3 = memalloc(120);
	printf("Allocate 3 : %p\n", p3);// should be of new addr.
	printlist();
	mfree(p3);

	void *p4 = mcalloc(4, 16);
	printf("mcalloc : %p\n", p4);
	printlist();
	mfree(p4);

	void *p5 = mrealloc(p4, 32);
	printf("realloc : %p\n", p5);
	printlist();
	mfree(p5);

	return 0;
}

//memallocator function.
void *memalloc(size_t size)
{
	//align size 8 or 16 bytes.
	size = (size + 7) & ~7;

	pthread_mutex_lock(&heap_lock);
	void *result = NULL;

	if(size >= MMAP_THRESHOLD)
	{
		block_t *block = mmap(NULL, sizeof(block_t) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if(block == MAP_FAILED)
		{
			result = NULL;
			goto done;
		}

		block->size = size;
		block->free = 0;
		block->is_mmap = 1;
		block->next = NULL;

		result = (void *)(block + 1);
		goto done;
	}


	//search existing blocks.
	block_t *curr = head;
	while(curr)
	{
		if(curr->free && curr->size >= size && curr->is_mmap == 0)
		{
			//split block logic.
			size_t remains = curr->size - size;
			if( curr->size >= (size + sizeof(block_t)) && remains >= (sizeof(block_t) + 8))
			{
				//create the new block.
				block_t *sblock = (block_t *)((char*)(curr+1)+size);
				sblock->size = remains - sizeof(block_t);
				sblock->free = 1;
				sblock->is_mmap = 0;
				sblock->next = curr->next;
				curr->next = sblock;
				curr->size = size;
			}
			curr->free  = 0;
			result = (void *)(curr + 1);
			goto done;
		}
		//go to the next block.
		curr = curr->next;
	}

	//create the block.
	block_t *new_blk = sbrk(sizeof(block_t) + size);
	if(new_blk == (void*)-1)
	{
		result = NULL;
		goto done;
	}

	new_blk->size = size;
	new_blk->free = 0;
	new_blk->is_mmap = 0;
	new_blk->next = NULL;

	if(head == NULL)
		head = new_blk;
	else
	{
		curr = head;
		while(curr->next)
			curr = curr->next;

		curr->next = new_blk;
	}
	result = (void*)(new_blk+1);
done:
	pthread_mutex_unlock(&heap_lock);

	return result;
}

//return the memory.
void mfree(void *ptr)
{
	if(ptr == NULL)
		return;

	pthread_mutex_lock(&heap_lock);

	block_t *block = (block_t*)ptr-1;
	if(block->is_mmap)
	{
		munmap(block, sizeof(block_t) + block->size);
		pthread_mutex_unlock(&heap_lock);
		return;
	}

	block->free = 1;
	coalesce();//freeing adjacent free blocks.

	pthread_mutex_unlock(&heap_lock);
}

void printlist()
{
	block_t *curr = head;

	int i = 1;
	while(curr)
	{
		printf("\n----------------\n");
		printf("Block %d\nSize : %lu\nFree : %d\nNext : %p\n", i, curr->size, curr->free, curr->next);
		curr = curr->next;
		printf("\n----------------\n");
	}
	printf("\n");
}

void coalesce()
{
	block_t *curr = head;
	while(curr && curr->next)
	{
		block_t *curr = head;
		//checking if adjacent mem location.
		uintptr_t expected = (uintptr_t)curr + sizeof(block_t) + curr->size;

		block_t *next = curr->next;
		if(curr->free && next->free && (uintptr_t)next == expected)
		{
			curr->size += sizeof(block_t) + next->size;
			curr->next = next->next;//linking the postNode of the current's next node.
		}
		else
			curr = curr->next;
	}
}

void *mcalloc(size_t n, size_t size)
{
	void *ptr = memalloc(n*size);
	memset(ptr, 0, n*size);
	return ptr;
}

void *mrealloc(void *ptr, size_t size)
{
	if(ptr == NULL)
		return memalloc(size);

	if(size == 0)
	{
		mfree(ptr);
		return NULL;
	}

	//align size.
	size = (size + 7) & ~7;

	block_t *block = (block_t *)ptr-1;

	//if blk is large enough.
	if(block->size >= size)
		return ptr;

	//expand into next free block.
	if(block->next && block->next->free && block->size + sizeof(block_t) + block->next->size >= size)
	{
		block_t *next = block->next;
		block->size += sizeof(block_t) + next->size;
		block->next = next->next;

		//remaining space.
		size_t remains = block->size - size;
		if(remains >= sizeof(block_t) + 8)
		{
			block_t *split = (block_t *)((char *)(block+1) + size);
			split->size = remains - sizeof(block_t);
			split->free = 1;
			split->next = block->next;
			block->next = split;
			block->size = size;
		}
		return ptr;
	}

	//allocate a new blk.
	void *nptr = memalloc(size);
	if(nptr == NULL)
		return NULL;

	memcpy(nptr, ptr, block->size);
	mfree(ptr);


	return nptr;
}

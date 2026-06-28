#include <stdio.h>
#include <unistd.h>

//problem with this implementation is that memory never returns.

void *my_alloc(size_t size)
{
	void *ptr = sbrk(size);

	if(ptr == (void*)-1)
		return NULL;

	return ptr;
}

int main()
{
	int *a = my_alloc(sizeof(int));
	*a = 123;

	printf("%d\n", *a);

	return 0;
}

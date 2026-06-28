#include <stdio.h>
#include <unistd.h>

int main()
{
	//sbrk(0) returns the location of the program break.
	void *before = sbrk(0);
	printf("Before : %p\n", before);
	sbrk(4096);
	void *after = sbrk(0);

	printf("After : %p\n", after);

	return 0;
}

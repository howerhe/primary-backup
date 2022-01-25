#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "../src/pool.h"

void *fun(void *args)
{
	long thread = (long)args;

	printf("running the thread of %ld\n", thread);

	return NULL;
}

int main(void)
{
	pool_t *pool = pool_init(5);
	if (pool == NULL) {
		printf("pool_init failed!\n");
		return -1;
	}

	for (long i = 0; i < 5000; i++)
		pool_add(pool, fun, (void *)i);

	sleep(2);
	pool_free(pool);
	pool = NULL;

	return 0;
}
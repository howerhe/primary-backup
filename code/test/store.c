#include <assert.h>
#include <stdio.h>

#include "../src/store.h"

int main(void)
{
	unsigned size = 10;
	int rv = -1;
	int value = 0;
	unsigned version = 0;

	// store_init().
	store_t s = store_init(size);
	assert(s);

	// store_read().
	rv = store_read(NULL, 0, &value, &version);
	assert(rv == -1);
	rv = store_read(s, size, &value, &version);
	assert(rv == -1);
	rv = store_read(s, 0, NULL, &version);
	assert(rv == -1);
	rv = store_read(s, 0, &value, NULL);
	assert(rv == -1);

	for (int i = 0; i < size; i++) {
		value = -1;
		version = 1;
		rv = store_read(s, i, &value, &version);
		assert(rv == 0);
		assert(value == 0);
		assert(version == 0);
	}

	// store_update(), store_read().
	rv = store_update(NULL, 0, &value, &version);
	assert(rv == -1);
	rv = store_update(s, size, &value, &version);
	assert(rv == -1);
	rv = store_update(s, 0, NULL, &version);
	assert(rv == -1);
	rv = store_update(s, 0, &value, NULL);
	assert(rv == -1);

	value = 3;
	version = 0;
	rv = store_update(s, 0, &value, &version);
	assert(rv == 0);
	assert(value == 3);
	assert(version == 1);
	value = 0;
	version = 0;
	rv = store_read(s, 0, &value, &version);
	assert(rv == 0);
	assert(value == 3);
	assert(version == 1);

	value = 5;
	version = 0;
	rv = store_update(s, 0, &value, &version);
	assert(rv == 0);
	assert(value == 5);
	assert(version == 2); // version is updated internally.
	value = 0;
	version = 0;
	rv = store_read(s, 0, &value, &version);
	assert(rv == 0);
	assert(value == 5);
	assert(version == 2);

	value = 2;
	version = 0;
	rv = store_update(s, 3, &value, &version);
	assert(rv == 0);
	assert(value == 2);
	assert(version == 1);
	value = 0;
	version = 0;
	rv = store_read(s, 3, &value, &version);
	assert(rv == 0);
	assert(value == 2);
	assert(version == 1);

	// store_synchronize(), store_read().
	rv = store_synchronize(NULL, 0, &value, &version);
	assert(rv == -1);
	rv = store_synchronize(s, size, &value, &version);
	assert(rv == -1);
	rv = store_synchronize(s, 0, NULL, &version);
	assert(rv == -1);
	rv = store_synchronize(s, 0, &value, NULL);
	assert(rv == -1);

	value = -5;
	version = 5;
	rv = store_synchronize(s, 5, &value, &version);
	assert(rv == 0);
	assert(value == -5);
	assert(version == 5);
	value = 0;
	version = 0;
	rv = store_read(s, 5, &value, &version);
	assert(rv == 0);
	assert(value == -5);
	assert(version == 5);

	value = 0;
	version = 6;
	rv = store_synchronize(s, 5, &value, &version);
	assert(rv == 0);
	assert(value == 0);
	assert(version == 6);
	value = 0;
	version = 0;
	rv = store_read(s, 5, &value, &version);
	assert(rv == 0);
	assert(value == 0);
	assert(version == 6);

	value = 99;
	version = 100;
	rv = store_synchronize(s, 0, &value, &version);
	assert(rv == 0);
	assert(value == 99);
	assert(version == 100);
	value = 0;
	version = 0;
	rv = store_read(s, 0, &value, &version);
	assert(rv == 0);
	assert(value == 99);
	assert(version == 100);

	// Lower version should not work.
	value = 10;
	version = 2;
	rv = store_synchronize(s, 5, &value, &version);
	assert(rv == 0);
	assert(value == 0);
	assert(version == 6);
	value = 0;
	version = 0;
	rv = store_read(s, 5, &value, &version);
	assert(rv == 0);
	assert(value == 0);
	assert(version == 6);

	// store_free().
	store_free(s);
	s = NULL;

	printf("All tests passed.\n");

	return 0;
}
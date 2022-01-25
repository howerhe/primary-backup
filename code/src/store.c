#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "store.h"

struct store_block {
	int value;
	unsigned version;
	pthread_rwlock_t lock;
};

struct store {
	unsigned size;
	struct store_block *head;
};

store_t store_init(unsigned size)
{
	store_t s = malloc(sizeof(struct store));
	if (s == NULL)
		goto error;

	s->size = size;
	s->head = malloc(sizeof(struct store_block) * s->size);
	if (s->head == NULL)
		goto error;

	for (int i = 0; i < s->size; i++) {
		struct store_block *b = s->head + i;
		b->value = 0;
		b->version = 0;
		if (pthread_rwlock_init(&(b->lock), NULL) != 0)
			goto error;
	}

	return s;

error:
	perror("store_init");
	store_free(s);
	s = NULL;
	return NULL;
}

void store_free(store_t s)
{
	if (s == NULL)
		return;

	if (s->head == NULL) {
		free(s);
		s = NULL;
		return;
	}

	for (int i = 0; i < s->size; i++)
		pthread_rwlock_destroy(&(s->head + i)->lock);
	free(s->head);
	s->head = NULL;
	free(s);
	s = NULL;
}

int store_read(store_t s, unsigned index, int *value, unsigned *version)
{
	char *err_msg = NULL;

	if (s == NULL) {
		err_msg = "s is NULL";
		goto error;
	}
	if (index >= s->size) {
		err_msg = "index is out of bound";
		goto error;
	}
	if (value == NULL) {
		err_msg = "value is NULL";
		goto error;
	}
	if (version == NULL) {
		err_msg = "version is NULL";
		goto error;
	}

	struct store_block *b = s->head + index;
	pthread_rwlock_rdlock(&(b->lock));
	*value = b->value;
	*version = b->version;
	pthread_rwlock_unlock(&(b->lock));

	return 0;

error:
	fprintf(stderr, "store_read: %s\n", err_msg);
	return -1;
}

int store_update(store_t s, unsigned index, int *value, unsigned *version)
{
	char *err_msg = NULL;

	if (s == NULL) {
		err_msg = "s is NULL";
		goto error;
	}
	if (index >= s->size) {
		err_msg = "index is out of bound";
		goto error;
	}
	if (value == NULL) {
		err_msg = "value is NULL";
		goto error;
	}
	if (version == NULL) {
		err_msg = "version is NULL";
		goto error;
	}

	struct store_block *b = s->head + index;
	pthread_rwlock_wrlock(&(b->lock));
	b->value = *value;
	b->version++;
	*version = b->version;
	pthread_rwlock_unlock(&(b->lock));

	return 0;

error:
	fprintf(stderr, "store_update: %s\n", err_msg);
	return -1;
}

int store_synchronize(store_t s, unsigned index, int *value, unsigned *version)
{
	char *err_msg = NULL;

	if (s == NULL) {
		err_msg = "s is NULL";
		goto error;
	}
	if (index >= s->size) {
		err_msg = "index is out of bound";
		goto error;
	}
	if (value == NULL) {
		err_msg = "value is NULL";
		goto error;
	}
	if (version == NULL) {
		err_msg = "version is NULL";
		goto error;
	}

	struct store_block *b = s->head + index;
	pthread_rwlock_wrlock(&(b->lock));
	if (*version > b->version) {
		b->value = *value;
		b->version = *version;
	} else {
		*value = b->value;
		*version = b->version;
	}
	pthread_rwlock_unlock(&(b->lock));

	return 0;

error:
	fprintf(stderr, "store_write: %s\n", err_msg);
	return -1;
}
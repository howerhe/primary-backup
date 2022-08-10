#ifndef STORE_H
#define STORE_H

/**
 * @brief Store consisting of data blocks. Each block contains a value and a
 * version. The initial value and version are both 0.
 *
 */
typedef struct store *store_t;

/**
 * @brief Initiate a store.
 *
 * @param size number of blocks
 * @return store, NULL when errors occur
 */
store_t store_init(unsigned size);

/**
 * @brief Free and clean up a store.
 *
 * @param s store
 */
void store_free(store_t s);

/**
 * @brief Read from a store at the given block index.
 *
 * @param s store
 * @param index block index
 * @param value pointer to the value, will be set with the value at that block
 * index
 * @param version pointer to the version, will be set with the version at that
 * block index
 * @return 0 when no error occurs
 */
int store_read(store_t s, unsigned index, int *value, unsigned *version);

/**
 * @brief Update a store at the block index with the new value. The version at
 * that block index increments by 1.
 *
 * @param s store
 * @param index block index
 * @param value pointer to the value replacing the value at that block index,
 * and will be set with the value at that block index
 * @param version pointer to the version to be set with the version at that
 * block index after increment
 * @return 0 when no error occurs
 */
int store_update(store_t s, unsigned index, int *value, unsigned *version);

/**
 * @brief Synchronize a store at the block index. It is used for force update a
 * block. The update can only succeed when the provided version is newer.
 *
 * @param s store
 * @param index block index
 * @param value pointer to the value replacing the value at that block index,
 * and will be set with the value at that block index after synchronization
 * @param version pointer to the version replacing the version at that block
 * index, and will be set with the version at that block index after
 * synchronization
 * @return 0 when no error occurs
 */
int store_synchronize(store_t s, unsigned index, int *value, unsigned *version);

#endif
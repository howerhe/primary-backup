#ifndef STORE_H
#define STORE_H

/**
 * @brief The store of data blocks. Each block contains a value and a version. The initial value and version are set to 0.
 * 
 */
typedef struct store *store_t;

/**
 * @brief Initiate a store.
 * 
 * @param size the number of blocks
 * @return store_t representing the store, NULL when errors occur
 */
store_t store_init(unsigned size);

/**
 * @brief Clean up a store.
 */
void store_free(store_t s);

/**
 * @brief Read from a store at a block index.
 * 
 * @param s the store
 * @param index the block index
 * @param value the value to be set with the data at that block index
 * @param version the version to be set with the data at that block index
 * @return int 0 when no error occurs
 */
int store_read(store_t s, unsigned index, int *value, unsigned *version);

/**
 * @brief Update a store at a block index with a new value. The version at the block index increments by 1.
 * 
 * @param s the store
 * @param index the block index
 * @param value the value replacing the value at that block index, and is set with the data at that block index
 * @param version the version to be set with the data at that block index after the incremention
 * @return int 0 when no error occurs
 */
int store_update(store_t s, unsigned index, int *value, unsigned *version);

/**
 * @brief Synchronize a store at a block index. It is used for force update a block. The update can only succeed when the version is newer.
 * 
 * @param s the store
 * @param index the block index
 * @param value the value replacing the value at that block index, and is set with the data at that block index
 * @param version the version replacing the version at that block index, and is set with the data at that block index
 * @return int  0 when no error occurs
 */
int store_synchronize(store_t s, unsigned index, int *value, unsigned *version);

#endif
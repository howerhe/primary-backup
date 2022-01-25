// Reference (in Chinese): https://zhuanlan.zhihu.com/p/44971598

#ifndef POOL_H
#define POOL_H

/**
 * @brief The thread pool to handle different tasks.
 * 
 */
typedef struct pool pool_t;

/**
 * @brief Initiate a thread pool with the size of the thread pool.
 * 
 * @param size the size of the thread pool
 * @return pool_t* representing the thread pool, NULL when errors occur
 */
pool_t *pool_init(unsigned size);

/**
 * @brief Clean up a thread pool.
 * 
 * @param pool the thread pool
 */
void pool_free(pool_t *pool);

/**
 * @brief Add a new task to a thread pool. A task contains a routine and an argument. The task will be executed if there is a free thread.
 * 
 * @param pool the thread pool
 * @param routine function pointer to the routine
 * @param args pointer to the arguments for the routine
 * @return int 0 when no error occurs
 */
int pool_add(pool_t *pool, void *(*routine)(void *), void *args);

#endif
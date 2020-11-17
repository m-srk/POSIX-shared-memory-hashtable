#ifndef __SERVER_UTILS_H__
#define __SERVER_UTILS_H__

#include <stdbool.h>
#include "hashtable.h"

#ifndef SERVER_THREAD_COUNT_CONF 
#define SERVER_THREAD_COUNT 50
#else
#define SERVER_THREAD_COUNT SERVER_THREAD_COUNT_CONF
#endif
 
#define SERVER_THREAD_SLEEP_MS 5

#define SEM_HT_QUERY_PROD "/sem-ht-producer"
#define SEM_HT_QUERY_CONS "/sem-ht-consumer"

//-------------------------------------------
// Server API
bool open_and_map_shm();
bool init_sems();
void print_err(const char* args);
void release_shm_segment();

//TODO rm void register_threadid_ptr(pthread_t* tptr);
void register_HT_instance(HashTable *);

// thread runner func
void* consumer_task_runner(void* args);
void* htquery_task_runner(void* args);

#endif //__SERVER_UTILS_H__

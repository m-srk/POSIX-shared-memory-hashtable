#ifndef __SHM_COMMON_H__
#define __SHM_COMMON_H__

#include <stdlib.h>
#include <fcntl.h>      // O_ constants
#include <sys/mman.h>   // shared memory API
#include <unistd.h>     // POSIX API
#include <semaphore.h>  // semaphore API

#include "hashtable.h" //HT

// HT query types
#define NUM_QUERY_TYPES 3

#define INSERT_QUERY 0
#define READ_QUERY 1
#define DELETE_QUERY 2

// sem names
#define LOGFILE "/tmp/tumproj.log"
#define SERVER_THREAD_MUTEX "/sem-server-mutex"

#define SEM_MUTEX_NAME "/sem-mutex"
#define SEM_PRODUCER_COUNT "/sem-producer-count"
#define SEM_CONSUMER_COUNT "/sem-consumer-count"

#define SHARED_MEM_NAME "/posix-shm-tumproj"

// Total num. of HT queries 
#define MAX_BUFFERS 10000

// misc. 
#ifndef IPC_FAILURE
#define IPC_FAILURE 1
#endif

#ifndef SEMAPHORE_FAILURE
#define SEMAPHORE_FAILURE -1
#endif

#define STRINGS_EQUAL 0

typedef struct hashtable_query {
    int ht_query;
    int key;
    int value;
    int response;
} hashtable_query_t;

struct shared_memory {
    hashtable_query_t hts[MAX_BUFFERS];
    int producer_index;
    int consumer_index;
};

#endif //__SHM_COMMON_H__
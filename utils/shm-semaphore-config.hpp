#ifndef __INCLUDED_SHM_CONFIG__
#define __INCLUDED_SHM_CONFIG__

#include <fcntl.h>      // O_ constants
#include <sys/mman.h>   // shared memory API
#include <unistd.h>     // POSIX API
#include <string>
#include <semaphore.h>  // semaphore API

// Buffer data structures
#ifndef IPC_FAILURE
#define IPC_FAILURE 1
#endif

#define MAX_BUFFERS (50)

#define SERVER_THREAD_COUNT 1

#define CLIENT_THREAD_COUNT 5
#define CLIENT_THREAD_SLEEP_MS 10

#define LOGFILE "/tmp/tumproj.log"
#define SEM_MUTEX_NAME "/sem-mutex"
#define SEM_PRODUCER_COUNT "/sem-producer-count"
#define SEM_CONSUMER_COUNT "/sem-consumer-count"
#define SHARED_MEM_NAME "/posix-shm-tumproj"


typedef struct hashtable_query {
    char ht_query[256];
    int key;
    char value[256];
    char response[256];
} hashtable_query_t;

struct shared_memory {
    hashtable_query_t hts [MAX_BUFFERS];
    int producer_index;
    int consumer_index;
};

typedef struct server_shm_data {
    struct shared_memory* shared_mem_ptr;
    int fd_shm;
} server_shm_data_t;

typedef struct server_sem_data {
    sem_t* mutex_sem;
    sem_t* producer_count_sem;
    sem_t* consumer_count_sem;
} server_sem_data_t;

typedef struct hashtable_worker_in {
    server_sem_data_t* sem_data;
    server_shm_data_t* shm_data;
} ht_worker_input_t;


//-------------------------------------------
// Server API
bool open_and_map_shm();
bool init_sems();
void print_err(const char* args);
void release_shm_segment();

server_shm_data_t* get_server_shm_data();
server_sem_data_t* get_server_semaphore_data();

//-------------------------------------------
// Client API
void set_rand_seed();
bool init_sems_client();
bool open_and_map_shm_client();
void* run_client_task_rand(void*);

//-------------------------------------------
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "include/server-utils.h"
#include "include/shm-common.h"


// globals start here
HashTable *ht;
//TODO rm server_shm_data_t s_shm_data;
pthread_t* tid_ptr;

// shm
int fd_shm;
struct shared_memory* shared_mem_ptr;

// semaphores
sem_t *mutex_sem, *producer_count_sem, *consumer_count_sem, *server_thread_mutex;
sem_t *ht_prod_sem, *ht_cons_sem;
bool _server_semphore_init = false;


HashTable* get_HT_instance()
{
    return ht;
}

void lock_shm_segment()
{   
    if (sem_wait(mutex_sem) == SEMAPHORE_FAILURE)
             print_err("sem_wait:mutex");
}

void release_shm_segment()
{
    // indicate shared memory segment is available
    if (sem_post (mutex_sem) == IPC_FAILURE)
        print_err("sem_post: mutex_sem");
}

bool init_sems()
{
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SERVER_THREAD_MUTEX);

    sem_unlink(SEM_PRODUCER_COUNT);
    sem_unlink(SEM_CONSUMER_COUNT);

    sem_unlink(SEM_HT_QUERY_PROD);
    sem_unlink(SEM_HT_QUERY_CONS);
    
    //  mutex for shared mem, mutex_sem with an initial value 0.
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        print_err("sem_open: mutex_sem");

    // server-only mutex
    if ((server_thread_mutex = sem_open (SERVER_THREAD_MUTEX, O_CREAT, 0660, 0)) == SEM_FAILED)
        print_err("sem_open: mutex_sem");

    // counting semaphore, indicating the number of available buffers. Initial value = MAX_BUFFERS
    if ((producer_count_sem = sem_open (SEM_PRODUCER_COUNT, O_CREAT, 0660, MAX_BUFFERS)) == SEM_FAILED)
        print_err ("sem_open: buffer_count");

    // counting semaphore, indicating the number of queries to be processed. Initial value = 0
    if ((consumer_count_sem = sem_open (SEM_CONSUMER_COUNT, O_CREAT, 0660, 0)) == SEM_FAILED)
        print_err ("sem_open");

    if ((ht_prod_sem = sem_open(SEM_HT_QUERY_PROD, O_CREAT, 0660, MAX_BUFFERS)) == SEM_FAILED)
        print_err("sem_open: ht_prod_sem");

    if ((ht_cons_sem = sem_open(SEM_HT_QUERY_CONS, O_CREAT, 0660, 0)) == SEM_FAILED)
        print_err("sem_open: ht_prod_sem");

    return true;
}   

bool open_and_map_shm()
{
    // Get shared memory 
    if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR | O_CREAT, 0660)) == -1)
        print_err ("shm_open");

    if (ftruncate (fd_shm, sizeof (struct shared_memory)) == -1)
       print_err ("ftruncate");
    
    if ((shared_mem_ptr = (struct shared_memory *)(mmap (NULL, 
                                                        sizeof (struct shared_memory), 
                                                        PROT_READ | PROT_WRITE,
                                                        MAP_SHARED,
                                                        fd_shm,
                                                        0
                                                        )
                                                    )
                                                ) == MAP_FAILED)
        print_err ("mmap");

    shared_mem_ptr->producer_index = shared_mem_ptr->consumer_index = 0;

    return true;
}

void register_HT_instance(HashTable *ht_instance)
{   
    ht = ht_instance;
}

void register_threadid_ptr(pthread_t* tptr)
{
    tid_ptr = tptr;
}

int execute_ht_query(hashtable_query_t htq)
{
    // printf("in execute ht\n");
    HashTable* ht = get_HT_instance();

    switch (htq.ht_query)
    {
    case INSERT_QUERY:
        hash_insert(ht, htq.key, htq.value);
        printf("Inserted key-value : %d - %d.\n", htq.key, htq.value);
        break;
    case READ_QUERY:
        hash_get(ht, htq.key, &htq.value);
        printf("Value against key %d is : %d\n", htq.key, htq.value);
        break;
    case DELETE_QUERY:
        hash_delete(ht, htq.key);
        printf("Deleted record with key : %d\n", htq.key);
        break;
    default:
        return -1;
    }

    return 0;
}

void* consumer_task_runner(void* args)
{
    int hit_max_buffers = 0;
    bool ami_last_consumer = false;
    int num_times_consumer_sem_acquired = 0;
	
    while (1) 
    {   
        //printf("[SERVER-%d] Waiting for consumer_count_sem...\n", (int)gettid());

        if (sem_wait (consumer_count_sem) == SEMAPHORE_FAILURE)
             print_err ("sem_wait: consumer_count_sem");
       
        // DBG only
        // num_times_consumer_sem_acquired++;
        // printf("[SERVER-%d] Num times consumer sem acquired - %d.\n", (int)gettid(), num_times_consumer_sem_acquired);
        // printf("[SERVER-%d] Waiting for mutex.\n", (int)gettid());

        // locking shm
        lock_shm_segment();

        // Critical section start
        if (shared_mem_ptr->consumer_index >= MAX_BUFFERS) {
            //printf("[SERVER-%d] Max buffers hit at check [1], thread exiting.\n", (int)gettid());
            hit_max_buffers = 1;
            release_shm_segment();
            pthread_exit(NULL);
        }
        
        int cons_index = shared_mem_ptr->consumer_index;

        printf("[SERVER-%d] query at index %d.\n", (int)gettid(), cons_index); 
        
        // query execution as HT is a concurrent DS
        execute_ht_query(shared_mem_ptr->hts[cons_index]);

        // increment consumer count
        (shared_mem_ptr->consumer_index)++;
        if (shared_mem_ptr->consumer_index == MAX_BUFFERS) {
            hit_max_buffers = 1;
            ami_last_consumer = true;
            //printf("[SERVER-%d] Max buffers hit at check [2], thread exiting.\n", (int)gettid());
            //shared_mem_ptr->consumer_index = 0;
        }
        // Critical section end

        // release shm
        release_shm_segment();

        if ( hit_max_buffers > 0 ) {
            if (ami_last_consumer) {
            // post for other threads
            printf("posting sems now...\n");
            for (int i=0; i<SERVER_THREAD_COUNT-1; i++)
                sem_post(consumer_count_sem);
            }
            pthread_exit(NULL);
        }

        // sleep for given ms - SERVER_THREAD_SLEEP_MS
        struct timespec ts;
        ts.tv_sec = SERVER_THREAD_SLEEP_MS / 1000;
        ts.tv_nsec = (SERVER_THREAD_SLEEP_MS % 1000) * 1000000;
        nanosleep(&ts, &ts);

    }
}

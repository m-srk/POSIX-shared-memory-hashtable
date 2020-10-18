#include <stdio.h>
#include <string.h>

#include "include/shm-semaphore-config.h"

using namespace std;

// globals
thread_task_cmd_t tt_cmd = (thread_task_cmd_t){ false };

HashTable *ht;
server_shm_data_t s_shm_data;
server_sem_data_t s_sem_data;
// shm related
int fd_shm;
struct shared_memory* shared_mem_ptr;
bool _server_shm_init = false;
// semaphore related
sem_t *mutex_sem, *producer_count_sem, *consumer_count_sem, *server_thread_mutex;
bool _server_semphore_init = false;


bool is_server_shm_ready()
{
    return _server_shm_init;
}

HashTable* get_HT_instance()
{
    return ht;
}

void release_shm_segment()
{
    // indicate shared memory segment is available
    if (sem_post (mutex_sem) == IPC_FAILURE)
        print_err("sem_post: mutex_sem");
}

void release_server_thread_lock()
{
    if (sem_post (server_thread_mutex) == IPC_FAILURE)
        print_err("sem_post: server_thread_mutex");
}

bool init_sems()
{
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_PRODUCER_COUNT);
    sem_unlink(SEM_CONSUMER_COUNT);
    sem_unlink(SERVER_THREAD_MUTEX);
    
    //  mutual exclusion semaphore, mutex_sem with an initial value 0.
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

    s_shm_data.fd_shm = fd_shm;
    s_shm_data.shared_mem_ptr = shared_mem_ptr;

    _server_shm_init = true;

    return true;
}

void register_HT_instance(HashTable *ht_instance)
{   
    ht = ht_instance;
}

int execute_ht_query(hashtable_query_t htq)
{
    printf("in execute ht\n");
    HashTable* ht = get_HT_instance();

    switch (htq.ht_query)
    {
    case INSERT_QUERY:
        hash_insert(ht, htq.key, htq.value);
        printf("Inserted key-value : %d - %s .\n", htq.key, htq.value);
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

void* hashtable_task_runner(void* args)
{
    bool is_max_buff_count_hit = false;
    int num_times_consumer_sem_acquired = 0;
    printf("iniside ht task thread\n");
    
    while (1) 
    {   
        printf("[SERVER-%d] Waiting for server thread lock...\n", (int)gettid());

        if (sem_wait(server_thread_mutex) == IPC_FAILURE)
             print_err ("sem_wait: server_thread_mutex");

        if (tt_cmd.is_max_buff_count_hit) {
            release_server_thread_lock();
            printf("[SERVER-%d] Max buffers hit at check [3], thread exiting.\n", (int)gettid());
            pthread_exit(NULL);
        }        

        printf("[SERVER-%d] Waiting for consumer_count_sem...\n", (int)gettid());

        if (sem_wait (consumer_count_sem) == IPC_FAILURE)
             print_err ("sem_wait: consumer_count_sem");
       
        num_times_consumer_sem_acquired++;
        printf("[SERVER-%d] Num times consumer sem acquired - %d.\n", (int)gettid(), num_times_consumer_sem_acquired);
        printf("[SERVER-%d] Waiting for mutex.\n", (int)gettid());

        // locking shm assuming all ps can access it all the time
        if (sem_wait(mutex_sem) == IPC_FAILURE)
             print_err("sem_wait:mutex");

        // Critical section start
        if (shared_mem_ptr->consumer_index >= MAX_BUFFERS) {
            is_max_buff_count_hit = true;
            printf("[SERVER-%d] Max buffers hit at check [1], thread exiting.\n", (int)gettid());
           //shared_mem_ptr->consumer_index = 0;
        }

        /* repair needed due to change in value type
        char query[256];
        strcpy(query, shared_mem_ptr->hts[shared_mem_ptr->consumer_index].ht_query);
        printf("[SERVER-%d] Query at index %d is : %s\n", (int)gettid(),
                                                            shared_mem_ptr->consumer_index,
                                                            query);
        */

        // TODO push current query to execution queue 
        
        // execute ht query
        execute_ht_query(shared_mem_ptr->hts[shared_mem_ptr->consumer_index]);
        
        // increment consumer count
        (shared_mem_ptr->consumer_index)++;
        if (shared_mem_ptr->consumer_index == MAX_BUFFERS) {
           is_max_buff_count_hit = true;
           tt_cmd.is_max_buff_count_hit = true;
           printf("[SERVER-%d] Max buffers hit at check [2], thread exiting.\n", (int)gettid());
            //shared_mem_ptr->consumer_index = 0;
        }
        // Critical section end

        // release shm
        release_shm_segment();

        // give out one more buffer - not required in the case of fixed no of reqs from client (=MAX_BUFFERS)
        // if (sem_post (ht_input-> producer_count_sem) == IPC_FAILURE)
        //       print_err ("sem_post: buffer_count_sem");
        // printf("[SERVER-%d] Released buff count sem.\n", (int)gettid());

        release_server_thread_lock();

        if (is_max_buff_count_hit) {
            printf("[SERVER-%d] server thread exiting...\n", (int)gettid());
            pthread_exit(NULL);
        }

        // sleep for given ms - SERVER_THREAD_SLEEP_MS
        struct timespec ts;
        ts.tv_sec = SERVER_THREAD_SLEEP_MS / 1000;
        ts.tv_nsec = (SERVER_THREAD_SLEEP_MS % 1000) * 1000000;
        nanosleep(&ts, &ts);
    }
}

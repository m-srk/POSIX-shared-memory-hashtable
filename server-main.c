#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#include "include/shm-semaphore-config.h"

#define IPC_FAILURE 1
#define STRINGS_EQUAL 0


// globals
HashTable* ht_instance;
bool is_HT_initialized = false;

void print_usage_and_exit(char* args)
{
    printf("USAGE: %s <BUCKET_SIZE> <NUM_MUTEX_LOCKS>\n", args);
    printf("     BUCKET_SIZE        : Max. number of entries supported by the Hash table.\n");
    printf("     NUM_MUTEX_LOCKS    : Num of mutex locked partitions of the table.\n");

    exit(EXIT_FAILURE);
}

bool init_hashtable(int size, int m_locks)
{
    if ( (ht_instance = hash_init(size, m_locks)) != NULL)
        return (is_HT_initialized = true);

    return false;
}

void  print_err(const char* args)
{
    perror(args);
    exit(EXIT_FAILURE);
}

int process_cmdline_args(int argc, char* argv[])
{
    if (argc != 3)
        return -1;
    
    // initialize HashTable here
    if (atoi(argv[1]) > 0 && atoi(argv[2]) > 0){
        if (!init_hashtable(atoi(argv[1]), atoi(argv[2]))) {
            print_err("Unable to init HT.");
        }
        printf("HT init done.\n");
        return 1;
    } else {
        return -1;
    }
    
}

int main (int argc, char* argv[])
{   
    if (process_cmdline_args(argc, argv) < 1)
        print_usage_and_exit(argv[0]);

    if (!is_HT_initialized)
         print_err("Failed to init HTable");

    register_HT_instance(ht_instance);
    open_and_map_shm();
    init_sems();
    printf("[SERVER] Semaphores inited... \n");

    // release shm so that client can start adding
    release_shm_segment();

    // create shm consumers and HT executor threads
    pthread_t tid[SERVER_THREAD_COUNT];
    int i=0;
    for (; i<SERVER_THREAD_COUNT; i++)
        pthread_create(&tid[i], NULL, consumer_task_runner, NULL);

    // wait for threads to do their work
    for (int j=0; j<SERVER_THREAD_COUNT; j++)
        pthread_join(tid[j], NULL);

    // free HT mem.
    hash_destroy(ht_instance);
    printf("Threads joined, exiting.\n");

    return 0;
}

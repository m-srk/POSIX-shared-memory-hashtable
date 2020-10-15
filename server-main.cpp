#include <iostream>
#include <string>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>

#include "HashTable.hpp"
#include "utils/shm-semaphore-config.hpp"

#define IPC_FAILURE 1
#define STRINGS_EQUAL 0

using namespace std;

// globals
HashTable* ht;
bool is_HT_initialized = false;

void print_usage_and_exit(char* args)
{
    printf("USAGE: %s [--size=<BUCKET_SIZE>]\n", args);
    printf("     BUCKET_SIZE  : Max. number of entries supporte by the Hash table\n");

    exit(EXIT_FAILURE);
}

bool init_hashtable(int size)
{
    if ( (ht = new HashTable(size)) != NULL)
        return (is_HT_initialized = true);

    return false;
}

// todo make this a gen purpose util
void print_errors(const char* args)
{
    perror(args);
    exit(EXIT_FAILURE);
}

int process_cmdline_args(int argc, char* argv[])
{
    if (argc != 2)
        return -1;

    string size_str = argv[1];
    try
    {
        int pos = size_str.find("=");
        if (pos <= -1)
            return -1;
        
        size_str = size_str.substr(pos + 1);
        // initialize HashTable here
        if (!init_hashtable(stoi(size_str))) {
            print_errors("Unable to init HT.");
        }
        cout << "HT init done." << endl;
        return 1;
    } 
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }
}

void execute_ht_query(hashtable_query_t htq)
{
    cout << "in execute ht" << endl;

    if ( strcmp(htq.ht_query, "INSERT") == STRINGS_EQUAL ) {
        ht->insert_item(htq.key, htq.value);
        printf("Inserted key-value : %d - %s .\n", htq.key, htq.value);
        ht->print_table();
    } else if ( strcmp(htq.ht_query, "READ") == STRINGS_EQUAL ) {
        printf("Value against key %d is : %s\n", htq.key, ht->read_item(htq.key));
        ht->print_table();
    } else if ( strcmp(htq.ht_query, "DELETE") == STRINGS_EQUAL ) {
        ht->delete_item(htq.key);
        ht->print_table();
    } else {
        return;
    }

}

void* hashtable_task_runner(void* args)
{
    ht_worker_input_t* ht_input = (ht_worker_input_t*)args;
    cout << "iniside ht task thread" << endl;

    while (1) 
    {   
        auto shared_mem_ptr = ht_input->shm_data->shared_mem_ptr;
        auto sem_data = ht_input->sem_data;

        if (sem_wait (sem_data->consumer_count_sem) == IPC_FAILURE)
            print_errors ("sem_wait: spool_signal_sem");

        // locking shm assuming all ps can access it all the time - confirm ?
        if (sem_wait(sem_data->mutex_sem) == IPC_FAILURE)
            print_errors("sem_wait:mutex");

        // Critical section start
        char query[256];
        strcpy(query, shared_mem_ptr->hts[shared_mem_ptr->consumer_index].ht_query);
        printf("Query from thread %d at index %d is : %s\n", (int)gettid(),
                                                            shared_mem_ptr->consumer_index,
                                                            query);
        // execute ht query
        execute_ht_query(shared_mem_ptr->hts[shared_mem_ptr->consumer_index]);
        // increment consumer count
        (shared_mem_ptr->consumer_index)++;
        if (shared_mem_ptr->consumer_index == MAX_BUFFERS)
           shared_mem_ptr->consumer_index = 0;
        // Critical section end

        // release shm
        release_shm_segment();

        // give out one more buffer
        if (sem_post (ht_input->sem_data->producer_count_sem) == IPC_FAILURE)
            print_errors ("sem_post: buffer_count_sem");

        cout << "Released buff count sem." << endl;
        
    }
}

int main (int argc, char* argv[])
{   
    if (process_cmdline_args(argc, argv) < 1)
        print_usage_and_exit(argv[0]);

    if (!is_HT_initialized)
        print_errors("HT nope init");

    open_and_map_shm();
    init_sems();

    cout << "Semaphores inited" << endl;

    release_shm_segment();

    ht_worker_input_t ht_input = *(ht_worker_input_t* )malloc(sizeof(ht_worker_input_t)); 
    ht_input.sem_data = get_server_semaphore_data();
    ht_input.shm_data = get_server_shm_data();

    int NUM_THREADS = 3; 
    pthread_t tid[NUM_THREADS];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    for (int i=0; i<NUM_THREADS; i++)
        pthread_create(&tid[i], &attr, hashtable_task_runner, &ht_input);

    // wait for thread to do its work
    for (int j=0; j<NUM_THREADS; j++)
        pthread_join(tid[j], NULL);

    return 0;
    
}

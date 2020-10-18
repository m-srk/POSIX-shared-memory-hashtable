#include <iostream>
#include <string>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>

#include "include/HashTable.hpp"
#include "include/shm-semaphore-config.hpp"

#define IPC_FAILURE 1
#define STRINGS_EQUAL 0

using namespace std;

// globals
HashTable* ht_instance;
bool is_HT_initialized = false;

void print_usage_and_exit(char* args)
{
    printf("USAGE: %s [--size=<BUCKET_SIZE>]\n", args);
    printf("     BUCKET_SIZE  : Max. number of entries supporte by the Hash table\n");

    exit(EXIT_FAILURE);
}

bool init_hashtable(int size)
{
    if ( (ht_instance = new HashTable(size)) != NULL)
        return (is_HT_initialized = true);

    return false;
}

// todo make this a gen purpose util
void  print_err(const char* args)
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
             print_err("Unable to init HT.");
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

int main (int argc, char* argv[])
{   
    if (process_cmdline_args(argc, argv) < 1)
        print_usage_and_exit(argv[0]);

    if (!is_HT_initialized)
         print_err("Failed to init HTable");

    register_HT_instance(ht_instance);
    open_and_map_shm();
    init_sems();

    cout << "[SERVER] Semaphores inited" << endl;

    release_shm_segment();

    release_server_thread_lock();

    pthread_t tid[SERVER_THREAD_COUNT];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    for (int i=0; i<SERVER_THREAD_COUNT; i++)
        pthread_create(&tid[i], &attr, hashtable_task_runner, NULL);

    // wait for thread to do its work
    for (int j=0; j<SERVER_THREAD_COUNT; j++)
        pthread_join(tid[j], NULL);

    return 0; 
}

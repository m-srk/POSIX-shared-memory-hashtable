#include <iostream>
#include <string.h>
#include "shm-semaphore-config.hpp"

using namespace std;


// globals
bool is_rand_seeded = false;
struct shared_memory* shared_mem_ptr;
sem_t *mutex_sem, *producer_count_sem, *consumer_count_sem;

void print_err(const char* args)
{
    perror(args);
    exit(EXIT_FAILURE);
}

bool init_sems_client()
{
    //  mutual exclusion semaphore, mutex_sem
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, 0, O_RDWR | O_CREAT)) == SEM_FAILED)
        print_err ("sem_open");

    if ((producer_count_sem = sem_open (SEM_PRODUCER_COUNT, 0, 0, 0)) == SEM_FAILED)
        print_err ("sem_open");

    if ((consumer_count_sem = sem_open (SEM_CONSUMER_COUNT, 0, 0, 0)) == SEM_FAILED)
        print_err ("sem_open");
    
    cout << "[CLIENT] All sems init done." << endl;
    
    return true;
}   

bool open_and_map_shm_client()
{
    int fd_shm;

    // Get shared memory 
    if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR, 0)) == -1)
        print_err ("shm_open");

    if ((shared_mem_ptr = (struct shared_memory *) mmap (NULL, 
                                    sizeof (struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED,
                                    fd_shm, 0)) == MAP_FAILED)
        print_err ("mmap");

    return true;
}

void set_rand_seed()
{
	
    if (!is_rand_seeded) {
        srand(time(0)); // seed for rand func
    	is_rand_seeded = true;
    }

}

void generate_rand_query(hashtable_query_t* htq)
{	
	int NUM_QUERY_TYPES = 3;
	char query_types[NUM_QUERY_TYPES][256] = {"INSERT", "READ", "DELETE"};
	int qtype_index = rand() % NUM_QUERY_TYPES;
	int key = rand() % 1000;
	char value[256];
	
	// write query into the given mem
	htq->key = key;
	strcpy(htq->ht_query, query_types[qtype_index]);
       	sprintf(value, "valuestr_%d", key);
	strcpy(htq->value, value);

	return;
}

void* run_client_task_rand(void* args)
{
    while (1) {

        cout << "Adding new query..." << endl;

        if (sem_wait (producer_count_sem) == -1)
            print_err ("sem_wait: buffer_count_sem");
        cout << "WAITING FOR mutex sem." << endl;

        // get shm mutex 
        if (sem_wait (mutex_sem) == -1)
            print_err ("sem_wait: mutex_sem");
        cout << "Got the mutex sem." << endl;
	
 	// if the buffers are full, nothing to do 	
	if (shared_mem_ptr->producer_index >= MAX_BUFFERS) {
		printf("Max buffers hit, client thread %d exiting\n", (int)gettid());
		pthread_exit(NULL);	
	}	

	// Critical section start
        hashtable_query_t* htq = (hashtable_query_t* ) malloc(sizeof(hashtable_query_t));
        generate_rand_query(htq);
              
        memcpy(
            & shared_mem_ptr->hts[shared_mem_ptr->producer_index], 
            htq, 
            sizeof(hashtable_query_t)
        );

        (shared_mem_ptr->producer_index)++;
        if (shared_mem_ptr->producer_index == MAX_BUFFERS) {
		printf("Max buffers hit, client thread %d exiting\n", (int)gettid());
		pthread_exit(NULL);	
		//shared_mem_ptr->producer_index = 0;
	}		
                
        // Critical section end
	
	// release mutex
        if (sem_post (mutex_sem) == IPC_FAILURE)
            print_err ("sem_post: mutex_sem");

        // free mem
        free(htq);

        // give out consumer sem
        if (sem_post(consumer_count_sem) == IPC_FAILURE)
            print_err("sem_post: consumer_count");
            
	// sleep for given ms - CLIENT_THREAD_SLEEP_MS
        struct timespec ts;
	ts.tv_sec = CLIENT_THREAD_SLEEP_MS / 1000;
	ts.tv_nsec = (CLIENT_THREAD_SLEEP_MS % 1000) * 1000000;
	nanosleep(&ts, &ts);
    }
   
    if (munmap (shared_mem_ptr, sizeof (struct shared_memory)) == IPC_FAILURE)
        print_err ("munmap");

}


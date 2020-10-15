#include <iostream>
#include <string>
#include <pthread.h>

#include "HashTable.hpp"
#include "utils/shm-semaphore-config.hpp"

using namespace std;


int main (int argc, char* argv[])
{   
    set_rand_seed();
    init_sems_client();
    open_and_map_shm_client();
    cout << "Semaphores inited" << endl;
    
    // lets spawn few client threads 
    pthread_t tid[CLIENT_THREAD_COUNT];
    for (int i=0; i<CLIENT_THREAD_COUNT; i++) {
    	pthread_create(&tid[i], NULL, run_client_task_rand, NULL); 
    }

    // wait for all threads to complete
    for (int j=0; j<CLIENT_THREAD_COUNT; j++) {
    	pthread_join(tid[j], NULL);
    }

    exit(EXIT_SUCCESS);

}

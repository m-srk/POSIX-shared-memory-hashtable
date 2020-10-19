#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "include/shm-semaphore-config.h"


int main (int argc, char* argv[])
{   
    set_rand_seed();

    init_sems_client();
    
    open_and_map_shm_client();

    printf("[CLIENT] Semaphores inited, shm mapped.\n");
    
    // spawn few client threads 
    pthread_t tid[CLIENT_THREAD_COUNT];
    
    for (int i=0; i<CLIENT_THREAD_COUNT; i++) {
    	pthread_create(&tid[i], NULL, run_client_task_rand, NULL); 
    }

    // wait for all threads to complete
    for (int j=0; j<CLIENT_THREAD_COUNT; j++) {
    	pthread_join(tid[j], NULL);
    }

    unmap_shm_mem();

    return 0;

}

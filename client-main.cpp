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
    
    pthread_t tid;
    pthread_create(&tid, NULL, run_client_task_rand, NULL); 
    pthread_join(tid, NULL);
    //run_client_task_rand();

}

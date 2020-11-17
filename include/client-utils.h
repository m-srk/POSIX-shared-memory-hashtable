#ifndef __CLIENT_UTILS_H__
#define __CLIENT_UTILS_H__

#include <stdbool.h>

#define CLIENT_THREAD_COUNT 5
#define CLIENT_THREAD_SLEEP_MS 5

//-------------------------------------------
// Client API
void set_rand_seed();

bool init_sems_client();
    
bool open_and_map_shm_client();

void* run_client_task_rand(void* args);

void unmap_shm_mem();

#endif __CLIENT_UTILS_H__

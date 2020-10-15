#include <iostream>

#include "shm-semaphore-config.hpp"

// globals
server_shm_data_t s_shm_data;
server_sem_data_t s_sem_data;

sem_t* mutex_sem;
sem_t* producer_count_sem;
sem_t* consumer_count_sem; 

bool _server_shm_init = false;
bool _server_semphore_init = false;


void print_err(const char* args)
{
    perror(args);
    exit(EXIT_FAILURE);
}

bool is_server_shm_ready()
{
    return _server_shm_init;
}

server_shm_data_t* get_server_shm_data()
{
    return &s_shm_data;
} 

server_sem_data_t* get_server_semaphore_data()
{
    return &s_sem_data;
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
    sem_unlink(SEM_PRODUCER_COUNT);
    sem_unlink(SEM_CONSUMER_COUNT);
    
    //  mutual exclusion semaphore, mutex_sem with an initial value 0.
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        print_err("sem_open: mutex_sem");
    s_sem_data.mutex_sem = mutex_sem;

    // counting semaphore, indicating the number of available buffers. Initial value = MAX_BUFFERS
    if ((producer_count_sem = sem_open (SEM_PRODUCER_COUNT, O_CREAT, 0660, MAX_BUFFERS)) == SEM_FAILED)
        print_err ("sem_open: buffer_count");
    s_sem_data.producer_count_sem = producer_count_sem;

    // counting semaphore, indicating the number of queries to be processed. Initial value = 0
    if ((consumer_count_sem = sem_open (SEM_CONSUMER_COUNT, O_CREAT, 0660, 0)) == SEM_FAILED)
        print_err ("sem_open");
    s_sem_data.consumer_count_sem = consumer_count_sem;
    
    return true;
}   

bool open_and_map_shm()
{
    int fd_shm;
    struct shared_memory* shared_mem_ptr;

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

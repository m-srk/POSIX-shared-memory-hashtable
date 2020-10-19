/*
* Thread safe hash table impl. using modulus on key (int type) as the hash function.
* Bucket size is fixed and mutexs are attached to partitions of buckets.
* HashTable accepts K, V with types int, int
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "include/hashtable.h"

#define unlock pthread_mutex_unlock
#define lock pthread_mutex_lock


pthread_mutex_t mutexes[100];
pthread_mutex_t destroy_mutex = PTHREAD_MUTEX_INITIALIZER;

HashTable* hash_init(int table_size, int M){
    int i;
    if( (table_size < MIN_N || table_size > MAX_N) || (M < MIN_M || M > MAX_M)){
        printf("table_size and M must be between 100 - 1000.\n");
        exit(1);
    }
        // allocate space as big as table
        HashTable *new_table;
        new_table = (HashTable *) malloc(sizeof( HashTable ));

        // initialize members
        new_table->size = table_size;
        new_table->m_val = M;
        new_table->table = (Node **) malloc(sizeof(Node*) * table_size);

        // If divides with no remainder, K = table_size/M
        if(table_size % M == 0){
            new_table->region = ceil(table_size/M);
        }
        // Else, +1
        else{
            new_table->region = ceil(table_size/M) + 1;
        }

        // All bucket heads to be NULL at first
        for( i = 0; i < table_size; i++ ) {
            new_table->table[i] = NULL;
        }

        // init all mutex locks
        for(i = 0; i < (new_table->region); i++){
            pthread_mutex_init(&mutexes[i], NULL);
        }
        return (new_table);
        
}

// Simple hash function, hash = key % table_size
int hash_func(int key, int table_size){
    if(key <= 0){
        printf("invalid key value\n");
        return -1;
    }
    else{
        int hash = key % table_size;
        return hash;
    }
}

// Function inserting to hp hash table, key k and v value of k.
int hash_insert (HashTable *hp, int k, int v){
    if(k <= 0){
        printf("invalid key value\n");
        return -1;
    }

    int bucket_index = hash_func(k, hp->size);
    
    // get correct region index
    int kth_lock = bucket_index / hp->m_val;

    lock(&mutexes[kth_lock]);
    Node* place = hp->table[bucket_index];
    Node* new_node;

    // bucket is initially empty
    if(place == NULL){
        hp->table[bucket_index] = (Node*) malloc (sizeof(Node));
        hp->table[bucket_index]->key = k;
        hp->table[bucket_index]->next = NULL;
        hp->table[bucket_index]->value = v;
        unlock(&mutexes[kth_lock]);
        return 0;
    }
    // search if key already exists
    else{
        while(place->next != NULL){
            if(place->key == k){
                unlock(&mutexes[kth_lock]);
                return -1;
            }
            place = place->next;
        }

        // key already exists
        if(place->key == k){
            unlock(&mutexes[kth_lock]);
            return -1;
        }

        // no key in the bucket, insert now
        new_node = (struct Node*)malloc(sizeof(struct Node));
        place->next = new_node;
        new_node->next = NULL;
        new_node->key = k;
        new_node->value = v;

        //unlock and return 0
        unlock(&mutexes[kth_lock]);
        return (0);
    }

}

// Retrieves key k's value into vptr from hash table hp
int hash_get (HashTable *hp, int k, int *vptr){
    if(k <= 0){
        printf("invalid key value\n");
        return -1;
    }

    // finding the correct bucket
    int bucket_index = hash_func(k, hp->size);
    int kth_lock = bucket_index / hp->m_val;

    // locking respective region
    lock(&mutexes[kth_lock]);
    Node* place = hp->table[bucket_index];

    // iterate to find the key
    while(place != NULL){
        // when key's found
        if(place->key == k){
            *vptr = place->value;
            unlock(&mutexes[kth_lock]);
            return 0;
        }
        place = place->next;
    }
    unlock(&mutexes[kth_lock]);

    return -1;
}

// Function deleting from hp hash table, key k and it's properties
int hash_delete (HashTable *hp, int k) {
    if(k <= 0){
        printf("invalid key value\n");
        return -1;
    }
    Node* node_to_delete;

    // The right bucket
    int bucket_index = hash_func(k, hp->size);
    // Correct lock for region K
    int kth_lock = bucket_index / hp->m_val;

    lock(&mutexes[kth_lock]);
    Node* place = hp->table[bucket_index];

    if(place != NULL){
        if(place->key == k){
            // change bucket head ptr
            hp->table[bucket_index] = place->next;
            free(place);
            unlock(&mutexes[kth_lock]);
            return 0;
        }
    }
    else{
        unlock(&mutexes[kth_lock]);
        return -1;
    }

    while(place->next != NULL){
        // having found our key in place's next
        if(place->next->key == k){
            //toDelete is set to place->next
            node_to_delete = place->next;
            //before deleting, it's set to the upcoming one
            place->next = place->next->next;
            //delete
            free(node_to_delete);
            unlock(&mutexes[kth_lock]);
            return 0;
        }
        place = place->next;
    }

    // If the key cannot be deleted
    printf("Key: %d -- cannot be deleted.\n", k);
    unlock(&mutexes[kth_lock]);

    return -1;
}

// iterate and destroy entire hash table
int hash_destroy (HashTable *hp){
    int i;
    lock(&destroy_mutex);

    Node* dest_node;

    //iterating whole table and freeing all
    for(i = 0; i < hp->size; ++i){
        dest_node = hp->table[i];
        while (dest_node != NULL){
            Node* del_ptr = dest_node;
            dest_node = dest_node->next;
            free(del_ptr);
        }
    }
    free(hp->table);
    free(hp);
    unlock(&destroy_mutex);
    return (0);
}

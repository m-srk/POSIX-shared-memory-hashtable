# POSIX Shared Memory Demo Project
Server and client processes communicate through POSIX shm API, client sends hashtable queries which the mult-threaded server works on. 

# How to Build and Run ->
(HT => HashTable)
1. Clone the repo, checkout main branch
2. run `make` to build the server and client binaries
3. In one terminal window, run the server using `./runserver <TABLE_SIZE> <NUM_HT_PARTITIONS_LOCK>`, by replacing the two args option with your desired input, tests were run using size = 1000, partitions/locks = 50
4. In another window, start the client process using `./runclient`
5. For cleanup run `make clean` 

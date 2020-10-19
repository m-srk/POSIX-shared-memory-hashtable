#!/bin/bash

rm server.log client.log

# make includes clean
make 

# $1 - size, $2 - num mutexs/partitions
{ time ./runserver $1 $2 > server.log & } 2>> time-server.log

pidof runclient >/dev/null && killall runclient 
./runclient > client.log

exit
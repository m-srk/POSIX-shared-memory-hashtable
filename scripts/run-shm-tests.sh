#!/bin/bash

make clean
rm server.log client.log

make SERVER_THREAD_COUNT_CONF=$2

{ time ./runserver --size=$1 >> server.log & } 2>> time-server.log

pidof runclient >/dev/null && killall runclient 
./runclient >> client.log

exit
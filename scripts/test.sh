#!/bin/bash

HT_SIZE=12

rm time-server.log

for i in {5..6}
do
	echo "-------------------------------------"
	echo "running tests with $i server threads."
	echo "-------------------------------------"
	./run-shm-tests.sh ${HT_SIZE} $i  > /dev/null
done





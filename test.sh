# !/bin/bash

#make clean
#make

master_hmn=vcm-9001.vm.duke.edu
master_port=4444
players=50
hops=500

./ringmaster $master_port $players $hops &
sleep 1

for ((i=0;i<$players;i++))
do
    ./player $master_hmn $master_port &
    
done
wait

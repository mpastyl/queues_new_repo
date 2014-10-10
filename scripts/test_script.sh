#!/bin/bash

cd /home/users/mpastyl/queues_new_repo/
<<comment
for i in 1 2 3 6 12 24 48;
do 
    for name in "global_lock" "msqueue_aba" "optimistic" "fc_queue" "fc_dedicated";
    do 
        if [ $i -lt 3 ] && [ $name = "fc_dedicated" ]; then
            echo "num_threads: $i" >>out_${name}_dunni_hyperthreading_50_50.out
            echo "Ops/usec: 0.000" >>out_${name}_dunni_hyperthreading_50_50.out
        else
            ./main.${name} -t $i -l 50 >> out_${name}_dunni_hyperthreading_50_50.out
        fi
    done;
done;
comment

for name in "global_lock" "msqueue_aba" "optimistic" "fc_queue" "fc_hybrid";
#for name in "fc_dedicated"
#for i in 128;
do 
    for i in 1 2 4 8 16 32 64;
    do 
        echo " ${name} thread $i"
        if [ $i -lt 3 ] && [ $name = "fc_dedicated" ]; then
            echo "num_threads: $i" >>out_${name}_sandman_hyperthreading_50_50.out
            echo "Ops/usec: 0.000" >>out_${name}_sandman_hyperthreading_50_50.out
        elif [ $i -lt 8 ] && [ $name = "fc_hybird" ]; then 
            
            echo "num_threads: $i" >>out_${name}_sandman_hyperthreading_50_50.out
            echo "Ops/usec: 0.000" >>out_${name}_sandman_hyperthreading_50_50.out
        else
            ./main.${name} -t $i -l 50 >> out_${name}_sandman_hyperthreading_50_50.out
        fi
    done;
done;


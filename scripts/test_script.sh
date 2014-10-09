#!/bin/bash

cd /home/users/mpastyl/queues_new_repo/

#for i in 1 2 3 6 12 24;
#do 
#    for name in "global_lock" "msqueue_aba" "optimistic" "fc_queue" "fc_dedicated";
#    do 
#        if [ $i -lt 3 ] && [ $name = "fc_dedicated" ]; then
#            echo "num_threads: $i" >>out_${name}_dunni_50_50.out
#            echo "Ops/usec: 0.000" >>out_${name}_dunni_50_50.out
#        else
#            ./main.${name} -t $i -l 50 >> out_${name}_dunni_50_50.out
#        fi
#    done;
#done;

for i in 1 2 4 8 16 32 64;
do 
    for name in "global_lock" "msqueue_aba" "optimistic" "fc_queue" "fc_dedicated";
    do 
        if [ $i -lt 3 ] && [ $name = "fc_dedicated" ]; then
            echo "num_threads: $i" >>out_${name}_sandman_50_50.out
            echo "Ops/usec: 0.000" >>out_${name}_sandman_50_50.out
        else
            ./main.${name} -t $i -l 50 >> out_${name}_sandman_50_50.out
        fi
    done;
done;


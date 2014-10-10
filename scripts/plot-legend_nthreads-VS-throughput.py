#!/usr/bin/env python

import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

nthreads_axis = []
all_results = []
ax = plt.subplot("111")

for filename in sys.argv[1:]:
    if "global_lock" in filename:
        name ="global_lock"
    elif "msqueue" in filename:
        name = "msqueue"
    elif "optimistic" in filename:
        name = "optimistic"
    elif "fc_queue" in filename:
        name = "fc_queue"
    elif "fc_dedicated" in filename:
        name = "fc_dedicated"
    elif "fc_hybrid" in filename:
        name = "fc_hybrid"
    
    fp = open(filename, "r")

    local_nthreads_axis = []
    local_results = []

    line = fp.readline()
    while line:
        if "num_threads:" in line:
            nthreads = int(line.split()[1])
            local_nthreads_axis.append(nthreads)
        elif "Ops/usec:" in line:
            throughput = float(line.split()[1])
            if (name == "fc_dedicated" and local_nthreads_axis[-1]<3):
                throughput = None
            if (name == "fc_hybrid" and local_nthreads_axis[-1]<8):
                throughput = None
            local_results.append(throughput)

        line = fp.readline()

    fp.close()

    if len(nthreads_axis) == 0:
        nthreads_axis = list(local_nthreads_axis)
    elif len(nthreads_axis) != len(local_nthreads_axis):
        print "File", filename, "contains less nthreads values."
        sys.exit(1)
    elif nthreads_axis != local_nthreads_axis:
        print "File", filename, "contains different nthreads values."
        sys.exit(1)
    
    ax.plot(local_results,label=name);
    all_results.append(local_results)

print nthreads_axis
print all_results


#for res in all_results:
#    ax.plot(np.arange(len(nthreads_axis)), res,label="5")

#plt.title()
legend=ax.legend(loc='upper center', shadow=True);
plt.grid(True)
plt.xticks(np.arange(len(nthreads_axis)), nthreads_axis)
ax.set_xlabel("#threads")
ax.set_ylabel("throuhput (ops/usec)")
plt.savefig("nthreads-VS-throughput.pdf")

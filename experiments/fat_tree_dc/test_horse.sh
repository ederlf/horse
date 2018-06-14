#!/bin/bash

k=$3
num_trials=$1
duration=$2
if [ "$#" != "3" ]; then
  echo "Usage: $0 <num_trials> <duration_per_run>"
  echo "Recommended: num_trials = 5, duration_per_run = 60"
  exit 1
fi
for (( trial=1; trial<=num_trials; trial++ ))
do
#./runhorse.sh $k stag,0.2,0.3 1 1111 $trial $duration
# ./runhorse.sh $k stag,0.5,0.3 1 1111 $trial $duration
# ./runhorse.sh $k stag,0.2,0.3 1 2222 $trial $duration
# ./runhorse.sh $k stag,0.5,0.3 1 2222 $trial $duration
# ./runhorse.sh $k stag,0.2,0.3 1 3333 $trial $duration
# ./runhorse.sh $k stag,0.5,0.3 1 3333 $trial $duration
# ./runhorse.sh $k stag,0.2,0.3 2 4444 $trial $duration
# ./runhorse.sh $k stag,0.5,0.3 2 4444 $trial $duration
# ./runhorse.sh $k stag,0.2,0.3 2 5555 $trial $duration
# ./runhorse.sh $k stag,0.5,0.3 2 5555 $trial $duration
# ./runhorse.sh $k stag,0.2,0.3 2 6666 $trial $duration
# ./runhorse.sh $k stag,0.5,0.3 2 6666 $trial $duration
./runhorse.sh $k stride,1 1 2345 $trial $duration
# ./runhorse.sh $k stride,2 1 2345 $trial $duration
# ./runhorse.sh $k stride,4 1 2345 $trial $duration
# ./runhorse.sh $k stride,8 1 2345 $trial $duration
#./runhorse.sh $k random 1 11115 $trial $duration
# ./runhorse.sh $k random 1 22225 $trial $duration
# ./runhorse.sh $k random 1 33335 $trial $duration
# ./runhorse.sh $k random 2 44444 $trial $duration
# ./runhorse.sh $k random 2 55555 $trial $duration
#./runhorse.sh $k randbij 1 1 $trial $duration
# ./runhorse.sh $k randbij 1 2 $trial $duration
# ./runhorse.sh $k randbij 2 3 $trial $duration
# # for random number of flows (create 6 flows per host and only use half)
# ./runhorse.sh $k random 6 4 $trial $duration
# ./runhorse.sh $k random 6 5 $trial $duration
# ./runhorse.sh $k random 6 6 $trial $duration
done

# python plot_results.py $k $duration results-horse -- results1.png 1 ecmp hedera random -- results2.png 1 st random ecmp  -- results3.png 2,6 ecmp hedera random -- results4.png 2,6 st random ecmp

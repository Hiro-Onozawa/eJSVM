#!/bin/bash

if [ $# -ge 1 ] && [ "$1" = "--profile" ]; then
N=1
suffix="_profile"
option="-u --alloc-info --collect-info --moving-info"
results_dir=./dats/profile
else
N=100
suffix=""
option="-u"
results_dir=./dats/results
fi

test_dir=../regression-test/bc/testcases
vms_dir=./dats/vms
algorithms=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
tests=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "fasta" "spectralnorm" "string-intensive" )
threasholds=( 1 2 3 )
sizes=( 10485760 7864320 5242880 3932160 2621440 1966080 1310720 )

# rm -f ${results_dir}/*.csv*
mkdir -p ${results_dir}
mkdir -p ${results_dir}/tmp

date
for algorithm in ${algorithms[@]}
do
  for threashold in ${threasholds[@]}
  do
    for size in ${sizes[@]}
    do
      echo ${algorithm}_${size}_t${threashold}
      for i in `seq 1 ${N}`
      do
        for test in ${tests[@]}
        do
          vm=${vms_dir}/ejsvm_64_${algorithm}_${size}_t${threashold}${suffix}
          out=${results_dir}/tmp/${algorithm}_${size}_t${threashold}_${test}${suffix}.csv
          ${vm} ${option} ${test_dir}/${test}.sbc &>> ${out}.tmp
          if [ $? -eq 139 ]; then
            echo "Segmentation fault" >> ${out}.tmp
          fi
        done
      done
    done
  done
done
date

if [ $# -ge 1 ] && [ "$1" = "--profile" ]; then
./profile_to_csv.sh
else
./result_to_csv.sh
fi

#!/bin/bash

N=100
test_dir=../regression-test/bc/testcases
vms_dir=./vms
results_dir=./results
algorithms=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
tests=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "fasta" "spectralnorm" "string-intensive" )
threasholds=( 1 2 3 )
sizes=( 10485760 7864320 5242880 3932160 2621440 2162688 1310720 )

rm -f ${results_dir}/*.csv*
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
          vm=${vms_dir}/ejsvm_64_${algorithm}_${size}_t${threashold}
          out=${results_dir}/${algorithm}_${size}_t${threashold}_${test}.csv
          ${vm} -u ${test_dir}/${test}.sbc &>> ${out}.tmp
        done
      done
    done
  done
done

for algorithm in ${algorithms[@]}
do
  for threashold in ${threasholds[@]}
  do
  for size in ${sizes[@]}
  do
      echo ${algorithm}_${size}_t${threashold}
    for test in ${tests[@]}
    do
        out=${results_dir}/${algorithm}_${size}_t${threashold}_${test}.csv
       grep "total GC time" ${out}.tmp | awk '{ print $5","$11","$15 }' > ${out}
       ./calc ${N} < ${out} >> ${out}
#        rm ${out}.tmp
      done
    done
  done
done

#!/bin/bash

test_dir=../regression-test/bc/testcases
results_dir=./dats/results
dat_dir=./dats/dat_in
graph_dir=./dats/dat_out
algorithms=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
tests=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "fasta" "spectralnorm" "string-intensive" )
threasholds=( 1 2 3 )
sizes=( 10485760 7864320 5242880 3932160 2621440 1966080 1310720 )

mkdir -p ${dat_dir}
mkdir -p ${graph_dir}

for algorithm in ${algorithms[@]}
do
  for threashold in ${threasholds[@]}
  do
    for test in ${tests[@]}
    do
      out=${dat_dir}/${algorithm}_t${threashold}_${test}.txt
      echo "# アルゴリズム : ${algorithm}" > ${out}
      echo "# スレッショルド : ${threashold}" >> ${out}
      echo "# ベンチマーク : ${test}" >> ${out}
      for size in ${sizes[@]}
      do
        in=${results_dir}/${algorithm}_${size}_t${threashold}_${test}.csv
        grep "GC time    \[" ${in} | awk -v s=${size} -e '{ print s" "$6 }' >> ${out}
      done
    done
  done
done

for threashold in ${threasholds[@]}
do
  for test in ${tests[@]}
  do
    gnuplot -e "indir='${dat_dir}'; outdir='${graph_dir}'; benchname='${test}'; threashold='t${threashold}'" scripts/plot.gp
  done
done

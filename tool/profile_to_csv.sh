#!/bin/bash

test_dir=../regression-test/bc/testcases
bin_dir=./bin
results_dir=./dats/profile/tmp
dat_dir=./dats/profile
algorithms=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
tests=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "fasta" "spectralnorm" "string-intensive" )
threasholds=( 1 2 3 )
sizes=( 10485760 7864320 5242880 3932160 2621440 2162688 1310720 )

for algorithm in ${algorithms[@]}
do
  for threashold in ${threasholds[@]}
  do
    for size in ${sizes[@]}
    do
      for test in ${tests[@]}
      do
        in=${results_dir}/${algorithm}_${size}_t${threashold}_${test}_profile.csv.tmp
        out=${dat_dir}/${algorithm}_${size}_t${threashold}_${test}.csv
        if [ `grep "Segmentation" ${in} | wc -l` -eq 0 ] && [ `grep "time out" ${in} | wc -l` -eq 0 ]; then
          echo "# アルゴリズム : ${algorithm}" > ${out}
          echo "# ベンチマーク : ${test}" >> ${out}
          echo "# スレッショルド : ${threashold}" >> ${out}
          echo "# 生データ : ${in}" >> ${out}
          ${bin_dir}/profile_to_csv 1 ${in} >> ${out}
        fi
      done
    done
  done
done

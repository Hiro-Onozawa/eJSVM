#!/bin/bash

test_dir=../regression-test/bc/testcases
bin_dir=./bin
results_dir=./dats/tmp
dat_dir=./dats/tmp
graph_dir=./dats/tmp
algorithms=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
tests=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "fasta" "spectralnorm" "string-intensive" )

args=
for test in ${tests[@]}
do
  in=${results_dir}/mark_sweep_10485760_t1_${test}_profile.csv.tmp
  args="${args} ${in} ${test}"
done

${bin_dir}/object_alloc_rate 11 ${args} > ${dat_dir}/mark_sweep_request_rate.txt
gnuplot -e "indir='${dat_dir}'; outdir='${graph_dir}'; algorithm='mark_sweep'; prop='request'; type='rate'" scripts/object_alloc_rate.gp

${bin_dir}/object_alloc_rate 1 ${args} > ${dat_dir}/mark_sweep_request_size.txt
gnuplot -e "indir='${dat_dir}'; outdir='${graph_dir}'; algorithm='mark_sweep'; prop='request'; type='size'" scripts/object_alloc_rate.gp

for algorithm in ${algorithms[@]}
do
  args=
  for test in ${tests[@]}
  do
    in=${results_dir}/${algorithm}_10485760_t1_${test}_profile.csv.tmp
    args="${args} ${in} ${test}"
  done

  ${bin_dir}/object_alloc_rate 13 ${args} > ${dat_dir}/${algorithm}_header_rate.txt
  gnuplot -e "indir='${dat_dir}'; outdir='${graph_dir}'; algorithm='${algorithm}'; prop='header'; type='rate'" scripts/object_alloc_rate.gp

  ${bin_dir}/object_alloc_rate 3 ${args} > ${dat_dir}/${algorithm}_header_size.txt
  gnuplot -e "indir='${dat_dir}'; outdir='${graph_dir}'; algorithm='${algorithm}'; prop='header'; type='size'" scripts/object_alloc_rate.gp
done

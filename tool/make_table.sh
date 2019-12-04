#!/bin/bash

test_dir=../regression-test/bc/testcases
vms_dir=./dats/vms
results_dir=./dats/results
out_dir=./dats/dat_out/make_table
algorithms=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
tests=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "fasta" "spectralnorm" "string-intensive" )
threasholds=( 1 2 3 )
sizes=( 10485760 7864320 5242880 3932160 2621440 1966080 1310720 )

# total_CPU_time, total_GC_time, non_CPU_time, avr_GC_time, GC_count
columns=( 2 3 4 5 )

str_segmentation_fault="x"
str_timeout="timeout"

mkdir -p ${out_dir}

for test in ${tests[@]}
do
  out=${out_dir}/${test}.txt

  echo "=== ベンチマーク : ${test} ===" > ${out}

  line="^ ヒープサイズ [byte] ^"
  for algorithm in ${algorithms[@]}
  do
    line="${line} ${algorithm} ^"

    i=0
    for column in ${columns[@]}
    do
      if [ ${i} -eq 0 ]; then
        line="${line}"
      else
        line="${line}^"
      fi
      i=$(( i + 1 ))
    done
  done
  echo "${line}" >> ${out}

  for size in ${sizes[@]}
  do
    i=0
    for threashold in ${threasholds[@]}
    do
      if [ ${i} -eq 0 ]; then
        line="^ ${size} |"
      else
        line="^ ::: |"
      fi

      for algorithm in ${algorithms[@]}
      do

        csv=${results_dir}/${algorithm}_${size}_t${threashold}_${test}.csv

        j=0
        for column in ${columns[@]}
        do
          if [ -e ${csv} ]; then
            tmp=`tail -n +6 ${csv} | awk -F, '{ if ($1 == "nan") { timeout = 1 } if ($'${column}' != "nan") { sum += $'${column}' } } END{ if (timeout == 1) { print "'${str_timeout}'" " (" sum/NR ")" } else { print sum/NR } }'`
          else
            tmp="${str_segmentation_fault}"
          fi

          if [ ${j} -eq 0 ]; then
            val="${tmp}"
          else
            val="${val} | ${tmp}"
          fi
          j=$(( j + 1 ))
        done

        line="${line} ${val} |"
      done
      echo "${line}" >> ${out}

      i=$(( i + 1 ))
    done
  done
done

for test in ${tests[@]}
do
  cat ${out_dir}/${test}.txt
  echo ""
done > ${out_dir}/all.txt

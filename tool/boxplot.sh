#!/bin/bash

. ./params/arg_parser.sh $@ || exit 1

if [[ $PROFILE = "TRUE" ]]; then
  echo "cannot use option \"PROFILE\""
  exit 1
fi

DIR_INFILE=${DIR_DATS}/tmp
DIR_GRAPH=${DIR_DATS}/graph/boxplot

mkdir -p ${DIR_INFILE}
mkdir -p ${DIR_GRAPH}

for ALGORITHM in ${ALGORITHMS[@]}
do
  for THREASHOLD in ${THREASHOLDS[@]}
  do
    for TEST in ${TESTS[@]}
    do
      out=${DIR_INFILE}/${ALGORITHM}_t${THREASHOLD}_${TEST}.txt
      echo "# アルゴリズム : ${ALGORITHM}" > ${out}
      echo "# スレッショルド : ${THREASHOLD}" >> ${out}
      echo "# ベンチマーク : ${TEST}" >> ${out}
      for SIZE in ${SIZES[@]}
      do
        in=${DIR_RESULT}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}.csv
        echo "# ソースファイル : ${in}" >> ${out}
        if [ -e ${in} ]; then
          #total_CPU_time, total_GC_time, non_CPU_time, max_GC_time, avr_GC_time, GC_count
          tail +6 ${in} | cut -d "," -f 5 | awk -v s=${SIZE} '{ print s" "$1 }' >> ${out}
          echo "" >> ${out}
          echo "" >> ${out}
        else
          echo "# ファイルが存在しません" >> ${out}
          echo "" >> ${out}
        fi
      done
    done
  done
done

label_max=${SIZES[0]}
label_min=${SIZES[$((${#SIZES[@]}-1))]}
for THREASHOLD in ${THREASHOLDS[@]}
do
  for TEST in ${TESTS[@]}
  do
    gnuplot -e "indir='${DIR_INFILE}'; outdir='${DIR_GRAPH}'; benchname='${TEST}'; threashold='t${THREASHOLD}'; label_max='${label_max}'; label_min='${label_min}'" ./scripts/boxplot.gp
  done
done

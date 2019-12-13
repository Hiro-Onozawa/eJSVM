#!/bin/bash

. ./params/arg_parser.sh || exit 1

mkdir -p ${DIR_RESULT}

for ALGORITHM in ${ALGORITHMS[@]}
do
  for THREASHOLD in ${THREASHOLDS[@]}
  do
    for SIZE in ${SIZES[@]}
    do
      for TEST in ${TESTS[@]}
      do
        in=${DIR_RESULT_RAW}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}.txt
        out=${DIR_RESULT}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}.csv
        if [ `grep "Segmentation" ${in} | wc -l` -eq 0 ]; then
          echo "# アルゴリズム : ${ALGORITHM}" > ${out}
          echo "# ベンチマーク : ${TEST}" >> ${out}
          echo "# スレッショルド : ${THREASHOLD}" >> ${out}
          echo "# 生データ : ${in}" >> ${out}
          ${DIR_BIN}/result_to_csv 1 ${in} >> ${out}
        fi
      done
    done
  done
done

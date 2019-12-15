#!/bin/bash

. ./params/arg_parser.sh || exit 1

if [[ $PROFILE != "TRUE" ]]; then
  echo "cannot use option \"PROFILE\""
  exit 1
fi

mkdir -p ${DIR_PROFILE}

for ALGORITHM in ${ALGORITHMS[@]}
do
  for THREASHOLD in ${THREASHOLDS[@]}
  do
    for SIZE in ${SIZES[@]}
    do
      for TEST in ${TESTS[@]}
      do
        in=${DIR_PROFILE_RAW}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}_profile.txt
        out=${DIR_PROFILE}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}.csv
        if [ `grep "Segmentation" ${in} | wc -l` -eq 0 ] && [ `grep "time out" ${in} | wc -l` -eq 0 ]; then
          echo "# アルゴリズム : ${ALGORITHM}" > ${out}
          echo "# ベンチマーク : ${TEST}" >> ${out}
          echo "# スレッショルド : ${THREASHOLD}" >> ${out}
          echo "# 生データ : ${in}" >> ${out}
          ${DIR_BIN}/profile_to_csv 1 ${in} >> ${out}
        fi
      done
    done
  done
done

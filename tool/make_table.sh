#!/bin/bash

. ./params/arg_parser.sh "$@" || exit 1

if [[ $PROFILE = "TRUE" ]]; then
  echo "cannot use option \"PROFILE\""
  exit 1
fi
COLUMNS=(${PARAM// / })
if [[ ${#COLUMNS[@]} -lt 1 ]]; then
echo "Usage: $(basename $0) [OPTIONS]  <--param Params>"
echo ""
echo "Params :"
echo "    1 : total_CPU_time"
echo "    2 : total_GC_time"
echo "    3 : non_GC_time"
echo "    4 : max_GC_time"
echo "    5 : avr_GC_time"
echo "    6 : GC_count"
exit 1
fi

DIR_OUT=${DIR_DATS}/table

STR_SEGMENTATION_FAULT="x"
STR_TIMEOUT="timeout"

mkdir -p ${DIR_OUT}

for TEST in ${TESTS[@]}
do
  out=${DIR_OUT}/${TEST}.txt

  echo "=== [${BASEBIT}bit] ベンチマーク : ${TEST} ===" > ${out}

  line="^ ヒープサイズ [byte] ^"
  for ALGORITHM in ${ALGORITHMS[@]}
  do
    line="${line} ${ALGORITHM} ^"

    i=0
    for COLUMN in ${COLUMNS[@]}
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

  for SIZE in ${SIZES[@]}
  do
    i=0
    for THREASHOLD in ${THREASHOLDS[@]}
    do
      if [ ${i} -eq 0 ]; then
        line="^ ${SIZE} |"
      else
        line="^ ::: |"
      fi

      for ALGORITHM in ${ALGORITHMS[@]}
      do

        csv=${DIR_RESULT}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}.csv

        j=0
        for COLUMN in ${COLUMNS[@]}
        do
          if [ -e ${csv} ]; then
            tmp=`tail -n +6 ${csv} | awk -F, '{ if ($1 == "nan") { timeout = 1 } if ($'${COLUMN}' != "nan") { sum += $'${COLUMN}' } } END{ if (timeout == 1) { print "'${STR_TIMEOUT}'" " (" sum/NR ")" } else { print sum/NR } }'`
          else
            tmp="${STR_SEGMENTATION_FAULT}"
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

for TEST in ${TESTS[@]}
do
  cat ${DIR_OUT}/${TEST}.txt
  echo ""
done > ${DIR_OUT}/all.txt

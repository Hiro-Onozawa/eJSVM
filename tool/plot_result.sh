#!/bin/bash

. ./params/arg_parser.sh "$@" || exit 1

if [[ $PROFILE = "TRUE" ]]; then
  echo "cannot use option \"PROFILE\""
  exit 1
fi
if [[ $PARAM -lt "1" ]] || [[ $PARAM -gt "6" ]]; then
echo "Usage: $(basename $0) [OPTIONS]  <--param Params>"
echo ""
echo "Params :"
echo "    1 : total_CPU_time (実行時間)"
echo "    2 : total_GC_time (総GC時間)"
echo "    3 : non_GC_time (非GC時間)"
echo "    4 : max_GC_time (最大GC時間)"
echo "    5 : avr_GC_time (平均GC時間)"
echo "    6 : GC_count (GC回数)"
exit 1
fi

DIR_INFILE=${DIR_DATS}/tmp
DIR_GRAPH=${DIR_DATS}/graph/plot

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
          tail +6 ${in} | cut -d "," -f ${PARAM} | awk -v s=${SIZE} '{ print s" "$1 }' >> ${out}
          echo "" >> ${out}
          echo "" >> ${out}
        else
          echo "# ファイルが存在しません" >> ${out}
          echo "${SIZE} -" >> ${out}
          echo "" >> ${out}
          echo "" >> ${out}
        fi
      done
    done
  done
done

rm -f ./error.log

for THREASHOLD in ${THREASHOLDS[@]}
do
  for TEST in ${TESTS[@]}
  do
    echo "plot to t${THREASHOLD}_${TEST}"
    gnuplot -e "indir='${DIR_INFILE}'; outdir='${DIR_GRAPH}'; benchname='${TEST}'; threashold='t${THREASHOLD}'; basebit='${BASEBIT}'; param=${PARAM}; lang='${USER_LANG}'" ./scripts/plot.gp 2>> ./error.log
    sed -i -e 's/<undefined>/x/g' "${DIR_GRAPH}/t${THREASHOLD}_${TEST}_values.txt"
  done
done

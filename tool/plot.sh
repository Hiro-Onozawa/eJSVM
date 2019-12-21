#!/bin/bash

# PARAM :
# 1 : total_CPU_time
# 2 : total_GC_time
# 3 : non_GC_time
# 4 : max_GC_time
# 5 : avr_GC_time
# 6 : GC_count

. ./params/arg_parser.sh "$@" || exit 1

if [[ $PROFILE = "TRUE" ]]; then
  echo "cannot use option \"PROFILE\""
  exit 1
fi
if [[ $PARAM -lt "1" ]] || [[ $PARAM -gt "6" ]]; then
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
          echo "" >> ${out}
        fi
      done
    done
  done
done

rm -f ./error.log

label_max=${SIZES[0]}
label_min=${SIZES[$((${#SIZES[@]}-1))]}
for THREASHOLD in ${THREASHOLDS[@]}
do
  for TEST in ${TESTS[@]}
  do
    echo "plot to t${THREASHOLD}_${TEST}"
    gnuplot -e "indir='${DIR_INFILE}'; outdir='${DIR_GRAPH}'; benchname='${TEST}'; threashold='t${THREASHOLD}'; basebit='${BASEBIT}'; param=${PARAM}; label_max='${label_max}'; label_min='${label_min}'; lang='${USER_LANG}'" ./scripts/plot.gp 2>> ./error.log
    sed -i -e 's/<undefined>/x/g' "${DIR_GRAPH}/t${THREASHOLD}_${TEST}_values.txt"
  done
done

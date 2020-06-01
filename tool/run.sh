#!/bin/bash

. ./params/arg_parser.sh "$@" || exit 1

echo "DIR_VMS : ${DIR_VMS}"
echo "PROFILE : ${PROFILE}"

if [[ $PROFILE = "TRUE" ]]; then
N=1
SUFFIX="_profile"
OPTION="-u --alloc-info --collect-info --moving-info --collect-time"
DIR_OUT=$DIR_PROFILE_RAW
else
N=50
SUFFIX=""
OPTION="-u"
DIR_OUT=$DIR_RESULT_RAW
fi

mkdir -p ${DIR_OUT}

date
for ALGORITHM in ${ALGORITHMS[@]}
do
  vm=${DIR_VMS}/${ALGORITHM}${SUFFIX}
  echo ${vm}
  for THREASHOLD in ${THREASHOLDS[@]}
  do
    for SIZE in ${SIZES[@]}
    do
      PARAM=`GEN_THREASHOLD_BYTE ${ALGORITHM} ${SIZE} ${THREASHOLD}`
      PARAM="-s 5000 -c ${SIZE} -b ${PARAM}"
      echo ${PARAM}
      for i in `seq 1 ${N}`
      do
        for TEST in ${TESTS[@]}
        do
          out=${DIR_OUT}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}${SUFFIX}.txt
          ${vm} ${OPTION} ${PARAM} ${DIR_TESTS}/${TEST}.sbc &>> ${out}
          if [ $? -eq 139 ]; then
            echo "Segmentation fault" >> ${out}
          fi
        done
      done
    done
  done
done
date

#!/bin/bash

. ./params/arg_parser.sh $@ || exit 1

echo "DIR_VMS : ${DIR_VMS}"
echo "PROFILE : ${PROFILE}"
echo "BASEBIT : ${BASEBIT}"

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
  for THREASHOLD in ${THREASHOLDS[@]}
  do
    for SIZE in ${SIZES[@]}
    do
      vm=${DIR_VMS}/ejsvm_${BASEBIT}_${ALGORITHM}_${SIZE}_t${THREASHOLD}${SUFFIX}
      echo ${vm}
      for i in `seq 1 ${N}`
      do
        for TEST in ${TESTS[@]}
        do
          out=${DIR_OUT}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}${SUFFIX}.txt
          ${vm} ${OPTION} ${DIR_TESTS}/${TEST}.sbc &>> ${out}
          if [ $? -eq 139 ]; then
            echo "Segmentation fault" >> ${out}
          fi
        done
      done
    done
  done
done
date

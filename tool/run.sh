#!/bin/bash

. ./params/arg_parser.sh "$@" || exit 1

echo "DIR_VMS : ${DIR_VMS}"
echo "PROFILE : ${PROFILE}"

if [[ $PROFILE = "TRUE" ]]; then
if [[ LOOPCNT = "" ]]; then
  LOOPCNT=1
fi
SUFFIX="_profile"
if [[ ${PARAM} = "" ]]; then
  OPTION="-u --alloc-info --collect-info --moving-info --collect-time"
else
  OPTION="${PARAM}"
fi
DIR_OUT=$DIR_PROFILE_RAW
else
if [[ LOOPCNT = "" ]]; then
  LOOPCNT=50
fi
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
    for TEST in ${TESTS[@]}
    do
      SBC="${DIR_TESTS}/${TEST}.sbc"
      for SIZE in ${SIZES[@]}
      do
        PARAM=`GEN_THREASHOLD_BYTE ${ALGORITHM} ${SIZE} ${THREASHOLD}`
        PARAM="${OPTION} -s 5000 -c ${SIZE} -b ${PARAM}"
        echo "${TEST}; ${PARAM}"
        ${vm} ${PARAM} ${SBC} &>> /dev/null
        for i in `seq 1 ${LOOPCNT}`
        do
          out=${DIR_OUT}/${ALGORITHM}_${SIZE}_t${THREASHOLD}_${TEST}${SUFFIX}.txt
          ${vm} ${PARAM} ${SBC} &>> ${out}
          if [ $? -eq 139 ]; then
            echo "Segmentation fault" >> ${out}
          fi
        done
      done
    done
  done
done
date

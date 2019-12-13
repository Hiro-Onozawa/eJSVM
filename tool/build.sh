#!/bin/bash

. ./params/arg_parser.sh $@ || exit 1

echo "DIR_VMS : ${DIR_VMS}"
echo "PROFILE : ${PROFILE}"
echo "BASEBIT : ${BASEBIT}"

DIR_BUILD=../build

if [[ $PROFILE = "TRUE" ]]; then
  SUFFIX="_profile"
else
  SUFFIX=""
fi

mkdir -p ${DIR_VMS}

cd `dirname $0`
DIR_CURRENT=`pwd`
echo ${DIR_CURRENT}
cd ${DIR_BUILD}

make clean

echo "OPT_BASEBIT = ${BASEBIT}" > BASEBIT.txt

for ALGORITHM in ${ALGORITHMS[@]}
do
  for THREASHOLD in ${THREASHOLDS[@]}
  do
    echo "OPT_GC_ALGORITHM = ${ALGORITHM}" > ALGORITHM.txt
    for SIZE in ${SIZES[@]}
    do
      rm -f *.o cell-header.h
      if [[ ${ALGORITHM} = "copy" ]]; then
        echo "CFLAGS += -DJS_SPACE_GC_THREASHOLD='(${SIZE}>>(${THREASHOLD}+1))'" > CFLAGS.txt
      else
        echo "CFLAGS += -DJS_SPACE_GC_THREASHOLD='(${SIZE}>>${THREASHOLD})'" > CFLAGS.txt
      fi
      if [[ $PROFILE = "TRUE" ]]; then
        echo "CFLAGS += -DGC_PROFILE" >> CFLAGS.txt
      fi
      echo "HEAPSIZE = -DJS_SPACE_BYTES=${SIZE}" > HEAPSIZE.txt
      echo "(${ALGORITHM}, ${SIZE}, ${THREASHOLD})"
#      cat ALGORITHM.txt HEAPSIZE.txt
      make -j &> /dev/null
      cp ejsvm ${DIR_CURRENT}/${DIR_VMS}/ejsvm_${BASEBIT}_${ALGORITHM}_${SIZE}_t${THREASHOLD}${SUFFIX}
    done
  done
done

#!/bin/bash

. ./params/arg_parser.sh "$@" || exit 1

echo "DIR_VMS : ${DIR_VMS}"
echo "PROFILE : ${PROFILE}"

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

make clean &> /dev/null

for ALGORITHM in ${ALGORITHMS[@]}
do
  for THREASHOLD in ${THREASHOLDS[@]}
  do
    for SIZE in ${SIZES[@]}
    do
      rm -f *.o cell-header.h
      
      echo "OPT_BASEBIT = ${BASEBIT}" > OPTIONS.txt
      echo "OPT_TARGET = ${TARGET}" >> OPTIONS.txt
      echo "OPT_GC_ALGORITHM = ${ALGORITHM}" >> OPTIONS.txt
      if [[ ${ALGORITHM} = "copy" ]]; then
        echo "CFLAGS += -DJS_SPACE_GC_THREASHOLD='(${SIZE}>>(${THREASHOLD}+1))'" >> OPTIONS.txt
      else
        echo "CFLAGS += -DJS_SPACE_GC_THREASHOLD='(${SIZE}>>${THREASHOLD})'" >> OPTIONS.txt
      fi
      if [[ $PROFILE = "TRUE" ]]; then
        echo "CFLAGS += -DGC_PROFILE" >> OPTIONS.txt
      fi
      echo "HEAPSIZE = -DJS_SPACE_BYTES=${SIZE}" >> OPTIONS.txt
      echo "(${ALGORITHM}, ${SIZE}, ${THREASHOLD})"
      make -j &> /dev/null
      cp ejsvm ${DIR_CURRENT}/${DIR_VMS}/${ALGORITHM}_${SIZE}_t${THREASHOLD}${SUFFIX}
    done
  done
done

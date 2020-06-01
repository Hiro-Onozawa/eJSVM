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
  rm -f *.o cell-header.h
  
  echo "OPT_BASEBIT = ${BASEBIT}" > OPTIONS.txt
  echo "OPT_TARGET = ${TARGET}" >> OPTIONS.txt
  echo "OPT_GC_ALGORITHM = ${ALGORITHM}" >> OPTIONS.txt
  echo "CFLAGS += -DUSE_DUMMY_GPIO -DUSE_DUMMY_SENSOR" >> OPTIONS.txt
  if [[ $PROFILE = "TRUE" ]]; then
    echo "CFLAGS += -DGC_PROFILE" >> OPTIONS.txt
  fi
  cat OPTIONS.txt
  make -j &> /dev/null
  cp ejsvm ${DIR_CURRENT}/${DIR_VMS}/${ALGORITHM}${SUFFIX}
done

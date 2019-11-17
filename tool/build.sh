#!/bin/bash

build_dir=../build
vms_dir=./vms
algorithms=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
threasholds=( 1 2 3 )
sizes=( 10485760 7864320 5242880 3932160 2621440 2162688 1310720 )


cd `dirname $0`
current_dir=`pwd`
echo ${current_dir}
cd ${build_dir}

for algorithm in ${algorithms[@]}
do
  for threashold in ${threasholds[@]}
  do
    echo "OPT_GC_ALGORITHM = ${algorithm}" > ALGORITHM.txt
    for size in ${sizes[@]}
    do
      rm *.o cell-header.h
      if [ ${algorithm} = "copy" ]; then
        echo "CFLAGS += -DJS_SPACE_GC_THREASHOLD='(${size}>>(${threashold}+1))'" > THREASHOLD.txt
      else
        echo "CFLAGS += -DJS_SPACE_GC_THREASHOLD='(${size}>>${threashold})'" > THREASHOLD.txt
      fi
      echo "HEAPSIZE = -DJS_SPACE_BYTES=${size}" > HEAPSIZE.txt
      echo "(${algorithm}, ${size}, ${threashold})"
#      cat ALGORITHM.txt HEAPSIZE.txt
      make -j &> /dev/null
      cp ejsvm ${current_dir}/${vms_dir}/ejsvm_64_${algorithm}_${size}_t${threashold}
    done
  done
done

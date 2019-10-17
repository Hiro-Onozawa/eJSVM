#!/bin/bash

BUILD_DIRS=builds
# RUN_TESTS="python ../../run-tests.py --config-file=./config --exit0"
RUN_TESTS="python3 ../../run-tests.py --config-file=./config --exit0"
DIRS=""

DIRS="$DIRS x64"
DIRS="$DIRS x86"

declare -A CONFIGS;

CONFIGS=(
    ["x64"]="x64"
    ["x64_obc"]="x64"
    ["x86"]="x86"
    ["x86_obc"]="x86"
)

function cleanest() {
    local dir=$1

    echo %%%%%%%
    echo %% cleanest $dir
    echo %%%%%%%
    
    (cd $BUILD_DIRS/$dir;\
     make cleanest;/
     rm -rf bc; rm -rf vm_result)
}

function build() {
    local dir=$1

    echo %%%%%%%
    echo %% build $dir
    echo %%%%%%%
    
    (cd $BUILD_DIRS/$dir; make -j)
}

function test() {
    local dir=$1
    local config=$2

    echo %%%%%%%
    echo %% testing $config
    echo %%%%%%%

    $RUN_TESTS -c $config --work-dir=$BUILD_DIRS/$dir
}

while [ x"$1" \!= x ]; do
    case "$1" in
	-r)
	    cleanest=yes
	    build=yes
	    test=yes
	    shift
	    ;;
	-t)
	    cleanest=no
	    build=no
	    test=yes
	    shift
	    ;;
	-b)
	    cleanest=yes
	    build=yes
	    test=no
	    shift
	    ;;
	-c)
	    cleanest=yes
	    build=no
	    test=no
	    shift
	    ;;
	*)
	    break
    esac
done

if [ x$1 = xcleanest ]; then
    cleanest=yes
    shift
fi
if [ x$1 = xbuild ]; then
    build=yes
    shift
fi
if [ x$1 = xtest ]; then
    test=yes
    shift
fi

if [ x$1 = x ]; then
    for d in $DIRS; do
        echo $d
        if [ x$cleanest = xyes ]; then
            cleanest $d || exit 1
        fi
        if [ x$build = xyes ]; then
            build $d || exit 1
        fi
    done
    for CONFIG in ${!CONFIGS[@]};
    do
        if [ x$test = xyes ]; then
            test ${CONFIGS[$CONFIG]} $CONFIG || exit 1
        fi
    done
fi

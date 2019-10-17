#!/bin/bash

BUILD_DIRS=builds
RUN_TESTS="python ../../run-tests.py --config-file=./config --exit0"
CONFIGS=""

# test overall
# Check if VM can be compiled with PROFILE on, GC_PROF and HC_PROF off,
# and superinstructions off ;these flags are toggled in the following tests.
CONFIGS="$CONFIGS sbc"

# ALLOC_CACHE
CONFIGS="$CONFIGS sbc_alloc"

# INLINE_CACHE
CONFIGS="$CONFIGS sbc_alloc_inl"

# VMDL (with simple configuration, super insn off) 
CONFIGS="$CONFIGS vmdl_sbc"

# VMDL (superinsn 3)
CONFIGS="$CONFIGS vmdl_sbc_alloc_inl_si3"

# VMDL (superinsn 4)
CONFIGS="$CONFIGS vmdl_sbc_alloc_inl_si4"

## VMDL (limited operand spec)
#CONFIGS="$CONFIGS vmdl_sbc_fixnum"

# OBC
CONFIGS="$CONFIGS obc_alloc_inl"

# handcrafted (without superinsn)
CONFIGS="$CONFIGS hand_alloc_inl"

function build() {
    local config=$1
    local rebuild=$2

    echo %%%%%%%
    echo %% building $config
    echo %%%%%%%
    
    (cd $BUILD_DIRS/$config;\
     if [ x$rebuild \!= x ]; then\
	    make cleanest;\
     fi;\
     make -j)
}

function test() {
    local config=$1

    echo %%%%%%%
    echo %% testing $config
    echo %%%%%%%

    $RUN_TESTS -c $config --work-dir=$BUILD_DIRS/$config
}

while [ x"$1" \!= x ]; do
    case "$1" in
	-r)
	    rebuild=yes
	    shift
	    ;;
	-b)
	    no_test=yes
	    shift
	    ;;
	-t)
	    no_build=yes
	    shift
	    ;;
	*)
	    break
    esac
done

if [ x$1 = xrebuild ]; then
    rebuild=yes
    shift
fi

if [ x$1 = x ]; then
    if [ x$no_build \!= xyes ]; then
	for c in $CONFIGS; do
	    echo $c
	    build $c $rebuild || exit 1
	done
    fi
    if [ x$no_test \!= xyes ]; then
	for c in $CONFIGS; do
	    test $c
	done
    fi
else
    build $1 $rebuild || exit 1
    test $1
fi

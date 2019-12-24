#!/bin/bash

PROGNAME=$(basename $0)
VERSION="1.0?"

usage() {
    echo "Usage: $PROGNAME [OPTIONS] FILE"
    echo "  This script is ~."
    echo
    echo "Options:"
    echo "  -h, --help"
    echo "      --version"
    echo "  -b, --basebit { 32 | 64 }"
    echo "      --target { 32 | 64 }"
    echo "  -s, --size { big | small | all }"
    echo "      --algorithm \"{ null | mark_sweep | mark_compact | threaded_compact | copy }\""
    echo "      --benchmark \"{ 3d-cube | 3d-morph | base64 | binaryTree | cordic | fasta | spectralnorm | string-intensive }\""
    echo "      --threashold \"{ 1 | 2 | 3 }\""
#    echo "  -a, --long-a [ARG]"
    echo "  -p, --profile"
    echo "      --param <param>"
    echo "  -l, --lang { jp | en }"
    echo
    exit 1
}

for OPT in "$@"
do
    case $OPT in
        -h | --help)
            usage
            exit 1
            ;;
        --version)
            echo $VERSION
            exit 1
            ;;
        -b | --basebit)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            if [[ $2 -ne 32 ]] && [[ $2 -ne 64 ]]; then
                echo "$PROGNAME: option requires an argument { 32 | 64 } -- $1" 1>&2
                exit 1
            fi
            BASEBIT=$2
            shift 2
            ;;
             --target)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            if [[ $2 -ne 32 ]] && [[ $2 -ne 64 ]]; then
                echo "$PROGNAME: option requires an argument { 32 | 64 } -- $1" 1>&2
                exit 1
            fi
            TARGET=$2
            shift 2
            ;;
        -s | --size)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            if [[ "$2" != "big" ]] && [[ "$2" != "small" ]] && [[ "$2" != "all" ]]; then
                echo "$PROGNAME: option requires an argument { big | small | all } -- $1" 1>&2
                exit 1
            fi
            SIZE_TYPE=$2
            shift 2
            ;;
             --algorithm)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            ALGORITHMS=(${2// / })
            for ALGORITHM in ${ALGORITHMS[@]}
            do
                if [[ "$ALGORITHM" != "null" ]] && [[ "$ALGORITHM" != "mark_sweep" ]] && [[ "$ALGORITHM" != "mark_compact" ]] && [[ "$ALGORITHM" != "threaded_compact" ]] && [[ "$ALGORITHM" != "copy" ]]; then
                    echo "$PROGNAME: option requires an argument { null | mark_sweep | mark_compact | threaded_compact | copy } -- $1" 1>&2
                    exit 1
                fi
            done
            shift 2
            ;;
             --benchmark)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            TESTS=(${2// / })
            for TEST in ${TESTS[@]}
            do
                if [[ "$TEST" != "3d-cube" ]] && [[ "$TEST" != "3d-morph" ]] && [[ "$TEST" != "base64" ]] && [[ "$TEST" != "binaryTree" ]] && [[ "$TEST" != "cordic" ]] && [[ "$TEST" != "fasta" ]] && [[ "$TEST" != "spectralnorm" ]] && [[ "$TEST" != "string-intensive" ]]; then
                    echo "$PROGNAME: option requires an argument { 3d-cube | 3d-morph | base64 | binaryTree | cordic | fasta | spectralnorm | string-intensive } -- $1" 1>&2
                    exit 1
                fi
            done
            shift 2
            ;;
             --threashold)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            THREASHOLDS=(${2// / })
            for THREASHOLD in ${THREASHOLDS[@]}
            do
                if [[ "$THREASHOLD" != "1" ]] && [[ "$THREASHOLD" != "2" ]] && [[ "$THREASHOLD" != "3" ]]; then
                    echo "$PROGNAME: option requires an argument { 1 | 2 | 3 } -- $1" 1>&2
                    exit 1
                fi
            done
            shift 2
            ;;
#        -a | --long-a)
#            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
#                shift
#            else
#                shift 2
#            fi
#            ;;
        -p | --profile)
            PROFILE="TRUE"
            shift 1
            ;;
             --param)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            PARAM="$2"
            shift 2
            ;;
        -l | --lang)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            if [[ "$2" != "jp" ]] && [[ "$2" != "en" ]]; then
                echo "$PROGNAME: option requires an argument { jp | en } -- $1" 1>&2
                exit 1
            fi
            USER_LANG=$2
            shift 2
            ;;
        -- | -)
            shift 1
            param+=( "$@" )
            break
            ;;
        -*)
            echo "$PROGNAME: illegal option -- '$(echo $1 | sed 's/^-*//')'" 1>&2
            exit 1
            ;;
        *)
            if [[ ! -z "$1" ]] && [[ ! "$1" =~ ^-+ ]]; then
                #param=( ${param[@]} "$1" )
                param+=( "$1" )
                shift 1
            fi
            ;;
    esac
done

DIR_DATS_32=./dats_32
DIR_VMS_32=$DIR_DATS_32/vms
DIR_RESULT_32=$DIR_DATS_32/results
DIR_RESULT_RAW_32=$DIR_DATS_32/results/raw
DIR_PROFILE_32=$DIR_DATS_32/profiles
DIR_PROFILE_RAW_32=$DIR_DATS_32/profiles/raw

DIR_DATS_64=./dats_64
DIR_VMS_64=$DIR_DATS_64/vms
DIR_RESULT_64=$DIR_DATS_64/results
DIR_RESULT_RAW_64=$DIR_DATS_64/results/raw
DIR_PROFILE_64=$DIR_DATS_64/profiles
DIR_PROFILE_RAW_64=$DIR_DATS_64/profiles/raw

if [[ $ALGORITHMS = "" ]]; then
    ALGORITHMS=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
fi
if [[ $TESTS = "" ]]; then
    TESTS=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "fasta" "spectralnorm" "string-intensive" )
fi
if [[ $THREASHOLDS = "" ]]; then
    THREASHOLDS=( 1 2 3 )
fi
SIZES_ALL=( 10485760 7864320 5242880 3932160 2621440 1966080 1310720 983040 655360 491520 )
SIZES_BIG=( 10485760 7864320 5242880 3932160 2621440 1966080 1310720 )
SIZES_SMALL=( 3932160 2621440 1966080 1310720 983040 655360 491520 )

DIR_TESTS=../regression-test/bc/testcases
DIR_BIN=./bin

if [[ $BASEBIT -eq 32 ]]; then
    ARCHITECTURE="x86"
    if [[ $TARGET = "" ]]; then
        TARGET=32
    fi
    SIZES=(${SIZES_SMALL[@]})
    DIR_DATS=$DIR_DATS_32
    DIR_VMS=$DIR_VMS_32
    DIR_RESULT=$DIR_RESULT_32
    DIR_RESULT_RAW=$DIR_RESULT_RAW_32
    DIR_PROFILE=$DIR_PROFILE_32
    DIR_PROFILE_RAW=$DIR_PROFILE_RAW_32
else
    BASEBIT=64
    ARCHITECTURE="x64"
    if [[ $TARGET = "" ]]; then
        TARGET=64
    fi
    SIZES=(${SIZES_BIG[@]})
    DIR_DATS=$DIR_DATS_64
    DIR_VMS=$DIR_VMS_64
    DIR_RESULT=$DIR_RESULT_64
    DIR_RESULT_RAW=$DIR_RESULT_RAW_64
    DIR_PROFILE=$DIR_PROFILE_64
    DIR_PROFILE_RAW=$DIR_PROFILE_RAW_64
fi

if [[ $SIZE_TYPE = "big" ]]; then
    SIZES=(${SIZES_BIG[@]})
fi
if [[ $SIZE_TYPE = "small" ]]; then
    SIZES=(${SIZES_SMALL[@]})
fi
if [[ $SIZE_TYPE = "all" ]]; then
    SIZES=(${SIZES_ALL[@]})
fi

if [[ $USER_LANG = "" ]]; then
    USER_LANG="jp"
fi

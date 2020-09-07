#!/bin/bash

PROGNAME=$(basename $0)
VERSION="1.1"

usage() {
    echo "Usage: $PROGNAME [OPTIONS] FILE"
    echo "  This script is ~."
    echo
    echo "Options:"
    echo "  -h, --help"
    echo "      --version"
    echo "  -b, --basebit { 32 | 64 }"
    echo "      --target { 32 | 64 }"
#    echo "  -s, --size { big | small | all }"
    echo "      --algorithm \"{ null | mark_sweep | mark_compact | threaded_compact | copy }\""
    echo "      --benchmark \"{ 3d-cube | 3d-morph | base64 | binaryTree | cordic | fasta | spectralnorm | string-intensive }\""
    echo "      --threashold \"{ 1 | 2 | 3 }\""
    echo "      --size-set \"<size> ...\""
#    echo "  -a, --long-a [ARG]"
    echo "  -p, --profile"
    echo "      --param <param>"
    echo "      --loop-count <number>"
    echo "      --dats-name <name>"
    echo "      --out-dir <dir name>"
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
#        -s | --size)
#            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
#                echo "$PROGNAME: option requires an argument -- $1" 1>&2
#                exit 1
#            fi
#            if [[ "$2" != "big" ]] && [[ "$2" != "small" ]] && [[ "$2" != "all" ]]; then
#                echo "$PROGNAME: option requires an argument { big | small | all } -- $1" 1>&2
#                exit 1
#            fi
#            SIZE_TYPE=$2
#            shift 2
#            ;;
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
                if [[ "$TEST" != "3d-cube" ]] && [[ "$TEST" != "3d-morph" ]] && [[ "$TEST" != "base64" ]] && [[ "$TEST" != "binaryTree" ]] && [[ "$TEST" != "cordic" ]] && [[ "$TEST" != "dht11" ]] && [[ "$TEST" != "fasta" ]] && [[ "$TEST" != "spectralnorm" ]] && [[ "$TEST" != "string-intensive" ]]; then
                    echo "$PROGNAME: option requires an argument { 3d-cube | 3d-morph | base64 | binaryTree | cordic | dht11 | fasta | spectralnorm | string-intensive } -- $1" 1>&2
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
             --size-set)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            SIZES=(${2// / })
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
             --loop-count)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            LOOPCNT="$2"
            shift 2
            ;;
             --dats-name)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            DATSNAME="$2"
            shift 2
            ;;
             --out-dir)
            if [[ -z "$2" ]] || [[ "$2" =~ ^-+ ]]; then
                echo "$PROGNAME: option requires an argument -- $1" 1>&2
                exit 1
            fi
            OUTDIR="$2"
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

if [[ $DATSNAME = "" ]]; then
  DIR_DATS_NAME=dats
else
  DIR_DATS_NAME=$DATSNAME
fi
DIR_VMS_NAME=vms
if [[ OUTDIR = "" ]]; then
  DIR_RESULT_NAME=results
  DIR_PROFILE_NAME=profiles
else
  DIR_RESULT_NAME=${OUTDIR}
  DIR_PROFILE_NAME=${OUTDIR}
fi
DIR_RESULT_RAW_NAME=raw
DIR_PROFILE_RAW_NAME=raw

if [[ $BASEBIT = "" ]]; then
    BASEBIT=64
fi
if [[ $TARGET = "" ]]; then
    TARGET=$BASEBIT
fi

if [[ $ALGORITHMS = "" ]]; then
    ALGORITHMS=( "mark_sweep" "mark_compact" "threaded_compact" "copy" )
fi
if [[ $TESTS = "" ]]; then
    TESTS=( "3d-cube" "3d-morph" "base64" "binaryTree" "cordic" "dht11" "fasta" "spectralnorm" "string-intensive" )
fi
if [[ $THREASHOLDS = "" ]]; then
    THREASHOLDS=( 1 2 3 )
fi
if [[ $SIZES = "" ]]; then
    SIZES=( 491520 655360 983040 1310720 1966080 2621440 3932160 5242880 7864320 10485760 )
fi
if [[ $USER_LANG = "" ]]; then
    USER_LANG="jp"
fi

DIR_TESTS=../regression-test/bc/testcases
DIR_BIN=./bin

DIR_DATS=./${DIR_DATS_NAME}/${BASEBIT}_${TARGET}
DIR_VMS=${DIR_DATS}/$DIR_VMS_NAME
DIR_RESULT=${DIR_DATS}/$DIR_RESULT_NAME
DIR_RESULT_RAW=${DIR_RESULT}/$DIR_RESULT_RAW_NAME
DIR_PROFILE=${DIR_DATS}/$DIR_PROFILE_NAME
DIR_PROFILE_RAW=${DIR_PROFILE}/$DIR_PROFILE_RAW_NAME


# $1 : GC algorithm
# $2 : Heap size
# $3 : Threashold type
function GEN_THREASHOLD_BYTE () {
  if [[ $1 = "copy" ]]; then
    echo $(($2>>$(($3+1))))
  else
    echo $(($2>>$3))
  fi
}

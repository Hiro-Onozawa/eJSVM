EJSVM_DIR = ../../../../../ejs/ejsvm

.PRECIOUS: %.c %.h %.o

##
## configuration files
##

DATATYPES = $(EJSVM_DIR)/datatypes/default.def
OPERANDSPEC = $(EJSVM_DIR)/operand-spec/any.spec
# OPERANDSPEC = $(EJSVM_DIR)/operand-spec/fixnum-unspec.spec
SUPERINSNSPEC = $(EJSVM_DIR)/superinsn-spec/all.si
SUPERINSNTYPE = 4
#VMDLC_FLAGS=-no-match-opt

##
## compile flags
##

#CFLAGS = -O2 -DNDEBUG -UDEBUG $(HEAPSIZE)
CFLAGS = -O2 -DNDEBUG -UDEBUG $(HEAPSIZE)
# CFLAGS = -Os -DNDEBUG -UDEBUG $(HEAPSIZE)
# CFLAGS = -O0 -g $(HEAPSIZE)
INSNGEN_FLAGS = -Xgen:pad_cases true -Xcmp:opt_pass MR:S -Xcmp:rand_seed 0

##
## VMDL
##
USE_VMDL=false

##
## commands
##

CC = gcc
SED = gsed
# SED = sed
RUBY = ruby
PYTHON = python
# PYTHON = python3
# CPP_VMDL = $(CPP) -E -x c -P
COCCINELLE = spatch
# COCCINELLE = spatch --python python3

##
## paths
##

INCLUDES =
# INCLUDES = -I/opt/local/include
LIBS =
# LIBS = -L/opt/local/lib

## 
## options
## 

OPT_REGEXP    = none
# OPT_REGEXP    = oniguruma
OPT_GC        = native
# OPT_GC        = none

## heap size in bytes
HEAPSIZE      = -DJS_SPACE_BYTES=10485760

## enable profiling to create customised VM
# CFLAGS       += -DPROFILE

##
## developer options
##

## accept SBC files
#CFLAGS       += -DUSE_SBC
## GC collects internal nodes in hidden class graph
CFLAGS       += -DHC_SKIP_INTERNAL
## cache shapes at allocation site
CFLAGS       += -DALLOC_SITE_CACHE
## GC collects unused shapes (effective if ALLOC_SITE_CACHE is specified)
CFLAGS       += -DWEAK_SHAPE_LIST
## use inlne cache
CFLAGS       += -DINLINE_CACHE

## show GC profiling information (make --gc-prof available)
CFLAGS       += -DGC_PROF
## show hidden class related profiling information (make --hc-prof available)
CFLAGS       += -DHC_PROF

include $(EJSVM_DIR)/common.mk

# EJSVM_DIR = ../../../../../ejs/ejsvm

.PRECIOUS: %.c %.h %.o

##
## configuration files
##

# DATATYPES = $(EJSVM_DIR)/datatypes/default.def
# DATATYPES = $(EJSVM_DIR)/datatypes/default_32.def
OPERANDSPEC = $(EJSVM_DIR)/operand-spec/any.spec
# OPERANDSPEC = $(EJSVM_DIR)/operand-spec/fixnum-unspec.spec
# SUPERINSNSPEC = $(EJSVM_DIR)/superinsn-spec/all.si
# SUPERINSNTYPE = 4

##
## compile flags
##

CFLAGS = -O2 -DNDEBUG -UDEBUG $(HEAPSIZE)
# CFLAGS = -Os -DNDEBUG -UDEBUG $(HEAPSIZE)
# CFLAGS = -O0 -g $(HEAPSIZE)
INSNGEN_FLAGS = -Xgen:pad_cases true -Xcmp:opt_pass MR:S -Xcmp:rand_seed 0

##
## commands
##

CC = gcc
# SED = gsed
SED = sed
RUBY = ruby
# PYTHON = python
PYTHON = python3
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

# OPT_BASEBIT   = 64
# OPT_BASEBIT   = 32
OPT_REGEXP    = none
# OPT_REGEXP    = oniguruma
OPT_GC        = native
# OPT_GC        = boehmgc
# OPT_GC        = none
HEAPSIZE      = -DJS_SPACE_BYTES=10485760

CFLAGS       += -DUSE_SBC
CFLAGS       += -DUSE_OBC

# CFLAGS       += -DPROFILE

include $(EJSVM_DIR)/common.mk

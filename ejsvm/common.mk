.PHONY: check clean

######################################################

ifeq ($(EJSVM_DIR),)
    EJSVM_DIR=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
endif

######################################################
# default values

ifeq ($(CC),cc)
    CC = clang
endif
ifeq ($(SED),)
    SED = gsed
endif
ifeq ($(RUBY),)
    RUBY = ruby
endif
ifeq ($(PYTHON),)
    PYTHON = python
endif
ifeq ($(COCCINELLE),)
    COCCINELLE = spatch
endif
ifeq ($(OPT_DEBUG),)
# DEBUG=true|false
    OPT_DEBUG=false
endif
ifeq ($(OPT_BASEBIT),)
# BASEBIT=32|64
    OPT_BASEBIT=64
endif
ifeq ($(OPT_GC),)
# GC=native|boehmgc|none
    OPT_GC=native
endif
ifeq ($(OPT_GC_ALGORITHM),)
# GC_ALGORITHM=mark_sweep|mark_compact|threaded_compact|copy
    OPT_GC_ALGORITHM=mark_sweep
endif
#ifeq ($(SUPERINSNSPEC),)
#    SUPERINSNSPEC=none
#endif
#ifeq ($(SUPERINSNTYPE),)
#    SUPERINSNTYPE=4
#endif
ifeq ($(OPT_REGEXP),)
# REGEXP=oniguruma|none
    OPT_REGEXP=none
endif

######################################################
# commands and paths

ifeq ($(SUPERINSNTYPE),)
GOTTA=$(PYTHON) $(EJSVM_DIR)/gotta.py\
    --otspec $(OPERANDSPEC)\
    --insndef $(EJSVM_DIR)/instructions.def
else
GOTTA=$(PYTHON) $(EJSVM_DIR)/gotta.py\
    --sispec $(SUPERINSNSPEC)\
    --otspec $(OPERANDSPEC)\
    --insndef $(EJSVM_DIR)/instructions.def\
    --sitype $(SUPERINSNTYPE)
endif

#SILIST=$(GOTTA) --silist --sispec
SILIST=$(SED) -e 's/^.*: *//'

EJSC_DIR=$(EJSVM_DIR)/../ejsc
EJSC=$(EJSC_DIR)/newejsc.jar

VMGEN_DIR=$(EJSVM_DIR)/../vmgen
VMGEN=$(VMGEN_DIR)/vmgen.jar

EJSI_DIR=$(EJSVM_DIR)/../ejsi
EJSI=$(EJSI_DIR)/ejsi

INSNGEN=java -cp $(VMGEN) vmgen.InsnGen
TYPESGEN=java -cp $(VMGEN) vmgen.TypesGen
SPECGEN=java -cp $(VMGEN) vmgen.SpecFileGen
CPP=$(CC) -E

CFLAGS += -std=gnu89 -Wall -Wno-unused-label -DUSER_DEF $(INCLUDES)
LIBS   += -lm

######################################################
# superinstructions

ifeq ($(SUPERINSNTYPE),1)
    SUPERINSN_MAKEINSN=true
    SUPERINSN_CUSTOMIZE_OT=false
    SUPERINSN_PSEUDO_IDEF=false
    SUPERINSN_REORDER_DISPATCH=false
else ifeq ($(SUPERINSNTYPE),2)
    SUPERINSN_MAKEINSN=true
    SUPERINSN_CUSTOMIZE_OT=true
    SUPERINSN_PSEUDO_IDEF=false
    SUPERINSN_REORDER_DISPATCH=false
else ifeq ($(SUPERINSNTYPE),3)
    SUPERINSN_MAKEINSN=true
    SUPERINSN_CUSTOMIZE_OT=true
    SUPERINSN_PSEUDO_IDEF=true
    SUPERINSN_REORDER_DISPATCH=false
else ifeq ($(SUPERINSNTYPE),4)
    SUPERINSN_MAKEINSN=false
    SUPERINSN_CUSTOMIZE_OT=false
    SUPERINSN_PSEUDO_IDEF=false
    SUPERINSN_REORDER_DISPATCH=true
else ifeq ($(SUPERINSNTYPE),5)
    SUPERINSN_MAKEINSN=false
    SUPERINSN_CUSTOMIZE_OT=false
    SUPERINSN_PSEUDO_IDEF=false
    SUPERINSN_REORDER_DISPATCH=false
endif

######################################################

GENERATED_HFILES = \
    instructions-opcode.h \
    instructions-table.h \
    instructions-label.h \
    cell-header.h \
    specfile-fingerprint.h

HFILES = $(GENERATED_HFILES) \
    prefix.h \
    context.h \
    header.h \
    builtin.h \
    hash.h \
    instructions.h \
    types.h \
    globals.h \
    extern.h \
    log.h \
    gc.h

SUPERINSNS = $(shell $(GOTTA) --list-si)

OFILES = \
    allocate.o \
    builtin-array.o \
    builtin-boolean.o \
    builtin-global.o \
    builtin-math.o \
    builtin-number.o \
    builtin-object.o \
    builtin-regexp.o \
    builtin-string.o \
    builtin-function.o \
    call.o \
    codeloader.o \
    context.o \
    conversion.o \
    hash.o \
    init.o \
    string.o \
    object.o \
    operations.o \
    vmloop.o \
    gc.o \
    main.o

ifeq ($(SUPERINSN_MAKEINSN),true)
  INSN_SUPERINSNS = $(patsubst %,insns/%.inc,$(SUPERINSNS))
endif

INSN_GENERATED = \
    insns/add.inc \
    insns/bitand.inc \
    insns/bitor.inc \
    insns/call.inc \
    insns/div.inc \
    insns/eq.inc \
    insns/equal.inc \
    insns/getprop.inc \
    insns/leftshift.inc \
    insns/lessthan.inc \
    insns/lessthanequal.inc \
    insns/mod.inc \
    insns/mul.inc \
    insns/new.inc \
    insns/rightshift.inc \
    insns/setprop.inc \
    insns/sub.inc \
    insns/tailcall.inc \
    insns/unsignedrightshift.inc \
    insns/error.inc \
    insns/fixnum.inc \
    insns/geta.inc \
    insns/getarg.inc \
    insns/geterr.inc \
    insns/getglobal.inc \
    insns/getglobalobj.inc \
    insns/getlocal.inc \
    insns/instanceof.inc \
    insns/isobject.inc \
    insns/isundef.inc \
    insns/jump.inc \
    insns/jumpfalse.inc \
    insns/jumptrue.inc \
    insns/localcall.inc \
    insns/makeclosure.inc \
    insns/makeiterator.inc \
    insns/move.inc \
    insns/newframe.inc \
    insns/nextpropnameidx.inc \
    insns/not.inc \
    insns/number.inc \
    insns/pushhandler.inc \
    insns/seta.inc \
    insns/setarg.inc \
    insns/setarray.inc \
    insns/setfl.inc \
    insns/setglobal.inc \
    insns/setlocal.inc \
    insns/specconst.inc \
    insns/typeof.inc \
    insns/end.inc \
    insns/localret.inc \
    insns/nop.inc \
    insns/pophandler.inc \
    insns/poplocal.inc \
    insns/ret.inc \
    insns/throw.inc \
    insns/unknown.inc

INSN_HANDCRAFT = 

CFILES = $(patsubst %.o,%.c,$(OFILES))
CHECKFILES = $(patsubst %.c,$(CHECKFILES_DIR)/%.c,$(CFILES))
INSN_FILES = $(INSN_SUPERINSNS) $(INSN_GENERATED) $(INSN_HANDCRAFT)

######################################################

ifeq ($(OPT_DEBUG),true)
    CFLAGS+=-DDEBUG -UNDEBUG
endif
ifeq ($(OPT_DEBUG),false)
    CFLAGS+=-DNDEBUG -UDEBUG
endif

ifeq ($(OPT_BASEBIT),32)
    CCOPT=-m32
    CFLAGS+=-DBIT_32=1
endif
ifeq ($(OPT_BASEBIT),64)
    CFLAGS+=-DBIT_64=1
endif

ifeq ($(OPT_GC),native)
    CFLAGS+=-DUSE_NATIVEGC=1
endif
ifeq ($(OPT_GC),boehmgc)
    CFLAGS+=-DUSE_BOEHMGC=1
    LIBS+=-lgc
endif
ifeq ($(OPT_GC_ALGORITHM),mark_sweep)
    CFLAGS+=-DGC_MARK_SWEEP
endif
ifeq ($(OPT_GC_ALGORITHM),mark_compact)
    CFLAGS+=-DGC_MARK_COMPACT
endif
ifeq ($(OPT_GC_ALGORITHM),threaded_compact)
    CFLAGS+=-DGC_THREADED_COMPACT
endif
ifeq ($(OPT_GC_ALGORITHM),copy)
    CFLAGS+=-DGC_COPY
endif
ifeq ($(OPT_REGEXP),oniguruma)
    CFLAGS+=-DUSE_REGEXP=1
    LIBS+=-lonig
endif

ifeq ($(DATATYPES),)
    GENERATED_HFILES += types-handcraft.h
else
    CFLAGS += -DUSE_TYPES_GENERATED=1
    GENERATED_HFILES += types-generated.h
endif

CHECKFILES_DIR = checkfiles
GCCHECK_PATTERN = ../gccheck.cocci

######################################################

all: ejsvm ejsc.jar ejsi

ejsc.jar: $(EJSC)
	cp $< $@

ejsi: $(EJSI)
	cp $< $@

ejsvm :: $(OFILES) ejsvm.spec
	$(CC) $(CCOPT) $(LDFLAGS) -o $@ $(OFILES) $(LIBS)

instructions-opcode.h: $(EJSVM_DIR)/instructions.def $(SUPERINSNSPEC)
	$(GOTTA) --gen-insn-opcode -o $@

instructions-table.h: $(EJSVM_DIR)/instructions.def $(SUPERINSNSPEC)
	$(GOTTA) --gen-insn-table -o $@

instructions-label.h: $(EJSVM_DIR)/instructions.def $(SUPERINSNSPEC)
	$(GOTTA) --gen-insn-label -o $@

vmloop-cases.inc: $(EJSVM_DIR)/instructions.def
	cp $(EJSVM_DIR)/gen-vmloop-cases-nonaka.rb ./gen-vmloop-cases-nonaka.rb
	$(GOTTA) --gen-vmloop-cases -o $@

ifeq ($(SUPERINSNTYPE),)
ejsvm.spec specfile-fingerprint.h: $(EJSVM_DIR)/instructions.def $(VMGEN)
	$(SPECGEN) --insndef $(EJSVM_DIR)/instructions.def -o ejsvm.spec\
		--fingerprint specfile-fingerprint.h
else
ejsvm.spec specfile-fingerprint.h: $(EJSVM_DIR)/instructions.def $(SUPERINSNSPEC) $(VMGEN)
	$(SPECGEN) --insndef $(EJSVM_DIR)/instructions.def\
		--sispec $(SUPERINSNSPEC) -o ejsvm.spec\
		--fingerprint specfile-fingerprint.h
endif

$(INSN_HANDCRAFT):insns/%.inc: $(EJSVM_DIR)/insns-handcraft/%.inc
	mkdir -p insns
	cp $< $@

ifeq ($(DATATYPES),)
$(INSN_GENERATED):insns/%.inc: $(EJSVM_DIR)/insns-handcraft/%.inc
	mkdir -p insns
	cp $< $@
else ifeq ($(SUPERINSN_REORDER_DISPATCH),true)
$(INSN_GENERATED):insns/%.inc: $(EJSVM_DIR)/insns-def/%.idef $(VMGEN)
	mkdir -p insns
	$(INSNGEN) $(INSNGEN_FLAGS) \
		-Xgen:type_label true \
		-Xcmp:tree_layer \
		`$(GOTTA) --print-dispatch-order $(patsubst insns/%.inc,%,$@)`\
		-Xgen:type_label true \
		$(DATATYPES) $< $(OPERANDSPEC) insns
else
$(INSN_GENERATED):insns/%.inc: $(EJSVM_DIR)/insns-def/%.idef $(VMGEN)
	mkdir -p insns
	$(INSNGEN) $(INSNGEN_FLAGS) \
		-Xgen:type_label true \
		-Xcmp:tree_layer p0:p1:p2:h0:h1:h2 \
		-Xgen:type_label true \
		$(DATATYPES) $< $(OPERANDSPEC) insns
endif

# generate si-otspec/*.ot for each superinsns
SI_OTSPEC_DIR = si/otspec
SI_OTSPECS = $(patsubst %,$(SI_OTSPEC_DIR)/%.ot,$(SUPERINSNS))
ifeq ($(SUPERINSN_CUSTOMIZE_OT),true)
$(SI_OTSPECS): $(OPERANDSPEC) $(SUPERINSNSPEC)
	mkdir -p $(SI_OTSPEC_DIR)
	$(GOTTA) --gen-ot-spec $(patsubst $(SI_OTSPEC_DIR)/%.ot,%,$@) -o $@
else
$(SI_OTSPECS): $(OPERANDSPEC)
	mkdir -p $(SI_OTSPEC_DIR)
	cp $< $@
endif


# generate insns/*.inc for each superinsns
ifeq ($(DATATYPES),)
$(INSN_SUPERINSNS):
	echo "Superinstruction needs DATATYPES specified"
	exit 1
else

SI_IDEF_DIR = si/idefs
orig_insn = \
    $(shell $(GOTTA) --print-original-insn-name $(patsubst insns/%.inc,%,$1))
tmp_idef = $(SI_IDEF_DIR)/$(patsubst insns/%.inc,%,$1).idef

ifeq ($(SUPERINSN_PSEUDO_IDEF),true)
$(INSN_SUPERINSNS):insns/%.inc: $(EJSVM_DIR)/insns-def/* $(SUPERINSNSPEC) $(SI_OTSPEC_DIR)/%.ot $(VMGEN)
	mkdir -p $(SI_IDEF_DIR)
	$(GOTTA) \
	    --gen-pseudo-idef $(call orig_insn,$@) \
	    -o $(call tmp_idef,$@)
	mkdir -p insns
	$(INSNGEN) $(INSNGEN_FLAGS) \
	    -Xgen:label_prefix $(patsubst insns/%.inc,%,$@) \
	    -Xcmp:tree_layer p0:p1:p2:h0:h1:h2 $(DATATYPES) \
	    $(call tmp_idef,$@) \
	    $(patsubst insns/%.inc,$(SI_OTSPEC_DIR)/%.ot,$@) > $@ || (rm $@; exit 1)
else
$(INSN_SUPERINSNS):insns/%.inc: $(EJSVM_DIR)/insns-def/* $(SUPERINSNSPEC) $(SI_OTSPEC_DIR)/%.ot $(VMGEN)
	mkdir -p insns
	$(INSNGEN) $(INSNGEN_FLAGS) \
	    -Xgen:label_prefix $(patsubst insns/%.inc,%,$@) \
	    -Xcmp:tree_layer p0:p1:p2:h0:h1:h2 $(DATATYPES) \
	    $(EJSVM_DIR)/insns-def/$(call orig_insn,$@).idef \
	    $(patsubst insns/%.inc,$(SI_OTSPEC_DIR)/%.ot,$@) > $@ || (rm $@; exit 1)
endif
endif

cell-header.h: $(EJSVM_DIR)/cell-header.def
	$(RUBY) $< $(OPT_DEBUG) $(OPT_BASEBIT) $(OPT_GC_ALGORITHM) > $@

instructions.h: instructions-opcode.h instructions-table.h

%.c:: $(EJSVM_DIR)/%.c
	cp $< $@

%.h:: $(EJSVM_DIR)/%.h
	cp $< $@

codeloader.o: specfile-fingerprint.h

vmloop.o: vmloop.c vmloop-cases.inc $(INSN_FILES) $(HFILES)
	$(CC) $(CCOPT) -c $(CFLAGS) -o $@ $<

%.o: %.c $(HFILES)
	$(CC) $(CCOPT) -c $(CFLAGS) -o $@ $<

#### vmgen
$(VMGEN):
	(cd $(VMGEN_DIR); ant)

#### ejsc
$(EJSC): $(VMGEN) ejsvm.spec
	(cd $(EJSC_DIR); ant -Dspecfile=$(PWD)/ejsvm.spec)

#### ejsi
ifeq ($(OPT_BASEBIT),32)
$(EJSI):
	make -C $(EJSI_DIR) ejsi-32
else
$(EJSI):
	make -C $(EJSI_DIR) ejsi-64
endif

#### check

CHECKFILES   = $(patsubst %.c,$(CHECKFILES_DIR)/%.c,$(CFILES))
CHECKRESULTS = $(patsubst %.c,$(CHECKFILES_DIR)/%.c.checkresult,$(CFILES))
CHECKTARGETS = $(patsubst %.c,%.c.check,$(CFILES))

types-generated.h: $(DATATYPES) $(VMGEN)
	$(TYPESGEN) $< > $@ || (rm $@; exit 1)

$(CHECKFILES):$(CHECKFILES_DIR)/%.c: %.c $(HFILES)
	mkdir -p $(CHECKFILES_DIR)
	$(CPP) $(CFLAGS) -DCOCCINELLE_CHECK=1 $< > $@ || (rm $@; exit 1)

$(CHECKFILES_DIR)/vmloop.c: vmloop-cases.inc $(INSN_FILES)

.PHONY: %.check
$(CHECKTARGETS):%.c.check: $(CHECKFILES_DIR)/%.c
	$(COCCINELLE) --sp-file $(GCCHECK_PATTERN) $<

$(CHECKRESULTS):$(CHECKFILES_DIR)/%.c.checkresult: $(CHECKFILES_DIR)/%.c
	$(COCCINELLE) --sp-file $(GCCHECK_PATTERN) $< > $@ || (rm $@; exit 1)

check: $(CHECKRESULTS)
	cat $^

#### clean

clean:
	rm -f *.o $(GENERATED_HFILES) vmloop-cases.inc *.c *.h
	rm -rf insns
	rm -f *.checkresult
	rm -rf $(CHECKFILES_DIR)
	rm -rf si

cleanest:
	rm -f *.o $(GENERATED_HFILES) vmloop-cases.inc *.c *.h
	rm -rf insns
	rm -f *.checkresult
	rm -rf $(CHECKFILES_DIR)
	rm -rf si
	(cd $(VMGEN_DIR); ant clean)
	rm -f $(VMGEN)
	(cd $(EJSC_DIR); ant clean)
	rm -f $(EJSC)
	make -C $(EJSI_DIR) clean

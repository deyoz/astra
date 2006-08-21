.PHONY: all depend tag clean \
	SysTarg2 rebuild  depend2 lock unlock
all:    SysTarg2
	$(MAKE) astra
ifndef SRCHOME
SRCHOME=.
endif
include include.mk

export EDILIB_PATH

SysTarg2:
	$(MAKE) -f systarg.mk

TCL_LIBS=$(TCL_SYS_LIB) $(TCL_MON_PATH)/tclmonlib.a
SERVERLIB=$(SERVERLIB_PATH)/serverlib.a
JXTLIB=$(JXTLIB_PATH)/jxtlib.a
OTHERLIBS= $(LIBXML2LIB) $(SERVERLIB) $(TCL_LIBS) $(JXTLIB)\
		   $(LDZLIB) $(LOCALLIBS) $(EDILIB) \
		   $(LIBAIO)
LIB_STATIC_INIT_NAME=astra_static_init
export LIB_STATIC_INIT_NAME

rebuild: clean all
tag:
	ctags "--langmap=c:+.pc.pcc" * 
	etags "--langmap=c:+.pc.pcc" * 

depend: depend2 

clean: 
	#-./delc	
	echo > deps
	-killall edit_deps2
	-rm -rf depsdir edit_deps_lock
	-rm -f *.o tlg/*.o *.h1  *.lis  *.d astra


#
# list of modules is now in file 'modules'
#

include modules


LINKMOD=$(MODULES) $(AIRIMPLIB) $(XP_TESTINGLIBS)
.PHONY: subdirs $(SUBDIRS) obr
subdirs: $(SUBDIRS)
$(SUBDIRS): 
	$(MAKE) -C $@


ast: $(MODULES) subdirs
	$(MAKE) astra

astra: $(LINKMOD)
	$(CXX) $(LDFLAGS) -o $@ $(LINKMOD)  \
	$(FINAL_LIBS)  $(OTHERLIBS)


sinclude deps

include $(SRCHOME)/commonvars1.mk
include $(SRCHOME)/commonopts.mk
include $(SRCHOME)/commonvars2.mk
export SRCHOME
lock:
    
	@ if [ x$$MAKE_LOCKED != xyes ] ; then \
    if mkdir $$SRCHOME/sirenamake ; then \
	echo $$MY_TTY >/$$SRCHOME/sirenamake/tty ; \
	elif [ x$$MY_TTY != x$$(cat /$$SRCHOME/sirenamake/tty) ]  ; \
	then echo "another make is running " ; exit 1 ; fi ; fi

unlock:
	@ if [ x$$MAKE_LOCKED != xyes ] ; then \
	if [ -d /$$SRCHOME/sirenamake ] ; then  \
	if [ x$$MY_TTY != x$$(cat /$$SRCHOME/sirenamake/tty) ]  ; then \
	echo "Do you really want to unlock make that is running?" ; \
	echo "type yes and ENTER if you do" ; \
	read aaa ;\
	if [ x$$aaa != xyes	] ; then exit 1 ; fi ; fi ; fi ;\
	rm -rf /$$SRCHOME/sirenamake ; fi
	


CFLAGS=-Wall $(COMPFLAGS)
CPPFLAGS=-Wall $(COMPFLAGS) $(CPPCOMPFLAGS)

include $(SRCHOME)/depsrules
include $(SRCHOME)/make_rules


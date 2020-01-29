# set the include path found by configure
INCLUDES= $(all_includes)

lib_LTLIBRARIES=libcoretypes.la
include $(top_srcdir)/src/single.mk

noinst_HEADERS = coretypes.h packer.h flight.h rbdorder.h pnr.h seats.h ticket.h \
				 predpoint.h segtime.h checkin.h pos.h pnr_conn.h rfisc.h tlg.h message_binders.h
libcoretypes_la_SOURCES = packer.cc flight.cc rbdorder.cc pnr.cc seats.cc route.cc \
						  predpoint.cc segtime.cc ticket.cc pos.cc pnr_conn.cc rfisc.cc \
						  tlg.cc

AM_CPPFLAGS = @CFLAGS@ @SERVERLIB_CXXFLAGS@ @LIBNSI_CXXFLAGS@ @BOOST_CPPFLAGS@ @ORACLE_CXXFLAGS@

if XP_TESTING
bin_PROGRAMS = testapp

testapp_SOURCES = testapp.cc
testapp_LDFLAGS = @SERVERLIB_LIBS@ @CHECK_LIBS@ @BOOST_LDFLAGS@ @ORACLE_LDFLAGS@ @LIBNSI_LIBS@
testapp_LDADD = libcoretypes.la @SERVERLIB_LIBS@ @CHECK_LIBS@ @ORACLE_LIBS@ @LIBNSI_LIBS@

xp-tests: testapp
	rm -f *.log; \
	if [ -n "$(XP_LIST)" ]; then \
		LC_ALL=C XP_TESTING=1 XP_LIST="$(XP_LIST)" ./testapp ./app.tcl -nosir -run_tests | tee xp-tests.log 2>&1; \
	else \
		LC_ALL=C XP_TESTING=1 XP_LIST=coretypes ./testapp ./app.tcl -nosir -run_tests | tee xp-tests.log 2>&1; \
	fi;

endif

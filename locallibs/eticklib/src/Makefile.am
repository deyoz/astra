#bin_PROGRAMS = etick
#etick_SOURCES = etick.cpp

# set the include path found by configure
INCLUDES= $(all_includes) -I$(top_srcdir)/include

lib_LTLIBRARIES=libetick.la
include $(top_srcdir)/src/single.mk

libetick_la_SOURCES = edi_cast.cpp err_msg.cpp et_utils.cpp \
	etick_localization.cpp exceptions.cpp tick_data.cpp ticket.cpp tests/check_tick_data.cpp \
	emd_data.cpp tick_doctype.cpp codeshare_type.cpp
	
AM_CPPFLAGS = @SERVERLIB_CXXFLAGS@ @EDILIB_CXXFLAGS@ @BOOST_CPPFLAGS@ @EDILIB_CXXFLAGS@ @ORACLE_CXXFLAGS@ @TCLLIB_CXXFLAGS@

if XP_TESTING
bin_PROGRAMS = 
#testapp

testapp_SOURCES = tests/testapp.cpp
testapp_LDFLAGS = @ORACLE_LDFLAGS@ @BOOST_LDFLAGS@
testapp_LDADD = libetick.la \
	@SERVERLIB_LIBS@ \
	@EDILIB_LIBS@ \
	@CHECK_LIBS@ \
	@ORACLE_LIBS@

xp-tests: testapp
	@if [ ! -d ./Cores ]; then \
		mkdir ./Cores; \
	fi; \
	rm -f sirena_run_time.txt; \
	rm -rf Cores/*; \
	rm -f *.log; \
	if [ -n "$(XP_LIST)" ]; then \
		XP_TESTING=1 XP_LIST="$(XP_LIST)" LC_ALL=C ./testapp ./app.tcl -nosir -run_tests | tee xp-tests.log 2>&1; \
	else \
		XP_TESTING=1 LC_ALL=C ./testapp ./app.tcl -nosir -run_tests | tee xp-tests.log 2>&1; \
	fi; \
	grep "100%: Checks" xp-tests.log > /dev/null 2>&1
endif




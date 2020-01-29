# set the include path found by configure
INCLUDES= $(all_includes)

lib_LTLIBRARIES=libtlg.la
include $(top_srcdir)/src/single.mk

noinst_HEADERS = tlgnum.h types.h telegrams.h tlg_func.h net_func.h \
				 airsrv.h sender.h express.h express_sender.h \
				 tlgsplit.h hth.h external_tests.h tlg_outbox.h air_q_callbacks.h \
				 tlg_loader.h air_q_dbora_callbacks.h air_q_dbpg_callbacks.h
libtlg_la_SOURCES = telegrams.cc tlg_func.cc net_func.cc airsrv.cc \
					tlgjoin.cc sender.cc express.cc express_sender.cc \
					tlgsplit.cc hth.cc external_tests.cc tlg_outbox.cc air_q_callbacks.cc \
					tlg_loader.cc air_q_dbora_callbacks.cc air_q_dbpg_callbacks.cc

AM_CPPFLAGS = @CFLAGS@ @SERVERLIB_CXXFLAGS@ @EDILIB_CXXFLAGS@ @BOOST_CPPFLAGS@ @ORACLE_CXXFLAGS@

if XP_TESTING
bin_PROGRAMS = testapp

testapp_SOURCES = testapp.cc
testapp_LDFLAGS = @ORACLE_LDFLAGS@ @BOOST_LDFLAGS@ @BOOST_REGEX_LIB@
testapp_LDADD = libtlg.la \
	@SERVERLIB_LIBS@ \
	@EDILIB_LIBS@ \
	@CHECK_LIBS@ \
	@ORACLE_LIBS@ \
	@BOOST_REGEX_LIB@

xp-tests: 
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

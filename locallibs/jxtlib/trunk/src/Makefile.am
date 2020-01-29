lib_LTLIBRARIES=libjxt.la
include $(top_srcdir)/src/single.mk
GOGOLDFLAGS = @ORACLE_LDFLAGS@ @XML_LIBS@ @XSLT_LIBS@ @TCLLIB_LDFLAGS@
if XP_TESTING
AM_LDFLAGS = @SERVERLIB_LIBS@ @CHECK_LIBS@ @ORACLE_LIBS@ @XML_LIBS@ $(GOGOLDFLAGS)
else
AM_LDFLAGS = $(GOGOLDFLAGS)
endif
AM_CPPFLAGS = @SERVERLIB_CXXFLAGS@ @ORACLE_CXXFLAGS@ @BOOST_CPPFLAGS@ @XML_CFLAGS@ @XSLT_CFLAGS@ @TCLLIB_CXXFLAGS@

libjxt_la_SOURCES = jxt_cont.cc jxt_handle.cc JxtInterface.cc jxt_stuff.cc jxt_xml_cont.cc \
        xml_msg.cc jxtlib.cc jxt_sys_reqs.cc utf2cp866.cc \
        cont_tools.cc jxt_cont_impl.cc jxt_tools.cc gettext.cc \
        JxtGlobalInterface.cc jxt_bgnd.cc zip.cc Accessible.cc AccessException.cc \
        xslt_tools.cc xml_utils.cc fill_interface.cc jxtlib_dbora_callbacks.cc \
        jxtlib_dbpg_callbacks.cc jxtlib_db_callbacks.cc

if XP_TESTING
noinst_PROGRAMS = testapp

testapp_SOURCES = main.cc
testapp_LDADD = libjxt.la

tests: testapp
	        XP_TESTING=1 LC_ALL=C ./testapp ./app.tcl -nosir -run_tests

endif


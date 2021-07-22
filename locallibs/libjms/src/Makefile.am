AM_CPPFLAGS = $(CFLAGS) @BOOST_CPPFLAGS@ $(ORACLE_CXXFLAGS) -I$(top_srcdir)/src
AM_LDFLAGS = @ORACLE_LDFLAGS@ @ORACLE_LIBS@
if HAVE_SSL
AM_CPPFLAGS += @SSL_CPPFLAGS@
AM_LDFLAGS += @SSL_LIBS@
else
endif

if HAVE_SERVERLIB
AM_CPPFLAGS += @SERVERLIB_CXXFLAGS@
AM_LDFLAGS += @SERVERLIB_LIBS@
else
endif

lib_LTLIBRARIES = libjms.la
libjms_la_SOURCES = aq.cpp aq.hpp jms.cpp jms_log.hpp


if BUILD_AGAINST_PYTHON
lib_LTLIBRARIES += jmspy.la
jmspy_la_SOURCES = jmspy.cpp
jmspy_la_LDFLAGS = -module
jmspy_la_CPPFLAGS = @BOOST_CPPFLAGS@ @PYTHON_INCLUDE_DIR@
jmspy_la_LIBADD = @BOOST_LDFLAGS@ @BOOST_PYTHON_LIB@ @PYTHON_LIB@ libjms.la
else

endif

#nobase_include_HEADERS = jms.hpp text_message.hpp
headersdir = $(includedir)/jms
headers_DATA = jms.hpp text_message.hpp errors.hpp

if HAVE_ACTIVEMQ
AM_CPPFLAGS += @ACTIVEMQ_CFLAGS@
AM_LDFLAGS += @ACTIVEMQ_LIBS@
libjms_la_SOURCES += amq.cpp amq.hpp
else
endif

if HAVE_AMQP_CPP
AM_CPPFLAGS += @AMQP_CPP_CFLAGS@
AM_LDFLAGS += @AMQP_CPP_LIBS@ @BOOST_SYSTEM_LIB@
libjms_la_SOURCES += amqp.cpp amqp.hpp asiohandler.cpp asiohandler.hpp
else
endif



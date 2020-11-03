AM_CPPFLAGS = @BOOST_CPPFLAGS@ -I$(top_srcdir)/src $(CFLAGS)

if XP_TESTING
AM_LDFLAGS = @BOOST_LDFLAGS@  @PTHREAD_CFLAGS@ @PTHREAD_LIBS@ @BOOST_SYSTEM_LIB@ @BOOST_PROGRAM_OPTIONS_LIB@

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

noinst_PROGRAMS =

if HAVE_ACTIVEMQ
AM_CPPFLAGS += @ACTIVEMQ_CFLAGS@
AM_LDFLAGS += @ACTIVEMQ_LIBS@
else
endif

if HAVE_AMQP_CPP
AM_CPPFLAGS += @AMQP_CPP_CFLAGS@
AM_LDFLAGS += @AMQP_CPP_LIBS@
noinst_PROGRAMS += amqp_addr_parser
amqp_addr_parser_SOURCES = amqp_addr_parser.cpp
amqp_addr_parser_LDADD = $(top_srcdir)/src/libjms.la
else
endif

if HAVE_ORACLE_AQ
AM_CPPFLAGS += $(ORACLE_CXXFLAGS)
AM_LDFLAGS += $(ORACLE_LDFLAGS) $(ORACLE_LIBS)
noinst_PROGRAMS += mq_tests aq_addr_parser
mq_tests_SOURCES = mq_tests.cpp
mq_tests_LDADD = $(top_srcdir)/src/libjms.la
aq_addr_parser_SOURCES = aq_addr_parser.cpp
aq_addr_parser_LDADD = $(top_srcdir)/src/libjms.la
else
endif

else
AM_LDFLAGS = @BOOST_LDFLAGS@ $(LT_LDFLAGS)
endif
#mq_test_input_LDADD = $(top_srcdir)/src/libprog_args.la

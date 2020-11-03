AM_CPPFLAGS = @BOOST_CPPFLAGS@ -I$(top_srcdir)/src
#PTHREAD_CFLAGS should be below
AM_LDFLAGS =  @BOOST_LDFLAGS@ @PTHREAD_CFLAGS@ @PTHREAD_LIBS@ @ORACLE_LDFLAGS@ @ORACLE_LIBS@ @BOOST_SYSTEM_LIB@

if BUILD_EXAMPLES

if HAVE_SSL
AM_CPPFLAGS += @SSL_CPPFLAGS@
AM_LDFLAGS += @SSL_LIBS@
else
endif

if HAVE_ACTIVEMQ
AM_CPPFLAGS += @ACTIVEMQ_CFLAGS@
AM_LDFLAGS += @ACTIVEMQ_LIBS@
else
endif

if HAVE_AMQP_CPP
AM_CPPFLAGS += @AMQP_CPP_CFLAGS@
AM_LDFLAGS += @AMQP_CPP_LIBS@ @BOOST_SYSTEM_LIB@
else
endif


noinst_PROGRAMS = deq_message enq_message perf_chk dc_chk file2queue resender resender_fork amqexample dequeue mt_enq_deq

deq_message_SOURCES = deq_message.cpp
deq_message_LDADD = $(top_srcdir)/src/libjms.la

enq_message_SOURCES = enq_message.cpp
enq_message_LDADD = $(top_srcdir)/src/libjms.la

perf_chk_SOURCES = perf_chk.cpp
perf_chk_LDADD = $(top_srcdir)/src/libjms.la $(BOOST_LDFLAGS) @BOOST_PROGRAM_OPTIONS_LIB@

mt_enq_deq_SOURCES = mt_enq_deq.cpp
mt_enq_deq_LDADD = $(top_srcdir)/src/libjms.la $(BOOST_LDFLAGS) @BOOST_PROGRAM_OPTIONS_LIB@

dc_chk_SOURCES = dc_chk.cpp
dc_chk_LDADD = $(top_srcdir)/src/libjms.la $(BOOST_LDFLAGS) @BOOST_PROGRAM_OPTIONS_LIB@

file2queue_SOURCES = file2queue.cpp
file2queue_LDADD = $(top_srcdir)/src/libjms.la $(BOOST_LDFLAGS) @BOOST_PROGRAM_OPTIONS_LIB@

resender_SOURCES = resender.cpp
resender_LDADD = $(top_srcdir)/src/libjms.la $(BOOST_LDFLAGS) @BOOST_PROGRAM_OPTIONS_LIB@

resender_fork_SOURCES = resender_fork.cpp
resender_fork_LDADD = $(top_srcdir)/src/libjms.la $(BOOST_LDFLAGS) @BOOST_PROGRAM_OPTIONS_LIB@

dequeue_SOURCES = dequeue.cpp
dequeue_LDADD = $(top_srcdir)/src/libjms.la $(BOOST_LDFLAGS) @BOOST_PROGRAM_OPTIONS_LIB@

amqexample_SOURCES = amqexample.cpp
amqexample_LDADD = $(top_srcdir)/src/libjms.la -lpthread

if HAVE_ACTIVEMQ
AM_CPPFLAGS += @ACTIVEMQ_CFLAGS@
amqexample_LDADD += @ACTIVEMQ_LIBS@
else
endif

endif

lib_LTLIBRARIES = libserver.la

include $(top_srcdir)/single.mk

GOGOLDFLAGS = @PTHREAD_CFLAGS@ @PTHREAD_LIBS@ @ORACLE_LDFLAGS@ @BOOST_LDFLAGS@ @TCLLIB_LDFLAGS@ \
			  @MEMCACHED_LIBS@ @XSLT_LIBS@ @BOOST_DATE_TIME_LIB@ @MESPRO_LDFLAGS@ \
			  @BOOST_SERIALIZATION_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_REGEX_LIB@ @SSL_LIBS@ \
			  @AMQP_CPP_LIBS@ @LIBPQ_LIBS@

if XP_TESTING
AM_LDFLAGS = @CHECK_LIBS@ $(GOGOLDFLAGS)
else
AM_LDFLAGS = $(GOGOLDFLAGS)
endif

AM_CPPFLAGS = @PTHREAD_CFLAGS@ @ORACLE_CXXFLAGS@ @BOOST_CPPFLAGS@ @CHECK_CFLAGS@ @TCLLIB_CXXFLAGS@ \
			  @MEMCACHED_CFLAGS@ @XML_CFLAGS@ @XSLT_CFLAGS@ @MESPRO_CFLAGS@ \
			  @AMQP_CPP_CFLAGS@ @LIBPQ_CFLAGS@ -I$(top_srcdir)/include

libserver_la_SOURCES = addr2line.cc cache.cc checkunit.cc crypting.cc daemon_event.cc \
					   daemon_kicker.cc daemon_task.cc dates.cc dbms_output.cc device.cc EdiHelpManager.cc \
					   EdiHelpDbCallbacks.cc EdiHelpDbOraCallbacks.cc EdiHelpDbPgCallbacks.cc ehelpsig.cc \
					   exception.cc guarantee_write.cc helpcpp.cc internal_msgid.cc int_parameters.cc \
					   local_tcp_event.cc logger.cc log_queue.cc logwriter.cc lwriter.cc md5.cc memcached_api.cc \
					   memmap.cc mes.cc mespro_crypt.cc monitor.cc monitor_ctl.cc new_daemon.cc base64.cc \
					   object.cc oci8.cc ocilocal.cc ourtime.cc posthooks.cc potok.cc profiler.cc crc32.cc \
					   pg_cursctl.cc dbcpp_cursctl.cc dbcpp_session.cc commit_rollback.cc \
					   query_runner.cc queue.cc queue_proc.cc savepoint.cc shmserv_hash.cc signal.cc simplelog.cc \
					   sirenaproc.cc sirena_queue.cc slogger.cc string_cast.cc str_utils.cc tclmon.cc tcl_utils.cc \
					   testmode.cc text_codec.cc timer.cc timestat.cc TlgLogger.cc xp_test_utils.cc tscript_vm.cc \
					   func_placeholders.cc text_diff.cc levenshtein.cc tscript_diff.cc tscript.cc tscript_cmp.cc \
					   period.cc period_joiner.cc freq.cc localtime.cc json_spirit_reader.cc json_spirit_writer.cc json_packer.cc json_pack_types.cc \
					   log_manager.cc blev.cc zlib_employment.cc proc_c.cc http_server.cc http_parser.cc \
					   fcgi.cc ts_funcs.cc oci8cursor.cc smtp.cc dispatcher.cc dispatcher_frontend.cc rip.cc \
					   smtp_dbora_callbacks.cc smtp_dbpg_callbacks.cc \
					   message.cc bgnd.cc xml_cpp.cc xml_tools.cc xml_stuff.cc xmllibcpp.cc xml_context.cc encstring.cc \
					   http_logs_callbacks.cc \
					   httpsrv.cc test_exp_map.cc simple_task.cc cc_censor.cc sha.cc logcout.cc msglog.cc enumset.cc \
					   logrange.cc algo.cc deffered_exec.cc balancer.cc base32.cc amqp_connection_handler.cc test_httpsrv.cc \
					   ntlm_msg.cc ntlm_service.cc ntlm_server.cc

if XP_TESTING

testapp = testapp
ntlmapp = ntlmapp

endif

noinst_PROGRAMS = dispatcher supervisor $(testapp) $(ntlmapp)

dispatcher_SOURCES = main.cc
dispatcher_LDADD = libserver.la @CHECK_LIBS@ @ORACLE_LIBS@

supervisor_SOURCES = supervisor_main.cc startparam.cc supervisor.cc
supervisor_LDADD = libserver.la @CHECK_LIBS@ @ORACLE_LIBS@
#connect.c
if XP_TESTING

testapp_SOURCES = main.cc
testapp_LDADD = libserver.la @ORACLE_LIBS@ @CHECK_LIBS@

ntlmapp_SOURCES = ntlm_main.cc
ntlmapp_LDADD = libserver.la @ORACLE_LIBS@ @CHECK_LIBS@

xp-tests: testapp ntlmapp
	@if [ ! -d ./Cores ]; then \
		mkdir ./Cores; \
	fi; \
	if [ ! -d ./.Sockets ]; then \
		mkdir ./.Sockets; \
	fi; \
	rm -f sirena_run_time.txt; \
	rm -rf Cores/*; \
	rm -rf .Sockets/*;\
	rm -f *.log; \
	if [ -n "$(XP_LIST)" ]; then \
		XP_TESTING=1 XP_LIST="$(XP_LIST)" LC_ALL=C ./testapp ./handler.tcl -nosir -run_tests | tee xp-tests.log 2>&1; \
	else \
		XP_TESTING=1 LC_ALL=C ./testapp ./handler.tcl -nosir -run_tests | tee xp-tests.log 2>&1; \
	fi; \
	grep "100%: Checks" xp-tests.log > /dev/null 2>&1

endif

#include $(TCLMON_PATH)/tclconfig

# set the include path found by configure
INCLUDES = $(all_includes)
export ASTRA_SRC=$(top_srcdir)

bin_PROGRAMS = astra

BUILT_SOURCES = SysTarg
export EDILIB_PATH
SysTarg:
	$(srcdir)/autotests.sh $(srcdir)/ts
	$(MAKE) -f $(srcdir)/systarg.mk

astra_LDFLAGS = @ORACLE_LDFLAGS@ @SHARED_LDFLAGS@

install-edimessages:
	./nosir.tcl -ediinsert -i ${EDILIB_PATH}/edi_messages


clean-xp-tests:
	@if [ ! -d ./Cores ]; then \
		mkdir ./Cores; \
	fi; \
	rm -rf Cores/*; \
	rm -f *.log sirena_run_time.txt;


xp-tests: clean-xp-tests
	@if [ -n "$(XP_LIST)" ]; then \
		XP_TESTING=1 XP_LIST=$(XP_LIST) ./nosir.tcl -run_tests | tee xp-tests.log 2>&1; \
	else \
		XP_TESTING=1 ./nosir.tcl -run_tests | tee xp-tests.log 2>&1; \
	fi; \
	grep "100%: Checks" xp-tests.log > /dev/null 2>&1
	

AM_CXXFLAGS = $(TCLINCLUDE) @PION_CXXFLAGS@
astra_SOURCES = \
  adm.cc \
  access.cc \
  alarms.cc \
  aodb.cc \
  apis.cc \
  apis_edi_file.cc \
  apis_tools.cc \
  apis_utils.cc \
  app_interaction.cc \
  arx_daily.cc \
  astra_callbacks.cc \
  astra_consts.cc \
  astra_context.cc \
  astra_elems.cc \
  astra_locale.cc \
  astra_locale_adv.cc \
  astra_main.cc \
  astra_misc.cc \
  astra_service.cc \
  astra_tick_read_edi.cpp \
  astra_tick_view_xml.cc \
  astra_emd_view_xml.cc \
  astra_utils.cc \
  autotests.cc \
  baggage.cc \
  base_tables.cc \
  basel_aero.cc \
  brd.cc \
  cache.cc \
  cent.cc \
  checkin.cc \
  checkin_utils.cc \
  cobra.cc \
  comp_layers.cc \
  convert.cc \
  crypt.cc \
  dev_consts.cc \
  dev_tuning.cc \
  dev_utils.cc \
  develop_dbf.cc \
  docs.cc \
  docs2.cc \
  edi_utils.cc \
  emdoc.cc \
  empty_proc.cc \
  etick_change_status.cpp \
  etick.cc \
  events.cc \
  external_spp_synch.cc \
  flt_binding.cc \
  file_queue.cc \
  hist.cc \
  http_io.cpp \
  http_main.cc \
  httpClient.cc \
  images.cc \
  load_fr.cc \
  locale.cc \
  maindcs.cc \
  msg_handler.cc \
  msg_queue.cc \
  obrnosir.cc \
  passenger.cc \
  payment.cc \
  pers_weights.cc \
  points.cc \
  prepreg.cc \
  print.cc \
  prn_tag_store.cc \
  remarks.cc \  
  request_dup.cc \
  rozysk.cc \
  salonform.cc \
  salonform2.cc \
  salons.cc \
  salons2.cc \
  season.cc \
  seats_utils.cc \
  seats.cc \
  sofi.cc \
  sopp.cc \
  spp_cek.cc \
  stages.cc \
  stat.cc \
  qrys.cpp \
  telegram.cc \
  telegram2.cc \
  timer.cpp \
  transfer.cc \
  trip_tasks.cc \
  tripinfo.cc \
  typeb_utils.cc \
  web_main.cc \
  web_search.cc \
  xml_unit.cc \
  xp_testing.cc \
  tscript_callbacks.cc \
  ticket_types.cpp \
  astra_emd.cpp \
  EdiTimeOutsDaemon.cc \
  iatci.cc \
  iatci_types.cc \
  iatci_api.cc \
  TypeBHelpMng.cpp \
  astra_msg.cc \
  Depeches.cpp \
  AsyncNet.cpp


astra_LDADD = $(all_libraries) \
	tlg/libtlg.a \
  tests/libtests.a \
	@ETICKLIB_LIBS@ \
	@ORACLE_LIBS@ @EDILIB_LIBS@ \
	@JXTLIB_LIBS@ @SHARED_LIBS@ \
	@LIBTLG_LIBS@ @SERVERLIB_LIBS@ \
	$(XML_LIBS) @CHECK_LIBS@\
	@BOOST_SYSTEM_LIB@\
	@BOOST_FILESYSTEM_LIB@\
	@BOOST_REGEX_LIB@\
        @BOOST_THREAD_LIB@\
        @PION_LDFLAGS@


#	-L/usr/lib -lboost_filesystem
#	-L/home/beta/crypt/locallibs/mespro -lmesprox

SUBDIRS = tlg tests

EXTRA_DIST = systarg.mk run astra.tcl PS Restart kills \
	MAKE local_after_example.tcl local_before_example.tcl

DISTCLEANFILES = local_after.tcl local_before.tcl
noinst_HEADERS = \
  adm.h \
  access.h \
  alarms.h \
  aodb.h \
  apis.h \
  apis_edi_file.h \
  apis_tools.h \
  app_interaction.h \
  arx_daily.h \
  astra_callbacks.h \
  astra_consts.h \
  astra_context.h \
  astra_elems.h \
  astra_locale.h \
  astra_locale_adv.h \
  astra_main.h \
  astra_misc.h \
  astra_service.h \
  astra_tick_read_edi.h \
  astra_tick_reader.h \
  astra_tick_view_xml.h \
  astra_emd_view_xml.h \
  astra_tick_view.h \
  astra_ticket.h \
  astra_utils.h \
  apis_utils.h \
  baggage.h \
  base_tables.h \
  basel_aero.h \
  brd.h \
  cache.h \
  cent.h \
  checkin.h \
  checkin_utils.h \
  cobra.h \
  comp_layers.h \
  convert.h \
  crypt.h \
  dev_consts.h \
  dev_tuning.h \
  dev_utils.h \
  develop_dbf.h \
  docs.h \  
  edi_utils.h \
  emdoc.h \
  empty_proc.h \
  etick_change_status.h \
  etick.h \
  events.h \
  external_spp_synch.h \
  flt_binding.h \
  file_queue.h \
  hist.h \
  hist_interface.h \
  http_io.h \
  http_main.h \
  httpClient.h \
  images.h \
  load_fr.h \
  maindcs.h \
  msg_queue.h \
  obrnosir.h \
  passenger.h \
  payment.h \
  pers_weights.h \
  points.h \
  prepreg.h \
  print.h \
  prn_tag_store.h \
  remarks.h \
  request_dup.h \
  rozysk.h \
  salonform.h \
  salonform2.h \
  salons.h \
  salons2.h \
  season.h \
  seats_utils.h \
  seats.h \
  sofi.h \
  sopp.h \
  spp_cek.h \
  stages.h \
  stat.h \
  qrys.h \
  timer.h \
  transfer.h \
  trip_tasks.h \
  tripinfo.h \
  typeb_utils.h \
  web_main.h \
  web_search.h \
  xml_unit.h \
  ticket_types.h \
  astra_dates.h \
  astra_dates_oci.h \
  astra_emd.h \
  iatci_types.h \
  iatci_serialization.h \
  iatci_api.h \
  iatci.h \
  TypeBHelpMng.h \
  astra_msg.h \
  Depeches.hpp \
  AsyncNet.hpp
  

#include $(TCLMON_PATH)/tclconfig

# set the include path found by configure
AM_CPPFLAGS = $(all_includes)
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


AM_CXXFLAGS = $(TCLINCLUDE) @PION_CFLAGS@ 
astra_SOURCES = \
  adm.cc \
  access.cc \
  alarms.cc \
  aodb.cc \
  apis.cc \
  apis_creator.cc \
  apis_edi_file.cc \
  apis_tools.cc \
  apis_utils.cc \
  apps_interaction.cc \
  arx_daily.cc \
  astra_callbacks.cc \
  astra_consts.cc \
  astra_context.cc \
  astra_elem_utils.cc \
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
  baggage_base.cc \
  baggage_calc.cc \
  baggage_tags.cc \
  baggage_wt.cc \
  base_tables.cc \
  basel_aero.cc \
  bi_rules.cpp \
  brd.cc \
  cache.cc \
  cent.cc \
  checkin.cc \
  checkin_utils.cc \
  code_convert.cc \
  comp_layers.cc \
  convert.cc \
  counters.cc \
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
  ffp_sirena.cc \
  flt_binding.cc \
  file_queue.cc \
  hist.cc \
  http_io.cpp \
  http_main.cc \
  httpClient.cc \
  images.cc \
  load_fr.cc \
  locale.cc \
  libra.cc \
  maindcs.cc \
  meridian.cc \
  obrnosir.cc \
  passenger.cc \
  payment.cc \
  payment_base.cc \
  pers_weights.cc \
  points.cc \
  postpone_trip_task.cc \
  prepreg.cc \
  print.cc \
  prn_tag_store.cc \
  remarks.cc \
  request_dup.cc \
  rfisc.cc \
  rfisc_sirena.cc \
  rozysk.cc \
  salonform.cc \
  salonform2.cc \
  salons.cc \
  salons2.cc \
  season.cc \
  seats_utils.cc \
  seats.cc \
  sirena_exchange.cc \
  sofi.cc \
  sopp.cc \
  spp_cek.cc \
  stages.cc \
  stat.cc \
  stat_doc_country.cc \
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
  iatci_types.cc \
  iatci_api.cc \
  iatci_help.cc \
  iatci_settings.cc \
  TypeBHelpMng.cpp \
  astra_msg.cc \
  img.cpp \
  astra_api.cc \
  astra_date_time.cpp \
  basetables.cc \
  collect.cc \
  collect_data.cc \
  SalonPaxList.cc \
  annul_bt.cc \
  html_pages.cc \
  brands.cpp \
  astra_pnr.cpp \
  recloc.cc \
  environ.cc \
  AirportControl.cpp \
  md5_sum.cc \
  bagmessage.cc \
  hotel_acmd.cc \
  kiosk_events.cc \
  self_ckin_log.cc \
  stat_bi.cc \
  stat_vo.cc \
  stat_ha.cc \
  stat_ad.cc \
  stat_unacc.cc \
  pax_events.cc \
  stat_utils.cc \
  stat_common.cc \
  db_pkg.cc \
  cr_lf.cc \
  bagmessage_daemon.cc

astra_LDADD = $(all_libraries) \
	tlg/libtlg.a \
        tests/libtests.a \
	@SHARED_LIBS@ @SHARED_DEPEND_LIBS@ \
	@LIBTLG_LIBS@ \
	@SERVERLIB_LIBS@ \
        @LIBTYPEB_LIBS@ \
	@ETICKLIB_LIBS@ @EDILIB_LIBS@ \
	@ORACLE_LIBS@ \
	@JXTLIB_LIBS@ \
	@LIBXML2_LIBS@ \
	@CHECK_LIBS@ \
	@PION_LIBS@ \
	@BOOST_IOSTREAMS_LIB@ \
	@BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ \
	@BOOST_REGEX_LIB@


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
  apis_creator.h \
  apis_edi_file.h \
  apis_tools.h \
  apps_interaction.h \
  arx_daily.h \
  astra_callbacks.h \
  astra_consts.h \
  astra_context.h \
  astra_elem_utils.h \
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
  baggage_base.h \
  baggage_calc.h \
  baggage_tags.h \
  baggage_wt.h \
  base_tables.h \
  basel_aero.h \
  bi_rules.h \
  brd.h \
  cache.h \
  cent.h \
  checkin.h \
  checkin_utils.h \
  code_convert.h \
  comp_layers.h \
  convert.h \
  counters.h \
  crypt.h \
  dev_consts.h \
  dev_tuning.h \
  dev_utils.h \
  develop_dbf.h \
  docs.h \
  edi_utils.h \
  emdoc.h \
  etick_change_status.h \
  etick.h \
  events.h \
  external_spp_synch.h \
  ffp_sirena.h \
  flt_binding.h \
  file_queue.h \
  hist.h \
  hist_interface.h \
  http_io.h \
  http_main.h \
  httpClient.h \
  images.h \
  load_fr.h \
  libra.h\
  maindcs.h \
  meridian.h \
  obrnosir.h \
  passenger.h \
  payment.h \
  payment_base.h \
  pers_weights.h \
  points.h \
  postpone_trip_task.h \
  prepreg.h \
  print.h \
  prn_tag_store.h \
  remarks.h \
  request_dup.h \
  rfisc.h \
  rfisc_sirena.h \
  rozysk.h \
  salonform.h \
  salonform2.h \
  salons.h \
  salons2.h \
  season.h \
  seats_utils.h \
  seats.h \
  sirena_exchange.h \
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
  iatci_help.h \
  iatci_settings.h \
  TypeBHelpMng.h \
  astra_msg.h \
  img.h \
  astra_iface.h \
  astra_api.h \
  astra_date_time.h \
  basetables.h \
  collect.h \
  collect_data.h \
  SalonPaxList.h \
  annul_bt.h \
  html_pages.h \
  brands.h \
  astra_pnr.h \
  AirportControl.h \
  md5_sum.h \
  hotel_acmd.h \
  kiosk_events.h \
  self_ckin_log.h \
  stat_bi.h \
  stat_vo.h \
  stat_ha.h \
  stat_ad.h \
  stat_unacc.h \
  pax_events.h \
  stat_utils.h \
  stat_common.h \
  db_pkg.h \
  cr_lf.h \
  report_common.h

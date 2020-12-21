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

astra_LDFLAGS = @ORACLE_LDFLAGS@ @SHARED_LDFLAGS@ @LIBTYPEB_LDFLAGS@

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


AM_CXXFLAGS = $(TCLINCLUDE) @SERVERLIB_CXXFLAGS@ @LIBTIMATIC_CXXFLAGS@
astra_SOURCES = \
  adm.cc \
  access.cc \
  alarms.cc \
  aodb.cc \
  apis_creator.cc \
  apis_edi_file.cc \
  apis_settings.cc \
  apis_tools.cc \
  apis_utils.cc \
  apps_interaction.cc \
  arx_daily.cc \
  arx_daily_pg.cpp \
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
  checkin_segments.cc \
  checkin_utils.cc \
  ckin_search.cc \
  code_convert.cc \
  comp_layers.cc \
  counters.cc \
  comp_props.cpp \
  crypt.cc \
  dcs_services.cc \
  dev_consts.cc \
  dev_tuning.cc \
  dev_utils.cc \
  develop_dbf.cc \
  edi_utils.cc \
  emdoc.cc \
  empty_proc.cc \
  etick_change_status.cpp \
  etick.cc \
  events.cc \
  external_spp_synch.cc \
  exch_checkin_result.cpp \
  ExchangeIface.cc \
  ffp_sirena.cc \
  flt_binding.cc \
  flt_settings.cc \
  flt_tasks_daemon.cc \
  file_queue.cc \
  hist.cc \
  http_io.cpp \
  http_main.cc \
  httpClient.cc \
  iapi_interaction.cc \
  images.cc \
  load_fr.cc \
  locale.cc \
  libra.cc \
  maindcs.cc \
  meridian.cc \
  mob_payment.cc \
  msg_queue.cc \
  MPSExchangeIface.cc \
  obrnosir.cc \
  passenger.cc \
  passenger_callbacks.cc \
  pax_calc_data.cc \
  pax_confirmations.cc \
  payment.cc \
  payment_base.cc \
  pers_weights.cc \
  points.cc \
  postpone_trip_task.cc \
  prepreg.cc \
  print.cc \
  prn_tag_store.cc \
  rbd.cc \
  remarks.cc \
  request_dup.cc \
  rfisc.cc \
  rfisc_calc.cc \
  rfisc_price.cc \
  rfisc_sirena.cc \
  rozysk.cc \
  salonform.cc \
  salonform2.cc \
  salons.cc \
  season.cc \
  seats_utils.cc \
  seats.cc \
  seat_descript.cc \
  seat_number.cc \
  seatPax.cc \
  service_eval.cc \
  sirena_exchange.cc \
  sofi.cc \
  sopp.cc \
  spp_cek.cc \
  stages.cc \
  stat_doc_country.cc \
  SWCExchangeIface.cc \
  qrys.cpp \
  telegram.cc \
  telegram2.cc \
  timer.cpp \
  transfer.cc \
  trip_tasks.cc \
  tripinfo.cc \
  typeb_utils.cc \
  web_craft.cc \
  web_exchange.cc \
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
  self_ckin_log.cc \
  pax_events.cc \
  db_pkg.cc \
  cr_lf.cc \
  franchise.cc \
  astra_ssim.cc \
  bagmessage_daemon.cc \
  get_cls_upgrade.cc \
  custom_alarms.cc \
  nosir_create_tlg.cc \
  prn_forms_layout.cc \
  cuws_main.cc \
  cuws_handlers.cc \
  timatic.cc \
  timatic_request.cc \
  timatic_response.cc \
  timatic_exchange.cc \
  zamar_dsm.cpp \
  pg_session.cc \
  base_callbacks.cpp \
  dbo.cpp \
  dbostructures.cpp \
  astra_calls.cc \
  wb_messages.cc \
  hooked_session.cc \
  PgOraConfig.cc


astra_LDADD = $(all_libraries) \
	tlg/libtlg.a \
	docs/libdocs.a \
	stat/libstat.a \
	kiosk/libkiosk.a \
	crafts/libcraft.a \
	tests/libtests.a \
	@LIBTIMATIC_LIBS@ \
	@SHARED_LIBS@ @SHARED_DEPEND_LIBS@ \
	@LIBTLG_LIBS@ \
	@SERVERLIB_LIBS@ \
	@LIBJMS_LIBS@ \
  @LIBSSIM_LIBS@ \
  @LIBCORETYPES_LIBS@ \
  @LIBNSI_LIBS@ \
        @LIBTYPEB_LIBS@ \
	@ETICKLIB_LIBS@ @EDILIB_LIBS@ \
	@ORACLE_LIBS@ @ORACLE_LDFLAGS@ \
	@JXTLIB_LIBS@ \
	@LIBXML2_LIBS@ \
	@CHECK_LIBS@ \
	@PION_LIBS@ \
	@BOOST_IOSTREAMS_LIB@ \
	@BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ \
	@BOOST_REGEX_LIB@ \
        @BOOST_THREAD_LIB@


#	-L/usr/lib -lboost_filesystem
#	-L/home/beta/crypt/locallibs/mespro -lmesprox

SUBDIRS = tlg tests docs stat kiosk crafts

EXTRA_DIST = systarg.mk run astra.tcl PS Restart kills \
	MAKE local_after_example.tcl local_before_example.tcl

DISTCLEANFILES = local_after.tcl local_before.tcl
noinst_HEADERS = \
  adm.h \
  access.h \
  alarms.h \
  aodb.h \
  apis_creator.h \
  apis_edi_file.h \
  apis_settings.h \
  apis_tools.h \
  apps_interaction.h \
  arx_daily.h \
  arx_daily_pg.h \
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
  checkin_segments.h \
  checkin_utils.h \
  ckin_search.h \
  code_convert.h \
  comp_layers.h \
  counters.h \
  comp_props.h \
  crypt.h \
  dcs_services.h \
  dev_consts.h \
  dev_tuning.h \
  dev_utils.h \
  develop_dbf.h \
  edi_utils.h \
  emdoc.h \
  etick_change_status.h \
  etick.h \
  events.h \
  external_spp_synch.h \
  exch_checkin_result.h \
  ExchangeIface.h \
  ffp_sirena.h \
  flt_binding.h \
  flt_settings.h \
  file_queue.h \
  hist.h \
  hist_interface.h \
  http_io.h \
  http_main.h \
  httpClient.h \
  iapi_interaction.h \
  images.h \
  load_fr.h \
  libra.h \
  maindcs.h \
  meridian.h \
  mob_payment.h \
  msg_queue.h \
  MPSExchangeIface.h \  
  obrnosir.h \
  passenger.h \
  passenger_callbacks.h \
  pax_calc_data.h \
  pax_confirmations.h \
  payment.h \
  payment_base.h \
  pers_weights.h \
  points.h \
  postpone_trip_task.h \
  prepreg.h \
  print.h \
  prn_tag_store.h \
  rbd.h \
  remarks.h \
  request_dup.h \
  rfisc.h \
  rfisc_calc.h \
  rfisc_price.h \
  rfisc_sirena.h \
  rozysk.h \
  salonform.h \
  salonform2.h \
  salons.h \
  season.h \
  seats_utils.h \
  seats.h \
  seat_descript.h \
  seat_number.h \
  seatPax.h \
  service_eval.h \
  sirena_exchange.h \
  sofi.h \
  sopp.h \
  spp_cek.h \
  stages.h \
  SWCExchangeIface.h \
  qrys.h \
  timer.h \
  transfer.h \
  trip_tasks.h \
  tripinfo.h \
  typeb_utils.h \
  web_craft.h \
  web_exchange.h \
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
  iatci.h \
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
  md5_sum.h \
  astra_pnr.h \
  recloc.h \
  environ.h \
  AirportControl.h \
  md5_sum.h \
  hotel_acmd.h \
  self_ckin_log.h \
  pax_events.h \
  db_pkg.h \
  cr_lf.h \
  franchise.h \
  report_common.h \
  astra_ssim.h \
  get_cls_upgrade.h \
  custom_alarms.h \
  nosir_create_tlg.h \
  prn_forms_layout.h \
  cuws_main.h \
  cuws_handlers.h \
  http_consts.h \
  timatic.h \
  timatic_request.h \
  timatic_response.h \
  timatic_exchange.h \
  zamar_dsm.h \
  astra_types.h \
  pg_session.h \
  base_callbacks.h \
  dbo.h \
  dbostructures.h \
  astra_calls.h \
  wb_messages.h \
  hooked_session.h \
  PgOraConfig.h

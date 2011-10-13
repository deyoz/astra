#include $(TCLMON_PATH)/tclconfig

# set the include path found by configure
INCLUDES = $(all_includes)
export ASTRA_SRC=$(top_srcdir)

bin_PROGRAMS = astra

BUILT_SOURCES = SysTarg
export EDILIB_PATH
SysTarg:
	$(MAKE) -f $(srcdir)/systarg.mk

astra_LDFLAGS = @ORACLE_LDFLAGS@ @SHARED_LDFLAGS@

AM_CXXFLAGS = $(TCLINCLUDE)
astra_SOURCES = \
  adm.cc \
  access.cc \
  aodb.cc \
  apis_edi_file.cc \
  arx_daily.cc \
  astra_callbacks.cc \
  astra_consts.cc \
  astra_context.cc \
  astra_elems.cc \
  astra_locale.cc \
  astra_main.cc \
  astra_misc.cc \
  astra_service.cc \
  astra_tick_read_edi.cpp \
  astra_tick_view_xml.cc \
  astra_utils.cc \
  base_tables.cc \
  brd.cc \
  cache.cc \
  cent.cc \
  checkin.cc \
  comp_layers.cc \
  convert.cc \
  crypt.cc \
  design_blank.cc \
  dev_consts.cc \
  dev_tuning.cc \
  dev_utils.cc \
  develop_dbf.cc \
  docs.cc \
  docs2.cc \
  empty_proc.cc \
  etick_change_status.cpp \
  etick.cc \
  events.cc \
  flight_cent_dbf.cc \
  images.cc \
  locale.cc \
  maindcs.cc \
  obrnosir.cc \
  payment.cc \
  payment2.cc \
  prepreg.cc \
  print.cc \
  prn_tag_store.cc \
  salonform.cc \
  salonform2.cc \
  salons.cc \
  salons2.cc \
  season.cc \
  seats_utils.cc \
  seats.cc \
  seats2.cc \
  sofi.cc \
  sopp.cc \
  spp_cek.cc \
  stages.cc \
  stat.cc \
  telegram.cc \
  telegram2.cc \
  timer.cpp \
  tripinfo.cc \
  web_main.cc \
  xml_unit.cc

astra_LDADD = $(all_libraries) \
	tlg/libtlg.a \
	@ETICKLIB_LIBS@ \
	@ORACLE_LIBS@ @EDILIB_LIBS@ @EDILIB_LIBLOADING@ \
	@JXTLIB_LIBS@ @SHARED_LIBS@ \
	@LIBTLG_LIBS@ @SERVERLIB_LIBS@ \
	$(XML_LIBS) @CHECK_LIBS@\
	@BOOST_SYSTEM_LIB@\
	@BOOST_REGEX_LIB@
#	-L/home/beta/crypt/locallibs/mespro -lmesprox

SUBDIRS = tlg

EXTRA_DIST = systarg.mk run astra.tcl PS Restart kills \
	MAKE local_after_example.tcl local_before_example.tcl

DISTCLEANFILES = local_after.tcl local_before.tcl
noinst_HEADERS = \
  adm.h \
  access.h \
  aodb.h \
  apis_edi_file.h \
  arx_daily.h \
  astra_callbacks.h \
  astra_consts.h \
  astra_context.h \
  astra_elems.h \
  astra_locale.h \
  astra_main.h \
  astra_misc.h \
  astra_service.h \
  astra_tick_read_edi.h \
  astra_tick_reader.h \
  astra_tick_view_xml.h \
  astra_tick_view.h \
  astra_ticket.h \
  astra_utils.h \
  base_tables.h \
  brd.h \
  cache.h \
  cent.h \
  checkin.h \
  comp_layers.h \
  convert.h \
  crypt.h \
  design_blank.h \
  dev_consts.h \
  dev_tuning.h \
  dev_utils.h \
  develop_dbf.h \
  docs.h \
  empty_proc.h \
  etick_change_status.h \
  etick.h \
  events.h \
  flight_cent_dbf.h \
  images.h \
  maindcs.h \
  obrnosir.h \
  payment.h \
  payment2.h \
  prepreg.h \
  print.h \
  prn_tag_store.h \
  salonform.h \
  salonform2.h \
  salons.h \
  salons2.h \
  season.h \
  seats_utils.h \
  seats.h \
  seats2.h \
  sofi.h \
  sopp.h \
  spp_cek.h \
  stages.h \
  stat.h \
  timer.h \
  tripinfo.h \
  web_main.h \
  xml_unit.h




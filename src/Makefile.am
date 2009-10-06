include $(TCLMON_PATH)/tclconfig

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
	adm.cc                   astra_utils.cc  etick_change_status.cpp  salons.cc  timer.cpp \
	astra_callbacks.cc       brd.cc          images.cc                season.cc  tripinfo.cc \
	astra_consts.cc          cache.cc        locale.cc                seats.cc   xml_unit.cc \
	astra_main.cc            cent.cc         maindcs.cc               sopp.cc    print.cc \
	astra_tick_read_edi.cpp  checkin.cc      prepreg.cc               stages.cc  docs.cc \
	astra_tick_view_xml.cc   etick.cc        salonform.cc             stat.cc    telegram.cc \
	events.cc		 base_tables.cc  astra_misc.cc            design_blank.cc \
	astra_service.cc	 develop_dbf.cc	 flight_cent_dbf.cc       payment.cc payment2.cc \
	sofi.cc			 aodb.cc	 spp_cek.cc telegram2.cc  czech_police_edi_file.cpp \
	obrnosir.c		 convert.cc      dev_tuning.cc            astra_context.cc \
	salons2.cc		 salonform2.cc	 seats2.cc		  seats_utils.cc \
	empty_proc.cc    dev_consts.cc   dev_utils.cc     access.cc

astra_LDADD = $(all_libraries) \
	tlg/libtlg.a \
	@ETICKLIB_LIBS@ \
	@ORACLE_LIBS@ @EDILIB_LIBS@ @EDILIB_LIBLOADING@ \
	@JXTLIB_LIBS@ @SHARED_LIBS@ \
	@LIBTLG_LIBS@ @SERVERLIB_LIBS@ \
	$(XML_LIBS) @CHECK_LIBS@\
	@TCLMON_LDFLAGS@ @BOOST_SYSTEM_LIB@

SUBDIRS = tlg

EXTRA_DIST = systarg.mk run astra.tcl PS Restart kills \
	MAKE local_after_example.tcl local_before_example.tcl

DISTCLEANFILES = local_after.tcl local_before.tcl
noinst_HEADERS = \
	adm.h                  astra_tick_view_xml.h  images.h            sopp.h \
	astra_callbacks.h      astra_utils.h          maindcs.h           stages.h \
	astra_consts.h         brd.h                  prepreg.h           stat.h \
	astra_main.h           cache.h                salonform.h         print.h \
	astra_ticket.h         cent.h                 salons.h            timer.h \
	astra_tick_read_edi.h  checkin.h              season.h            tripinfo.h \
	astra_tick_reader.h    etick_change_status.h  seats.h             xml_unit.h \
	astra_tick_view.h      etick.h                docs.h \
	events.h	       base_tables.h	      astra_misc.h        design_blank.h \
	astra_service.h	       develop_dbf.h	      flight_cent_dbf.h   payment.h payment2.h \
	sofi.h		       aodb.h		      spp_cek.h		  convert.h \
	dev_tuning.h           czech_police_edi_file.h                    astra_context.h \
	salons2.h	       salonform2.h	      seats2.h		  seats_utils.h \
	empty_proc.h       dev_consts.h       dev_utils.h     access.h





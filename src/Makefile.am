include $(TCLMON_PATH)/tclconfig

bin_PROGRAMS = astra

-include systarg.mk

# set the include path found by configure
INCLUDES = @SERVERLIB_CXXFLAGS@ @ETICKLIB_CXXFLAGS@ @TCLMON_CXXFLAGS@ \
	$(all_includes)
# the library search path.

BUILT_SOURCES = SysTarg

astra_LDFLAGS = @ORACLE_LDFLAGS@ @SHARED_LDFLAGS@

astra_SOURCES = \
	adm.cc                   astra_utils.cc  etick_change_status.cpp  salons.cc  timer.cpp \
	astra_callbacks.cc       brd.cc          images.cc                season.cc  tripinfo.cc \
	astra_consts.cc          cache.cc        locale.cc                seats.cc   xml_unit.cc \
	astra_main.cc            cent.cc         maindcs.cc               sopp.cc \
	astra_tick_read_edi.cpp  checkin.cc      prepreg.cc               stages.cc \
	astra_tick_view_xml.cc   etick.cc        salonform.cc             stat.cc 

astra_LDADD = $(all_libraries) \
	tlg/libtlg.a \
	@ETICKLIB_LIBS@ \
	@TCLMON_LIBS@ @ORACLE_LIBS@ @EDILIB_LIBS@ @JXTLIB_LIBS@ @SERVERLIB_LIBS@ @SHARED_LIBS@ \
	$(LDZLIB) $(TCL_SYS_LIB) $(LDCRYPT) $(XML_LIBS) @CHECK_LIBS@

SUBDIRS = tlg

EXTRA_DIST = systarg.mk run astra.tcl PS Restart kills \
	MAKE local_after_example.tcl local_before_example.tcl

DISTCLEANFILES = local_after.tcl local_before.tcl
noinst_HEADERS = \
	adm.h                  astra_tick_view_xml.h  images.h     sopp.h \
	astra_callbacks.h      astra_utils.h          maindcs.h    stages.h \
	astra_consts.h         brd.h                  prepreg.h    stat.h \
	astra_main.h           cache.h                salonform.h  testing_def.h \
	astra_ticket.h         cent.h                 salons.h     timer.h \
	astra_tick_read_edi.h  checkin.h              season.h     tripinfo.h \
	astra_tick_reader.h    etick_change_status.h  seats.h      xml_unit.h \
	astra_tick_view.h      etick.h                setup.h


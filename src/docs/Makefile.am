#include $(TCLMON_PATH)/tclconfig

AM_CPPFLAGS = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libdocs.a
libdocs_a_SOURCES = \
  docs_common.cc \
  docs_utils.cc \
  docs_main.cc \
  docs_main2.cc \
  docs_ptm.cc \
  docs_exam.cc \
  docs_notpres.cc \
  docs_refuse.cc \
  docs_rem.cc \
  docs_services.cc \
  docs_reseat.cc \
  docs_pax_list.cc
        

noinst_HEADERS = \
  docs_common.h \
  docs_utils.h \
  docs_main.h \
  docs_ptm.h \
  docs_exam.h \
  docs_notpres.h \
  docs_refuse.h \
  docs_rem.h \
  docs_consts.h \
  docs_services.h \
  docs_reseat.h \
  docs_pax_list.h

#include $(TCLMON_PATH)/tclconfig

AM_CPPFLAGS = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libdocs.a
libdocs_a_SOURCES = \
  common.cc \
  utils.cc \
  main.cc \
  main2.cc \
  ptm.cc \
  exam.cc \
  pax_list.cc
        

noinst_HEADERS = \
  common.h \
  utils.h \
  main.h \
  ptm.h \
  exam.h \
  pax_list.h

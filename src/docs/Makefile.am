#include $(TCLMON_PATH)/tclconfig

AM_CPPFLAGS = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libdocs.a
libdocs_a_SOURCES = \
  common.cc \
  main.cc \
  main2.cc \
  ptm.cc \
  pax_list.cc
        

noinst_HEADERS = \
  common.h \
  main.h \
  ptm.h \
  pax_list.h

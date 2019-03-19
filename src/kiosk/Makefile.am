#include $(TCLMON_PATH)/tclconfig

AM_CPPFLAGS = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libkiosk.a
libkiosk_a_SOURCES = \
  kiosk_events.cc \
  kiosk_config.cc
        

noinst_HEADERS = \
  kiosk_events.h \
  kiosk_config.h

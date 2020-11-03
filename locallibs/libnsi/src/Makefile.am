# set the include path found by configure
INCLUDES= $(all_includes)

lib_LTLIBRARIES=libnsi.la
include $(top_srcdir)/src/single.mk

noinst_HEADERS = nsi.h callbacks.h details.h message_binders.h time_utils.h geozones.h
libnsi_la_SOURCES = nsi.cc packer.cc callbacks.cc time_utils.cc geozones.cc

AM_CPPFLAGS = @CFLAGS@ @SERVERLIB_CXXFLAGS@ @BOOST_CPPFLAGS@


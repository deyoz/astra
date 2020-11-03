# not a GNU package. You can remove this line, if
ACLOCAL_AMFLAGS = -I m4
# have all needed files, that a GNU package needs

AUTOMAKE_OPTIONS = foreign 1.11

include $(top_srcdir)/single.mk
SUBDIRS = src
##AM_CFLAGS = -w -Wall
##AM_CXXFLAGS = -w -Wall

tests:
	$(MAKE) -C  $(top_srcdir)/src $@


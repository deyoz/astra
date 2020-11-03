# not a GNU package. You can remove this line, if
ACLOCAL_AMFLAGS = -I m4
# have all needed files, that a GNU package needs
.PHONY: doc

AUTOMAKE_OPTIONS = foreign 1.11

include $(top_srcdir)/single.mk
SUBDIRS = src doc
AM_CFLAGS = -w -Wall
AM_CXXFLAGS = -w -Wall

xp-tests:
	$(MAKE) -C  $(top_srcdir)/src $@

doc:
	$(MAKE) -C  $(top_srcdir)/doc $@


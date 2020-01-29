# not a GNU package. You can remove this line, if
ACLOCAL_AMFLAGS = -I m4
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.11

.PHONY:

include $(top_srcdir)/single.mk
SUBDIRS = src include
AM_CFLAGS = -w -Wall
AM_CXXFLAGS = -w -Wall

if XP_TESTING
xp-tests:
	$(MAKE) -C  $(top_srcdir)/src $@
endif # XP_TESTING

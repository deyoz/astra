AUTOMAKE_OPTIONS = foreign 1.11
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src edi_messages
EXTRA_DIST=msgproc.awk msgtypes.awk

include $(top_srcdir)/single.mk

.dat.etp:
	@./msgtypes.sh src $^

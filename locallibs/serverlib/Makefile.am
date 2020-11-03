AUTOMAKE_OPTIONS = foreign 1.11
ACLOCAL_AMFLAGS = -I m4

include $(top_srcdir)/single.mk
SUBDIRS = src

#serverlibconfigshdir = $(top_srcdir)
#serverlibconfigsh_DATA = serverlib-config

pkg: arr.tcl
	echo "pkg_mkIndex . arr.tcl" | @TCLSH@
	$(MAKE) pkg -C scripts

all: pkg

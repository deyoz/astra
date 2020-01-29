SUFFIXES = .dat

#AUTOMSG=$(addprefix $(top_srcdir)/include/edilib/, edi_err.h) edimsgout.dat
#bin_PROGRAMS = TestProg
#TestProg_SOURCES = TestProg.c
# set the include path found by configure
INCLUDES= $(all_includes) -DASTRA2007
BUILT_SOURCES= msg_create

msg_create:
	./make_msg.pl
	if ( ! diff msg.h msg.h1 > /dev/null 2>&1 ) ; then \
		mv msg.h1 msg.h; \
	else \
		rm -f msg.h1; \
	fi

lib_LIBRARIES=libshared.a
# the library search path.
#libshared_a_LDFLAGS = -static


#TestProg_LDFLAGS = $(all_libraries)
#TestProg_LDADD = libedilib.a @ORACLE_LIBS@

AM_CPPFLAGS = @TCLLIB_CXXFLAGS@ @SERVERLIB_CXXFLAGS@ @BOOST_CPPFLAGS@ @ORACLE_CXXFLAGS@ @ICU_CFLAGS@ -D ASTRA2007
AM_LDFLAGS = @TCLLIB_LDFLAGS@ @ORACLE_LDFLAGS@ @SERVERLIB_LIBS@ @ORACLE_LIBS@ @ICU_LIBS@

libshared_a_SOURCES = builder.h misc.h parser.cpp  ssod.h \
			exceptions.h exceptions.cpp oralib.cpp parser.h stl_utils.cpp \
			misc.cpp oralib.h ssod.cpp stl_utils.h memory_manager.h memory_manager.cpp \
			date_time.h date_time.cpp
#builder.cpp


$(AUTOMSG):
	$(top_srcdir)/src/msgproc.awk $(top_srcdir)/src/edi_msg.dat



AM_CXXFLAGS =
CLEANFILES = msg.h

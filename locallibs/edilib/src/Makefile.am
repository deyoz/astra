SUFFIXES = .dat
.PHONY:	CREATEMSG
include $(top_srcdir)/single.mk

all-local: CREATEMSG

ERRMSG_H = edi_err.h
edimsgout.dat:$(ERRMSG_H)
BUILT_SOURCES = $(ERRMSG_H) edimsgout.dat
BUILT_SOURCES: Makefile

lib_LTLIBRARIES=libedifact.la

all-local: $(lib_LTLIBRARIES)
	@if [ -f .libs/libedifact.a ]; then \
	        cp -f .libs/libedifact.a $(top_srcdir)/lib/edilib.a; \
	fi

clean-local:
	-rm -f $(top_srcdir)/lib/edilib.a

AM_CFLAGS = -fexceptions @ORACLE_CXXFLAGS@ @EDILIB_CFLAGS@
AM_LDFLAGS = @ORACLE_LDFLAGS@ @TCLLIB_LDFLAGS@ @SERVERLIB_LIBS@

AM_CPPFLAGS =@SERVERLIB_CXXFLAGS@ @LIBTLG_CXXFLAGS@ @ORACLE_CXXFLAGS@ \
             @BOOST_CPPFLAGS@ @TCLLIB_CXXFLAGS@ @EDILIB_CFLAGS@ \
             -I$(top_srcdir)/include/edilib -I$(top_srcdir)/include

libedifact_la_SOURCES =   edi_all.h      edi_malloc.h \
			edi_sql_insert.h  edi_tick_msg_types.h \
			edi_err_msg.h  edi_tables.h \
			edi_func.h     edi_sql_func.h       edi_test.h \
			msgproc.awk edi_msg.dat \
			edi_delete.cpp    edi_insert.cpp \
			edi_read_table.cpp  edi_sql_insert.cpp  edi_test_tab.cpp \
			edi_err.cpp       edi_logger.cpp   edi_print.cpp  edi_sess.cpp \
			edi_sql_slct.cpp  edi_types.cpp \
			edi_get.cpp       edi_msgproc.cpp  edi_put.cpp \
			edi_template.cpp edi_mask_chars.cpp \
			edi_session.cpp edi_request.cpp edi_response_handler.cpp \
			edi_request_handler.cpp \
			EdiSessionTimeOut.cpp fse_types.cpp edi_stuff.cpp \
			edi_session_rc.cpp edi_handler.cpp EdiProcInd.cpp \
			edi_sql_insert_main.cpp \
			edilib_db_callbacks.cpp edilib_dbora_callbacks.cpp \
			edilib_dbpg_callbacks.cpp

include_HEADERS = \
			$(ERRMSG_H)  edi_func_cpp.h  edi_logger.h \
			edi_sess.h  edi_types.h  edi_user_func.h edi_session.h\
			edi_sess_except.h edi_except.h edi_request.h\
            EdiSessionTimeOut.h fse_types.h edi_response_handler.h \
            edi_request_handler.h edi_handler.h edi_session_rc.h \
			EdiProcInd.h \
			edilib_db_callbacks.h edilib_dbora_callbacks.h edilib_dbpg_callbacks.h

$(ERRMSG_H):
	./msgproc.awk ./edi_msg.dat

CLEANFILES = $(BUILT_SOURCES) $(ERRMSG_H)


include $(TCLMON_PATH)/tclconfig

INCLUDES = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libtlg.a
libtlg_a_SOURCES = EdifactRequest.cpp edi_handler.cpp edi_msg.cpp edi_msg.h \
	edi_tlg.cpp edi_tlg.h main_snd.cpp main_srv.cpp tlg.cpp tlg.h tlg_parser.cpp \
	typeb_handler.cpp
noinst_HEADERS = EdiSessionAstra.h EdifactRequest.h

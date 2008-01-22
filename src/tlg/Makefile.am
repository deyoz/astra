include $(TCLMON_PATH)/tclconfig

INCLUDES = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libtlg.a
libtlg_a_SOURCES = edi_handler.cpp  edi_msg.h  main_snd.cpp  \
		   tlg.h tlg.cpp tlg_parser.cpp  typeb_handler.cpp \
		   edi_msg.cpp edi_tlg.h edi_tlg.cpp main_srv.cpp \
		   tlg_parser.cpp

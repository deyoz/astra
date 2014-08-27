#include $(TCLMON_PATH)/tclconfig

INCLUDES = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libtlg.a
libtlg_a_SOURCES = edi_handler.cpp edi_msg.cpp edi_msg.h \
	edi_tlg.cpp edi_tlg.h main_snd.cpp main_srv.cpp tlg.cpp tlg.h tlg_parser.cpp \
	typeb_handler.cpp main_http_snd.cpp \
        ssm_parser.cpp lci_parser.cpp read_edi_elements.cpp view_edi_elements.cpp \
        remote_system_context.cpp edi_elements.cpp emd_disp_request.cpp \
        remote_results.cpp tlg_types.cpp tlg_source.cpp tlg_source_edifact.cpp \
        ResponseHandler.cpp EmdDispResponseHandler.cpp EdifactProfile.cc \
        emd_edifact.cpp AgentWaitsForRemote.cpp CheckinBaseTypes.cc \
        EdifactRequest.cpp EdiSessionTimeOut.cpp EdiHandlersFactory.cpp \
        emd_system_update_request.cpp emd_request.cpp EmdSysUpdateResponseHandler.cpp
        

noinst_HEADERS = EdifactRequest.h read_edi_elements.h edi_elements.h \
   	view_edi_elements.h emd_disp_request.h edi_tkt_request.h \
   	remote_system_context.h remote_results.h tlg_types.h tlg_source.h \
   	tlg_source_edifact.h ResponseHandler.h EmdDispResponseHandler.h \
   	EdifactProfile.h emd_edifact.h AgentWaitsForRemote.h CheckinBaseTypes.h \
   	EdiSessionTimeOut.h EdiHandlersFactory.h CheckinBaseTypesOci.h \
   	emd_system_update_request.h emd_request.h EmdSysUpdateResponseHandler.h
   	
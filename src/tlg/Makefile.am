#include $(TCLMON_PATH)/tclconfig

AM_CPPFLAGS = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libtlg.a
libtlg_a_SOURCES = apps_handler.cc apps_answer_emul.cc edi_handler.cpp edi_msg.cpp edi_tlg.cpp main_snd.cpp \
	main_srv.cpp tlg.cpp tlg_parser.cpp typeb_handler.cpp main_http_snd.cpp \
        lci_parser.cpp read_edi_elements.cpp view_edi_elements.cpp \
		ucm_parser.cpp \
        remote_system_context.cpp edi_elements.cpp emd_disp_request.cpp \
        remote_results.cpp tlg_types.cpp tlg_source.cpp tlg_source_edifact.cpp \
        ResponseHandler.cpp EmdDispResponseHandler.cpp EdifactProfile.cc \
        RequestHandler.cpp emd_edifact.cpp AgentWaitsForRemote.cpp CheckinBaseTypes.cpp \
        EdifactRequest.cpp EdiSessionTimeOut.cpp EdiHandlersFactory.cpp \
        emd_system_update_request.cpp emd_request.cpp EmdSysUpdateResponseHandler.cpp \
        emd_cos_request.cpp EmdCosResponseHandler.cpp IatciCkiRequest.cpp \
        IatciCkuRequest.cpp IatciCkxRequest.cpp IatciResponseHandler.cpp IatciRequestHandler.cpp \
        IatciCkiResponseHandler.cpp IatciCkuResponseHandler.cpp IatciCkxResponseHandler.cpp \
        IatciCkiRequestHandler.cpp IatciCkuRequestHandler.cpp IatciCkxRequestHandler.cpp \
        IatciPlfRequest.cpp IatciPlfResponseHandler.cpp IatciPlfRequestHandler.cpp \
        IatciSeatmapResponseHandler.cpp IatciSmfRequest.cpp IatciSmfResponseHandler.cpp \
        IatciBprRequest.cpp IatciBprResponseHandler.cpp IatciBprRequestHandler.cpp \
        IatciSeatmapRequestHandler.cpp IatciSmfRequestHandler.cpp EtDispResponseHandler.cpp \
        EtCosResponseHandler.cpp postpone_edifact.cpp mvt_parser.cpp et_request.cpp request_params.cpp \
        et_disp_request.cpp et_cos_request.cpp typeb_handler.cpp IatciIfm.cpp tlg_source_typeb.cpp \
        TpbMessage.cpp ifm_parser.cpp typeb_template_init.cpp UacRequestHandler.cpp et_rac_request.cpp \
        EtRacResponseHandler.cpp EtCosRequestHandler.cpp
        

noinst_HEADERS = EdifactRequest.h read_edi_elements.h edi_msg.h edi_elements.h \
   	view_edi_elements.h emd_disp_request.h edi_tlg.h tlg.h \
   	remote_system_context.h remote_results.h tlg_types.h tlg_source.h \
   	tlg_source_edifact.h ResponseHandler.h EmdDispResponseHandler.h \
   	EdifactProfile.h emd_edifact.h AgentWaitsForRemote.h CheckinBaseTypes.h \
   	RequestHandler.h EdiSessionTimeOut.h EdiHandlersFactory.h CheckinBaseTypesOci.h \
   	emd_system_update_request.h emd_request.h EmdSysUpdateResponseHandler.h \
   	emd_cos_request.h EmdCosResponseHandler.h IatciCkiRequest.h \
   	IatciCkuRequest.h IatciCkxRequest.h IatciResponseHandler.h IatciRequestHandler.h \
   	IatciCkiResponseHandler.h IatciCkuResponseHandler.h IatciCkxResponseHandler.h \
   	IatciCkiRequestHandler.h IatciCkuRequestHandler.h IatciCkxRequestHandler.h \
   	IatciPlfRequest.h IatciPlfResponseHandler.h IatciPlfRequestHandler.h \
   	IatciSeatmapResponseHandler.h IatciSmfRequest.h IatciSmfResponseHandler.h \
   	IatciBprRequest.h IatciBprResponseHandler.h IatciBprRequestHandler.h \
   	IatciSeatmapRequestHandler.h IatciSmfRequestHandler.h EtDispResponseHandler.h \
   	EtCosResponseHandler.h postpone_edifact.h mvt_parser.h et_request.h request_params.h \
   	et_disp_request.h et_cos_request.h typeb_handler.h IatciIfm.h tlg_source_typeb.h \
   	TpbMessage.h ifm_parser.h typeb_template_init.h UacRequestHandler.h et_rac_request.h \
   	EtRacResponseHandler.h EtCosRequestHandler.h
   	  	

#include $(TCLMON_PATH)/tclconfig

AM_CPPFLAGS = -I$(top_srcdir)/src $(all_includes) $(TCLINCLUDE)
METASOURCES = AUTO
lib_LIBRARIES = libtlg.a
libtlg_a_SOURCES = apps_handler.cc apps_answer_emul.cc edi_handler.cpp edi_msg.cpp \
	edi_tlg.cpp edi_tlg.h main_snd.cpp main_srv.cpp tlg.cpp tlg.h tlg_parser.cpp \
	typeb_handler.cpp main_http_snd.cpp lci_parser.cpp read_edi_elements.cpp \
	view_edi_elements.cpp ucm_parser.cpp remote_system_context.cpp edi_elements.cpp emd_disp_request.cpp \
        remote_results.cpp tlg_types.cpp tlg_source.cpp tlg_source_edifact.cpp tlg_source_typeb.cpp \
        ResponseHandler.cpp EmdDispResponseHandler.cpp EdifactProfile.cc \
        emd_edifact.cpp AgentWaitsForRemote.cpp CheckinBaseTypes.cpp \
        EdifactRequest.cpp EdiSessionTimeOut.cpp EdiHandlersFactory.cpp \
        emd_system_update_request.cpp emd_request.cpp EmdSysUpdateResponseHandler.cpp \
        emd_cos_request.cpp EmdCosResponseHandler.cpp mvt_parser.cpp TpbMessage.cpp \
        RequestHandler.cpp EtCosRequestHandler.cpp EtRacResponseHandler.cpp \
        UacRequestHandler.cpp et_rac_request.cpp ifm_parser.cpp postpone_edifact.cpp \
        typeb_template_init.cc request_params.cpp et_disp_request.cpp et_cos_request.cpp \
	et_request.cpp EtDispResponseHandler.cpp EtCosResponseHandler.cpp \
        IatciCkiRequest.cpp IatciCkuRequest.cpp IatciCkxRequest.cpp IatciResponseHandler.cpp \
        IatciRequestHandler.cpp IatciCkiResponseHandler.cpp IatciCkuResponseHandler.cpp \
        IatciCkxResponseHandler.cpp IatciCkiRequestHandler.cpp IatciCkuRequestHandler.cpp \
        IatciCkxRequestHandler.cpp IatciPlfRequest.cpp IatciPlfResponseHandler.cpp IatciPlfRequestHandler.cpp \
        IatciSeatmapResponseHandler.cpp IatciSmfRequest.cpp IatciSmfResponseHandler.cpp \
        IatciBprRequest.cpp IatciBprResponseHandler.cpp IatciBprRequestHandler.cpp \
        IatciSeatmapRequestHandler.cpp IatciSmfRequestHandler.cpp IatciIfm.cpp \
        CusResponseHandler.cpp CusRequestHandler.cpp \
        ldm_parser.cpp paxlst_request.cpp typeb_remarks.cc uws_parser.cpp
        
        

noinst_HEADERS = EdifactRequest.h read_edi_elements.h edi_elements.h \
   	view_edi_elements.h emd_disp_request.h typeb_handler.h \
   	remote_system_context.h remote_results.h tlg_types.h tlg_source.h \
   	tlg_source_edifact.h tlg_source_typeb.h ResponseHandler.h EmdDispResponseHandler.h \
   	EdifactProfile.h emd_edifact.h AgentWaitsForRemote.h CheckinBaseTypes.h edi_msg.h \
   	EdiSessionTimeOut.h EdiHandlersFactory.h CheckinBaseTypesOci.h \
   	emd_system_update_request.h emd_request.h EmdSysUpdateResponseHandler.h \
   	emd_cos_request.h EmdCosResponseHandler.h mvt_parser.h TpbMessage.h \
   	RequestHandler.h EtCosRequestHandler.h EtRacResponseHandler.h \
   	UacRequestHandler.h et_rac_request.h ifm_parser.h postpone_edifact.h \
   	typeb_template_init.h request_params.h et_disp_request.h et_cos_request.h \
   	et_request.h EtDispResponseHandler.h EtCosResponseHandler.h \
   	IatciCkiRequest.h IatciCkuRequest.h IatciCkxRequest.h IatciResponseHandler.h \
   	IatciRequestHandler.h IatciCkiResponseHandler.h IatciCkuResponseHandler.h \
   	IatciCkxResponseHandler.h IatciCkiRequestHandler.h IatciCkuRequestHandler.h \
   	IatciCkxRequestHandler.h IatciPlfRequest.h IatciPlfResponseHandler.h IatciPlfRequestHandler.h \
        IatciSeatmapResponseHandler.h IatciSmfRequest.h IatciSmfResponseHandler.h \
        IatciBprRequest.h IatciBprResponseHandler.h IatciBprRequestHandler.h \
        IatciSeatmapRequestHandler.h IatciSmfRequestHandler.h IatciIfm.h \
        CusResponseHandler.h CusRequestHandler.h \
        ldm_parser.h paxlst_request.h typeb_remarks.h uws_parser.h
   	
   	

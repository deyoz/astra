#noinst_PROGRAMS = TestProg
#TestProg_SOURCES = testapp.cc
# set the include path found by configure
INCLUDES= $(all_includes) -I$(top_srcdir)/include/typeb -I$(top_srcdir)/include

lib_LTLIBRARIES=libtypeb.la
# the library search path.


#TestProg_LDFLAGS = $(all_libraries) @ORACLE_LDFLAGS@ @ETICKLIB_CXXFLAGS@ @SERVERLIB_CXXFLAGS@
#TestProg_LDADD = @SERVERLIB_LIBS@ \
#    @CHECK_LIBS@ \
#    @ORACLE_LIBS@ \
#    libtypeb.la \
#	-ltcl -lcheck -lcrypto -lssl -lboost_regex-mt

include $(top_srcdir)/src/single.mk
AM_CPPFLAGS = -fexceptions @SERVERLIB_CXXFLAGS@ @ETICKLIB_CXXFLAGS@ @BOOST_CPPFLAGS@
AM_LDFLAGS =

libtypeb_la_SOURCES = \
ItinElem.cc      RemTkneElem.cc     typeb_core_parser.cc\
NameElem.cc      SeatConfigElem.cc  tb_elements.cc     typeb_msg.cc\
recloc_element.cc  ssr_parser.cc      test_true_typeb.cc   typeb_parse_exceptions.cc\
FlightElem.cc    EndElem.cc 		RemFoidElem.cc     ssr_tknx.cc        TotalsByDestElem.cc  typeb_template.cc\
ActDepTimeTemplate.cc  FlightTemplate.cc           NameTemplate.cc    SeatConfigTemplate.cc\
EndTemplate.cc         MarketingFlInfoTemplate.cc  RemarkTemplate.cc  TotalsByDestTemplate.cc\
NumericsByDestElem.cc NumericsByDestTemplate.cc \
CategoryApTemplate.cc CategoryApElem.cc CategoryTemplate.cc CategoryElem.cc FreqTrElem.cc \
FreqTrTemplate.cc AutomPNRAddr.cc AutomPNRAddrTemplate.cc typeb_pp.cc RemFqtxElem.cc RemDocsElem.cc \
AvaElem.cc AvaTemplate.cc AvsElem.cc AvsTemplate.cc RvrElem.cc RvrTemplate.cc \
AnyStringElem.cc AnyStringTemplate.cc ReferenceElem.cc ReferenceTemplate.cc \
SsmElem.cc SsmTemplate.cc AsmTypes.cc TlgStrings.cc \
RasElem.cc RasTemplate.cc ssr_asvx.cc \
ASM_template.cc AVA_template.cc AVR_template.cc \
AVS_template.cc RAS_template.cc RVR_template.cc SSM_template.cc

#AM_CXXFLAGS = -Wall -O0
CLEANFILES =

noinst_HEADERS = \
ActDepTimeTemplate.h         ItinElem.h                 RemarkTemplate.h      ssr_tknx.h              typeb_core_parser.h\
AutomPNRAddrTemplate.h       MarketingFlInfoTemplate.h  RemFoidElem.h         TailAddressTemplate.h   typeb_message.h\
BaggagePoolingGrTemplate.h   NameElem.h                 RemTkneElem.h         tb_elements.h           typeb_msg.h\
BaggageTagDetailsTemplate.h  NameTemplate.h             SeatConfigElem.h      test_true_typeb.h       typeb_parse_exceptions.h\
EndTemplate.h                NilTemplate.h              SeatConfigTemplate.h  TotalsByDestElem.h      typeb_template.h\
FlightElem.h                 PiecesDataTemplate.h       SentTemplate.h        TotalsByDestTemplate.h  EndElem.h\
FlightTemplate.h             recloc_element.h           ssr_parser.h          typeb_cast.h  NumericsByDestElem.h\
NumericsByDestTemplate.h CategoryApTemplate.h CategoryApElem.h CategoryElem.h CategoryTemplate.h FreqTrElem.h \
FreqTrTemplate.h AutomPNRAddr.h typeb_pp.h RemFqtxElem.h RemDocsElem.h \
AvaTemplate.h AvaElem.h AvsTemplate.h AvsElem.h RvrElem.h RvrTemplate.h AsmTypes.h AsmStrings.h \
RasElem.h RasTemplate.h ssr_asvx.h \
ASM_template.h AVA_template.h AVR_template.h \
AVS_template.h RAS_template.h RVR_template.h SSM_template.h


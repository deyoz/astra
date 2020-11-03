lib_LTLIBRARIES=libssim.la

include $(top_srcdir)/single.mk

noinst_HEADERS =\
	oag_filter.h oag_parser.h oag_records.h oag_records_handler.h \
	traffic_restriction.h \
	callbacks.h \
	csh_context.h \
	csh_settings.h \
	dei_default_values.h \
	dei_handlers.h \
	inflight_service.h \
	meal_service.h \
	ssim_data_types.h \
	ssim_enums.h \
	ssim_parse.h \
	ssim_proc.h \
	ssim_schedule.h \
	ssim_utils.h \
	ssm_data_types.h \
	ssm_proc.h \
	asm_data_types.h \
	dei_subsets.h \
	ssim_create.h \
	mct.h \
	mct_parse.h \
	packer.h

libssim_la_SOURCES =\
    ssimexport.cc oag_filter.cc oag_parser.cc oag_records_handler.cc traffic_restriction.cc \
	csh_context.cc \
	dei_handlers.cc \
	inflight_service.cc \
	meal_service.cc \
	ssim_data_types.cc \
	ssim_parse.cc \
	ssim_proc.cc \
	ssim_schedule.cc \
	ssim_utils.cc \
	ssm_data_types.cc \
	ssm_parse.cc \
	ssm_proc.cc \
	asm_data_types.cc \
	asm_parse.cc \
	dei_subsets.cc \
	ssim_create.cc \
	mct.cc \
	mct_parse.cc \
	packer.cc

AM_CPPFLAGS = -I$(top_srcdir)/include$(all_includes) \
			  -Werror=switch \
              @SERVERLIB_CXXFLAGS@ \
			  @LIBCORETYPES_CXXFLAGS@ \
			  @LIBNSI_CXXFLAGS@ \
			  @LIBTYPEB_CXXFLAGS@ \
			  @ETICKLIB_CXXFLAGS@ \
			  @BOOST_CPPFLAGS@


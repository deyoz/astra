INCLUDES = -I$(top_srcdir)/src $(all_includes)
AM_CXXFLAGS=`if test -d $(top_builddir)/src/precompile.gch ; then echo "-include $(top_builddir)/src/precompile"; fi`
METASOURCES = AUTO
lib_LTLIBRARIES = libloading.la
libloading_la_SOURCES = edi_sql_slct.c edi_sql_insert_main.c

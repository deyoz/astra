#!/bin/bash -e

function uab_check_version() {
#    grep -w '^[ ]*#[ ]*define[ ]\+LIBXML_VERSION[ ]\+20904' $1/include/libxml2/libxml/xmlversion.h &>/dev/null
    grep -w '^[ ]*#[ ]*define[ ]\+LIBXML_VERSION[ ]\+20706' $1/include/libxml2/libxml/xmlversion.h &>/dev/null
}

function uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1

    echo '--- configure.in	2009-10-06 16:28:58.000000000 +0000
+++ configure.in	2017-01-19 20:20:08.000000000 +0000
@@ -61,7 +61,7 @@
 AC_PATH_PROG(XSLTPROC, xsltproc, /usr/bin/xsltproc)
 
 dnl Make sure we have an ANSI compiler
-AM_C_PROTOTYPES
+#AM_C_PROTOTYPES
 test "x$U" != "x" && AC_MSG_ERROR(Compiler not ANSI compliant)
 
 AC_LIBTOOL_WIN32_DLL'| patch -p0 
        
    autoreconf --force --install
    ./configure --disable-maintainer-mode --with-mem-debug --without-python --without-zlib --prefix=$prefix CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" $@
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
#    echo libxml2-2.9.4.tar.gz
    echo libxml2-2.7.6.tar.gz
}


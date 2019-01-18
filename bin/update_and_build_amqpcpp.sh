#!/bin/bash -e

prefix=${1:?no prefix parameter}

uab_config_and_build() {

    if [ -n "${PLATFORM:-}" ] ; then
        export CXX="$LOCALCXX -$PLATFORM"
        export CC="$LOCALCC -$PLATFORM"
    else
        export CXX=$LOCALCXX
        export CC=$LOCALCC
    fi

    glibcxx_debug=
    if [ "${ENABLE_GLIBCXX_DEBUG:-0}" = "1" ]; then
        glibcxx_debug='-D_GLIBCXX_DEBUG'
    fi

    patch -p1 <<'EOF'
--- a/Makefile	Fri Dec 02 16:11:46 2016 +0300
+++ b/Makefile	Fri Dec 02 16:21:39 2016 +0300
@@ -32,6 +32,13 @@
 		cp -f include/*.h ${INCLUDE_DIR}/$(LIBRARY_NAME)
 		-cp -f src/lib$(LIBRARY_NAME).so.$(VERSION) ${LIBRARY_DIR}
 		-cp -f src/lib$(LIBRARY_NAME).a.$(VERSION) ${LIBRARY_DIR}
-		ln -r -s -f $(LIBRARY_DIR)/lib$(LIBRARY_NAME).so.$(VERSION) $(LIBRARY_DIR)/lib$(LIBRARY_NAME).so.$(SONAME)
-		ln -r -s -f $(LIBRARY_DIR)/lib$(LIBRARY_NAME).so.$(VERSION) $(LIBRARY_DIR)/lib$(LIBRARY_NAME).so
-		ln -r -s -f $(LIBRARY_DIR)/lib$(LIBRARY_NAME).a.$(VERSION) $(LIBRARY_DIR)/lib$(LIBRARY_NAME).a
+		ln -s -f $(LIBRARY_DIR)/lib$(LIBRARY_NAME).a.$(VERSION) $(LIBRARY_DIR)/lib$(LIBRARY_NAME).a
+
+uninstall:
+		rm -rf ${INCLUDE_DIR}/$(LIBRARY_NAME)
+		rm -f  ${INCLUDE_DIR}/$(LIBRARY_NAME).h
+		rm -f  ${LIBRARY_DIR}/lib$(LIBRARY_NAME).*
+
+static_pure:
+		$(MAKE) -C src static_pure
+
EOF
    patch -p1 << EOF
--- a/src/Makefile	2018-12-10 14:16:00.067812038 +0300
+++ b/src/Makefile	2018-12-10 14:25:47.638856969 +0300
@@ -1,7 +1,7 @@
-CPP	        		= g++
+CPP	        		= $CXX
 RM	        		= rm -f
-CPPFLAGS			= -Wall -c -I../include -std=c++11 -MD
-LD	        		= g++
+CPPFLAGS			= -Wall -c -I../include -std=$CPP_STD_VERSION -MD -fPIC $glibcxx_debug
+LD	        		= $CXX
 LD_FLAGS			= -Wall -shared
 SHARED_LIB			= lib\$(LIBRARY_NAME).so.\$(VERSION)
 STATIC_LIB			= lib\$(LIBRARY_NAME).a.\$(VERSION)
EOF

    make static_pure PREFIX=$prefix CPP="$CXX"
    make install PREFIX=$prefix

    uab_chk_pc $prefix amqp-cpp '4.0.1' "'-lamqpcpp'"
}

uab_check_version() {
    target_v='4.0.1'
    cur_v=`sed -n  's/.*VERSION.*= *\([^ ]\+\) */\1/p' "$1/src/Makefile"`
    test "$cur_v" == "$target_v"
    test `awk '/^Version: /{print $2}' $1/lib/pkgconfig/amqp-cpp.pc` = '4.0.1'
}

uab_pkg_tarball() {
    echo AMQP-CPP-4.0.1.tar.bz2
}

uab_sha1sum() {
    echo "7863723210397ca7c24b855dae2784d1000efcaa"
}

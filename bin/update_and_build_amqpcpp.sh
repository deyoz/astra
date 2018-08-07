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

    if [ "$CPP_STD_VERSION" = "c++11" ]; then
        amqp_cppflags="-Wall -c -I. -std=c++11 -g"
    elif [ "$CPP_STD_VERSION" = "c++14" ]; then
        amqp_cppflags="-Wall -c -I. -std=c++14 -g"
    fi

    if [ "${ENABLE_GLIBCXX_DEBUG:-0}" = "1" ]; then
        amqp_cppflags+=' -D_GLIBCXX_DEBUG'
    fi

    patch -p1 <<'EOF'
--- a/Makefile	Fri Dec 02 16:11:46 2016 +0300
+++ b/Makefile	Fri Dec 02 16:21:39 2016 +0300
@@ -30,6 +30,13 @@
		cp -f include/*.h ${INCLUDE_DIR}/$(LIBRARY_NAME)
-		-cp -f src/lib$(LIBRARY_NAME).so.$(VERSION) ${LIBRARY_DIR}
+		
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

    make static_pure PREFIX=$prefix CPPFLAGS="$amqp_cppflags -MD -g -fpic" CPP="$CXX"
    make install PREFIX=$prefix
}

uab_check_version() {
    target_v='2.6.2'
    cur_v=`sed -n  's/.*VERSION.*= *\([^ ]\+\) */\1/p' "$1/src/Makefile"`
    test "$cur_v" == "$target_v"
}

uab_pkg_tarball() {
    echo amqpcpp.2.6.2.tar.gz
}

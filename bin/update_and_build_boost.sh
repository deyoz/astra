#!/bin/bash
set -e

wanna_boost_ver=107600

grep_version() {
    grep -w "BOOST_VERSION[ ]\+$wanna_boost_ver" $1/boost/version.hpp
}

uab_config_and_build() {

prefix=${1:?no prefix parameter}

toolset=''
[[ $LOCALCC =~ clang ]] && toolset="clang"
[[ $LOCALCC =~ gcc ]] && toolset="gcc"

export BOOST_BUILD_USER_CONFIG="$prefix/src/user-config.jam"
[ -f "$BOOST_BUILD_USER_CONFIG" ] && die 1 "$BOOST_BUILD_USER_CONFIG is already here - another $0 is running? If not, remove the file and try again."
touch "$BOOST_BUILD_USER_CONFIG"

cxxflags=
b2_options=
if [ -n "$toolset" ]; then
    for tok in $LOCALCXX; do
        if [[ $tok = -* ]] ; then
            [[ $tok != -fsanitize=* ]] && [[ $tok != -$PLATFORM ]] && cxxflags="$cxxflags cxxflags=$tok"
        elif [ -z "$b2_options" ] && which $tok && $tok --version && $tok -dumpversion; then
            cxx=`which $tok`
            cxx_version=`$cxx -dumpversion`
            if ! which $toolset &> /dev/null || [ "$cxx_version" != "`$toolset -dumpversion`" ]; then
                cat <<EOF > "$BOOST_BUILD_USER_CONFIG"
using $toolset : $cxx_version : $cxx ;
EOF
                if [ -n "$cxx_version" ]; then
                    b2_options="toolset=$toolset-$cxx_version"
                fi
            fi
        fi
    done
    if [ -z "$b2_options" ]; then
        b2_options="toolset=$toolset"
    fi
fi
b2_options="$b2_options --disable-icu --ignore-site-config"

# pyconfig.h compile error fix
python_include_path=`(pkg-config python2 --cflags-only-I --silence-errors || true) | awk '{print $1}'`

glibcxxdebug=''
[ "${ENABLE_GLIBCXX_DEBUG:-}" = "1" ] && glibcxxdebug='cxxflags=-D_GLIBCXX_DEBUG'

if [ -n "$python_include_path" ]; then
    cxxflags="$cxxflags cxxflags='$python_include_path'"
fi
[ -n "$CPP_STD_VERSION" ] && cxxflags="$cxxflags cxxflags=-std=$CPP_STD_VERSION"
if [ "${SIRENA_USE_LIBCXX:-}" = "1" ]; then
    cxxflags="$cxxflags cxxflags=-stdlib=libc++ linkflags=-stdlib=libc++"
fi
#cxxflags="cxxflags=-fPIC linkflags=-fPIC $cxxflags"

address_model=''
[ "${PLATFORM:-}" = "m64" ] && address_model='address-model=64'
[ "${PLATFORM:-}" = "m32" ] && address_model='address-model=32'

b2_options="$b2_options threading=multi --layout=system"
[[ ${SIRENA_ENABLE_SHARED:-} == 1 ]] && b2_options="$b2_options link=shared runtime-link=shared"
[[ ${SIRENA_ENABLE_SHARED:-} == 0 ]] && b2_options="$b2_options link=static runtime-link=static"

exclude_libs_1_57="math mpi atomic graph_parallel context coroutine signals python wave locale random timer"
exclude_libs_1_76=" atomic container context coroutine fiber graph_parallel locale log math mpi python random test timer type_erasure wave contract graph headers nowide"
test $wanna_boost_ver = 107600 && exclude_libs="$exclude_libs_1_76" || exclude_libs="$exclude_libs_1_57"
b2_options="$b2_options `echo \"$exclude_libs\" | sed 's/ \b/ --without-/g'`"
cd $prefix/src

test $wanna_boost_ver != 107600 && patch -p0 << EOF
--- boost/config/stdlib/libcpp.hpp	2016-12-22 15:33:14.000000000 +0300
+++ boost/config/stdlib/libcpp.hpp	2020-01-15 16:11:40.045855577 +0300
@@ -110,4 +110,7 @@
 #  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
 #endif

+#ifndef BOOST_NO_AUTO_PTR
+#define BOOST_NO_AUTO_PTR
+#endif
 //  --- end ---
EOF
echo '--- boost/date_time/time_clock.hpp    2015-03-04 01:19:01.000000000 +0300
+++ boost/date_time/time_clock.hpp    2016-02-16 15:16:08.380216972 +0300
@@ -14,7 +14,6 @@
 */
 
 #include "boost/date_time/c_time.hpp"
-#include "boost/shared_ptr.hpp"
 
 namespace boost {
 namespace date_time {
@@ -53,12 +52,12 @@
       return create_time(curr_ptr);
     }
 
-    template<class time_zone_type>
-    static time_type local_time(boost::shared_ptr<time_zone_type> tz_ptr)
+    template<class time_zone_ptr_type>
+    static time_type local_time(time_zone_ptr_type&& tz_ptr)
     {
       typedef typename time_type::utc_time_type utc_time_type;
       utc_time_type utc_time = second_clock<utc_time_type>::universal_time();
-      return time_type(utc_time, tz_ptr);
+      return time_type(utc_time, std::forward<time_zone_ptr_type>(tz_ptr));
     }
 

' | patch -p0
test $wanna_boost_ver != 107600 && patch -p0 << EOF
--- libs/date_time/src/gregorian/greg_month.cpp 2016-02-18 13:42:17.140625396 +0300
+++ libs/date_time/src/gregorian/greg_month.cpp 2016-02-18 13:45:05.701701551 +0300
@@ -34,21 +34,21 @@
    */
   greg_month::month_map_ptr_type greg_month::get_month_map_ptr()
   {
-    static month_map_ptr_type month_map_ptr(new greg_month::month_map_type());
+    static greg_month::month_map_type month_map;
 
-    if(month_map_ptr->empty()) {
+    if(month_map.empty()) {
       std::string s("");
       for(unsigned short i = 1; i <= 12; ++i) {
         greg_month m(static_cast<month_enum>(i));
         s = m.as_long_string();
         s = date_time::convert_to_lower(s);
-        month_map_ptr->insert(std::make_pair(s, i));
+        month_map.insert(std::make_pair(s, i));
         s = m.as_short_string();
         s = date_time::convert_to_lower(s);
-        month_map_ptr->insert(std::make_pair(s, i));
+        month_map.insert(std::make_pair(s, i));
       }
     }
-    return month_map_ptr;
+    return &month_map;
   }
 

--- boost/date_time/gregorian/greg_month.hpp    2016-02-18 13:42:56.063411961 +0300
+++ boost/date_time/gregorian/greg_month.hpp    2016-02-18 13:45:11.624669109 +0300
@@ -11,7 +11,6 @@
 
 #include "boost/date_time/constrained_value.hpp"
 #include "boost/date_time/date_defs.hpp"
-#include "boost/shared_ptr.hpp"
 #include "boost/date_time/compiler_config.hpp"
 #include <stdexcept>
 #include <string>
@@ -56,7 +55,7 @@
   public:
     typedef date_time::months_of_year month_enum;
     typedef std::map<std::string, unsigned short> month_map_type;
-    typedef boost::shared_ptr<month_map_type> month_map_ptr_type;
+    typedef month_map_type* month_map_ptr_type;
     //! Construct a month from the months_of_year enumeration
     greg_month(month_enum theMonth) : 
       greg_month_rep(static_cast<greg_month_rep::value_type>(theMonth)) {}


--- libs/regex/src/regex_raw_buffer.cpp	2014-10-18 19:54:49.000000000 +0400
+++ libs/regex/src/regex_raw_buffer.cpp	2021-07-22 09:34:20.809678112 +0300
@@ -36,15 +36,15 @@
 
 void BOOST_REGEX_CALL raw_storage::resize(size_type n)
 {
-   register size_type newsize = start ? last - start : 1024;
+   size_type newsize = start ? last - start : 1024;
    while(newsize < n)
       newsize *= 2;
-   register size_type datasize = end - start;
+   size_type datasize = end - start;
    // extend newsize to WORD/DWORD boundary:
    newsize = (newsize + padding_mask) & ~(padding_mask);
 
    // allocate and copy data:
-   register pointer ptr = static_cast<pointer>(::operator new(newsize));
+   pointer ptr = static_cast<pointer>(::operator new(newsize));
    BOOST_REGEX_NOEH_ASSERT(ptr)
    if(start)
       std::memcpy(ptr, start, datasize);
@@ -63,7 +63,7 @@
    BOOST_ASSERT(pos <= size_type(end - start));
    if(size_type(last - end) < n)
       resize(n + (end - start));
-   register void* result = start + pos;
+   void* result = start + pos;
    std::memmove(start + pos + n, start + pos, (end - start) - pos);
    end += n;
    return result;
 

--- boost/asio/ssl/impl/context.ipp 2016-03-30 10:34:22.588612334 +0300
+++ boost/asio/ssl/impl/context.ipp 2016-03-30 10:36:21.794878510 +0300
@@ -89,6 +89,14 @@
     handle_ = ::SSL_CTX_new(::SSLv2_server_method());
     break;
 #endif // defined(OPENSSL_NO_SSL2)
+#if defined(OPENSSL_NO_SSL3_METHOD)
+  case context::sslv3:
+  case context::sslv3_client:
+  case context::sslv3_server:
+    boost::asio::detail::throw_error(
+        boost::asio::error::invalid_argument, "context");
+    break;
+#else // defined(OPENSSL_NO_SSL3_METHOD)
   case context::sslv3:
     handle_ = ::SSL_CTX_new(::SSLv3_method());
     break;
@@ -98,6 +106,7 @@
   case context::sslv3_server:
     handle_ = ::SSL_CTX_new(::SSLv3_server_method());
     break;
+#endif // defined(OPENSSL_NO_SSL3_METHOD)
   case context::tlsv1:
     handle_ = ::SSL_CTX_new(::TLSv1_method());
     break;
 

--- boost/serialization/singleton.hpp   2016-07-05 13:16:29.706484854 +0300
+++ boost/serialization/singleton.hpp   2016-07-05 13:13:04.739070578 +0300
@@ -123,13 +123,13 @@
 private:
     BOOST_DLLEXPORT static T & instance;
     // include this to provoke instantiation at pre-execution time
-    static void use(T const &) {}
+    static void use(T const *) {}
     BOOST_DLLEXPORT static T & get_instance() {
         static detail::singleton_wrapper< T > t;
         // refer to instance, causing it to be instantiated (and
         // initialized at startup on working compilers)
         BOOST_ASSERT(! detail::singleton_wrapper< T >::m_is_destroyed);
-        use(instance);
+        use(& instance);
         return static_cast<T &>(t);
     }
 public:
--- boost/optional/optional_fwd.hpp 2016-07-04 18:12:16.088459101 +0300
+++ boost/optional/optional_fwd.hpp 2016-07-04 18:12:08.619474276 +0300
@@ -16,7 +16,7 @@
#ifndef BOOST_OPTIONAL_OPTIONAL_FWD_FLC_19NOV2002_HPP
#define BOOST_OPTIONAL_OPTIONAL_FWD_FLC_19NOV2002_HPP
 
-#include <boost/config/suffix.hpp>
+#include <boost/config.hpp>
 
 namespace boost {
EOF

sed -i 's/print sys\.prefix/print(sys.prefix)/' ./bootstrap.sh
CC="$LOCALCC" ./bootstrap.sh --prefix=$prefix --without-icu
b2_build_flags="$cxxflags $glibcxxdebug $address_model $b2_options --build_dir=/tmp/boost_build/$prefix --prefix=$prefix -j${MAKE_J:-3}"
echo "./b2 $b2_build_flags clean" > build.clean
echo "./b2 -q $b2_build_flags stage" | tee build.stage
./b2 -q $b2_build_flags stage > /dev/null #really annoying output
echo "./b2 install"
./b2 $b2_build_flags install > /dev/null # -- # --
echo "./b2 $b2_build_flags install" > build.previous
cat "$BOOST_BUILD_USER_CONFIG" >> build.previous
rm -f "$BOOST_BUILD_USER_CONFIG"

    needed_libs='-lboost_serialization -lboost_system -lboost_date_time -lboost_program_options -lboost_filesystem -lboost_regex'
    test $wanna_boost_ver = 106300 && uab_chk_pc $prefix boost '1.63.0' "'$needed_libs', Cflags='-DBOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS -DBOOST_FILESYSTEM_DEPRECATED'"
    test $wanna_boost_ver = 107600 && uab_chk_pc $prefix boost '1.76.0' "'$needed_libs', Cflags='-DBOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS -DBOOST_FILESYSTEM_DEPRECATED'"
    true
}

uab_check_version() {
grep_version $1/include && \
    ! grep -w shared_ptr $1/include/boost/date_time/gregorian/greg_month.hpp && \
    ! grep -w shared_ptr $1/include/boost/date_time/time_clock.hpp && \
    test `awk '/^Version: /{print $2}' $1/lib/pkgconfig/boost.pc` = "1.76.0"
}

uab_pkg_tarball() {
    test $wanna_boost_ver = 105700 && echo boost_1_57_0.tar.bz2
    test $wanna_boost_ver = 107600 && echo boost_1_76_0.tar.bz2
    true
}

uab_sha1sum() {
    test $wanna_boost_ver = 105700 && echo e151557ae47afd1b43dc3fac46f8b04a8fe51c12
    test $wanna_boost_ver = 107600 && echo 8064156508312dde1d834fec3dca9b11006555b6
    true
}

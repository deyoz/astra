#!/bin/bash -e

function uab_config_and_build() {

prefix=${1:?no prefix parameter}

if echo $LOCALCXX | fgrep -w clang++ ; then
    toolset="clang"
elif echo $LOCALCXX | fgrep -w g++ ; then
    toolset="gcc"
fi

userconfigjam=~/user-config.jam
[ -f $userconfigjam ] && die 1 "$userconfigjam is already here - another $0 is running? If not, remove the file and try again."

if [ -n "$toolset" ]; then
    for tok in $LOCALCXX; do
        if [ -z "$b2_options" ] && which $tok && $tok -dumpversion; then
            cxx=`which $tok`
            cxx_version=`$cxx -dumpversion`
            tls_version=`$toolset -dumpversion`
            if [ "$cxx_version" != "$tls_version" ]; then
                cat <<EOF > $userconfigjam
using $toolset : $cxx_version : $cxx ;
EOF
                if [ -n "$cxx_version" ]; then
                    b2_options="toolset=$toolset-$cxx_version"
                fi
            fi
        elif [ "${tok:0:1}" = "-" ]; then
            cxxflags="$cxxflags cxxflags=$tok"
        fi
    done
    if [ -z "$b2_options" ]; then
        b2_options="toolset=$toolset"
    fi
fi

# pyconfig.h compile error fix
python_include_path=`pkg-config python2 --cflags-only-I 2>/dev/null | awk '{print $1}'`

if [ "$ENABLE_GLIBCXX_DEBUG" = "1" ]; then
    glibcxxdebug='cxxflags=-D_GLIBCXX_DEBUG'
fi
cxxflags="$cxxflags cxxflags=-std=c++11 cxxflags=-DBOOST_NO_CXX11_SCOPED_ENUMS cxxflags=-DBOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS"
if [ -n "$python_include_path" ]; then
    cxxflags="$cxxflags cxxflags='$python_include_path'"
fi
if [ "$SIRENA_USE_LIBCXX" = "1" ]; then
    cxxflags="$cxxflags cxxflags=-stdlib=libc++ linkflags=-stdlib=libc++  linkflags=-lcxxrt"
fi

if [ "$PLATFORM" = "m64" ]; then
    address_model='address-model=64'
elif [ "$PLATFORM" = "m32" ]; then
    address_model='address-model=32'
fi

b2_options="$b2_options link=shared threading=multi runtime-link=shared --without-graph_parallel --without-mpi --without-context --without-coroutine --without-signals --without-python --without-wave --layout=system"
cd $prefix/src

sed -i 's/print sys\.prefix/print(sys.prefix)/' ./bootstrap.sh
./bootstrap.sh --prefix=$prefix --with-toolset=$toolset
b2_build_flags="$cxxflags $glibcxxdebug $address_model $b2_options --build_dir=/tmp/boost_build/$prefix --prefix=$prefix -j${MAKE_J:-3}"
echo "./b2 $b2_build_flags clean" > build.clean
echo "./b2 $b2_build_flags stage" > build.stage
./b2 $b2_build_flags stage
./b2 $b2_build_flags install
echo "./bootstrap.sh --prefix=$prefix --with-toolset=$toolset
./b2 $b2_build_flags install" > build.previous
rm -f $userconfigjam

}

function uab_check_version() {
    grep -w 'BOOST_VERSION[ ]\+105900' $1/include/boost/version.hpp
}

function uab_pkg_tarball() {
    echo boost_1_59_0.tar.bz2
}


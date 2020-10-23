#!/bin/bash
set -euo pipefail

die() {
    code=$1
    shift 1
    if [ $code -ne 0 ] ; then printf %s "${@+$@$'\n'}" 1>&2
                         else printf %s "${@+$@$'\n'}"
    fi
    exit $code;
}

pkg=${1:?need a package name as the first parameter}
source `dirname $0`/update_and_build_$pkg.sh

p=${2:?need a path to reside as the 1st parameter}
prefix=$( (stat $p > /dev/null || mkdir -p $p) && cd $p && pwd )
pkg_src="$prefix/src"

if [ -d $prefix/include ] && [ -d $prefix/lib ] && uab_check_version $prefix ; then
    die 0 "$0 $pkg : already built";
fi
rm -rf $prefix/include $prefix/lib $prefix/src

ftp=ftp://storage.komtex/externallibs_astra
pkg_tgz=$(uab_pkg_tarball)
pkg_uri=$ftp/$pkg_tgz

uab_checksum() {
    ! declare -F uab_sha1sum || ! which sha1sum &> /dev/null || echo "$(uab_sha1sum)  $prefix/$pkg_tgz" | sha1sum --check
}

[[ -z "${LOUD:-}" ]] && quiet='--no-verbose'
if ! [ -f $prefix/$pkg_tgz ] || ! uab_checksum $prefix/$pkg_tgz ; then
    if ! wget --timestamping $quiet --directory-prefix=$prefix $pkg_uri ; then
        stat $prefix/$pkg_tgz > /dev/null
    fi
    uab_checksum "$prefix/$pkg_tgz"
fi
[ -d $pkg_src ] || mkdir -p $pkg_src
compression=`file --dereference $prefix/$pkg_tgz | cut -f2 -d\ `
tar --$compression -xf $prefix/$pkg_tgz --strip-components=1 --directory $pkg_src

if [ -n "${PLATFORM:-}" ] ; then
    export CXX="${LOCALCXX:?LOCALCXX is not set} -$PLATFORM"
    export CC="${LOCALCC?:LOCALCC is not set} -$PLATFORM"
else
    export CXX=${LOCALCXX:?LOCALCXX is not set}
    export CC=${LOCALCC?:LOCALCC is not set}
fi
export CFLAGS=${MY_LOCAL_CFLAGS:-}
CXXFLAGS=${MY_LOCAL_CFLAGS:-}
LDFLAGS=${MY_LOCAL_LDFLAGS:-}

if [ "${SIRENA_USE_LIBCXX:-}" = "1" ]; then
    CXXFLAGS+=" -stdlib=libc++"
    LDFLAGS+=" -stdlib=libc++ -lc++abi"
    [ "${ENABLE_GLIBCXX_DEBUG:-}" = "1" ] && CXXFLAGS+=' -D_LIBCPP_DEBUG=1'
else
    [ "${ENABLE_GLIBCXX_DEBUG:-}" = "1" ] && CXXFLAGS+=' -D_GLIBCXX_DEBUG'
fi
export CXXFLAGS
export LDFLAGS

configureac_opts=''
[[ ${SIRENA_ENABLE_SHARED:-} == 1 ]] && configureac_opts='--enable-shared --disable-static'
[[ ${SIRENA_ENABLE_SHARED:-} == 0 ]] && configureac_opts='--enable-static --disable-shared'

uab_chk_pc() {
    prefix=${1:?prefix as the 1st parameter}
    version=${3:?version str as the 3rd parameter}
    pkgname=${2:?package name as the 2nd parameter}
    grep -F $version $prefix/lib/pkgconfig/$pkgname.pc && return 0
    test -d $prefix/lib/pkgconfig || mkdir -p $prefix/lib/pkgconfig
    shift 3
    script="C.create_pc('$pkgname', '$prefix', $1, Version='$version');"
    shift
    ( cd $prefix/lib && PYTHONPATH=$SIRENA_HOME/bin:${PYTHONPATH:-} python -c "import create_pkgconfig as C; $@ $script" )
}

shift 2
cd $pkg_src
uab_config_and_build $prefix $configureac_opts "$@"
uab_check_version $prefix

#!/bin/bash -e

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

if [ -z "$LOUD" ] ; then quiet='--no-verbose' ; fi
if ! wget --timestamping $quiet --directory-prefix=$prefix $pkg_uri ; then
    stat $prefix/$pkg_tgz > /dev/null
fi

[ -d $pkg_src ] || mkdir -p $pkg_src
compression=`file --dereference $prefix/$pkg_tgz | cut -f2 -d\ `
tar --$compression -xf $prefix/$pkg_tgz --strip-components=1 --directory $pkg_src

if [ -n "$PLATFORM" ] ; then
    export CXX="${LOCALCXX:?LOCALCXX is not set} -$PLATFORM"
    export CC="${LOCALCC?:LOCALCC is not set} -$PLATFORM"
else
    export CXX=${LOCALCXX:?LOCALCXX is not set}
    export CC=${LOCALCC?:LOCALCC is not set}
fi
export CFLAGS=$MY_LOCAL_CFLAGS
export LDFLAGS=$MY_LOCAL_LDFLAGS

shift 2
cd $pkg_src
uab_config_and_build $prefix $@


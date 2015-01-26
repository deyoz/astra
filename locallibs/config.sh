#!/bin/bash -e
if [ "$BUILD_TESTS" = "1" ]; then
    export XP_TESTING=1
fi
export MAKE_J=${MAKE_J:-3}
export LIBROOT=$(pwd)
cat <<EOF >env_file
export XP_TESTING=$XP_TESTING
export MAKE_J=$MAKE_J
export LIBROOT=$LIBROOT
export ENABLE_SHARED=${ENABLE_SHARED:-1}
export ENABLE_JMS=${ENABLE_JMS}
export BOOST_LIBS_SUFFIX=$BOOST_LIBS_SUFFIX
export MY_LOCAL_CFLAGS="${MY_LOCAL_CFLAGS}"
if [ -f external_env_file ]; then . external_env_file; fi
EOF

. ./env_file

WITH_PARAMS="--silent"
if [ -n "$ORACLE_INSTANT" ]; then
  WITH_PARAMS="--with-instant-client=yes --with-oracle-includes=$ORACLE_INSTANT/sdk/include --with-oracle-libraries=$ORACLE_INSTANT --with-oci-version=11G"
fi

if [ -n "$UNIT_CHECK" ]; then
  WITH_PARAMS="$WITH_PARAMS --with-check=$UNIT_CHECK"
fi

if [ -n "$BOOST" ]; then
  WITH_PARAMS="$WITH_PARAMS --with-boost=$BOOST"
fi

if [ -n "$BOOST_LIB" ]; then
  WITH_PARAMS="$WITH_PARAMS --with-boost-libdir=$BOOST_LIB"
fi

if [ -n "$ENABLE_GLIBCXX_DEBUG" ]; then
  WITH_PARAMS="$WITH_PARAMS --enable-glibcxx-debug"
fi

if [ "${ENABLE_SHARED}" = "1" ]; then
    #WITH_PARAMS="-shared ${WITH_PARAMS}"
    echo not turning shared on
fi

CONFIG_CACHE_FILE=`pwd`/config.cache
rm -rf $CONFIG_CACHE_FILE
for i in $@; do
    if [ -f $i/Makefile ] ; then
    	(cd $i && make distclean) || echo 'make distclean failed. Probably not configured before.'
    fi
    (cd $i && ./Config -f $WITH_PARAMS --cache-file=$CONFIG_CACHE_FILE)  || exit 1
done

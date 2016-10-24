#!/bin/bash
export BUILD_TESTS=${BUILD_TESTS:? BUILD_TESTS not set}
export ENABLE_SHARED=${ENABLE_SHARED:? ENABLE_SHARED not set}
export ASTRA_HOME=$(pwd)
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$(pwd)/pkgconfig"
export MAKE_J=${MAKE_J:-`grep -c ^processor /proc/cpuinfo`}

if [ -z "$LOCALCXX" ]; then
    if [ -z "$CXX" ]; then
        VERSION=$(gcc -dumpversion)
        SHORT_V=$(gcc -dumpversion | cut -d. -f-2)
        if which gcc-$VERSION; then
            CC="gcc-$VERSION"
            CXX="g++-$VERSION"
        elif which gcc-$SHORT_V; then
            CC="gcc-$SHORT_V"
            CXX="g++-$SHORT_V"
        elif which clang; then
            CC="clang"
            CXX="clang++"
        else
            CC="gcc"
            CXX="g++"
        fi
    fi
    if which ccache; then
        if [ -z "$CCACHE_DIR" ]; then
            export CCACHE_DIR="/tmp/$(pwd | tr \/ _)"
        fi
        CXX="ccache $CXX"
        CC="ccache $CC"
    fi
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${ORACLE_HOME}/lib"

    export LOCALCC=${CC}
    export LOCALCXX=${CXX}
else
    CXX=$LOCALCXX
fi
if echo $CXX | fgrep -w clang ; then
    export MY_LOCAL_CFLAGS="-pipe -Wno-mismatched-tags -Wno-overloaded-virtual -Wno-invalid-source-encoding -Qunused-arguments -fstack-protector -Wstack-protector -foptimize-sibling-calls -Werror-unknown-warning-option ${MY_LOCAL_CFLAGS}"
    # ubsan is broken in 3.2 ... -fsanitize=undefined 
elif [ $($CXX -dumpversion | sed 's/\./0/' | sed 's/\..*//') -ge 408 ]; then
    export MY_LOCAL_CFLAGS="-Wno-unused-local-typedefs $MY_LOCAL_CFLAGS"
fi
#if [ "$BUILD_TESTS" == "1" ]; then
#    export MY_LOCAL_CFLAGS="-DXP_TESTING ${MY_LOCAL_CFLAGS}"
#fi

#. ./bin/config_bases.sh


# Hack
if [ -f ./bin/box_hack.sh ] ; then
./bin/box_hack.sh -need_hack
#. ./bin/box_hack.sh
  if [ `./bin/box_hack.sh -need_hack | grep -c 'YES' ` -eq 1 ] ; then
    ./bin/box_hack.sh -hacking
    exit;
  fi
fi
# end hack

set_cxx11="1"
buildboost="1"


EXTERNALLIBS_DIR=${SIRENA_EXTERNALS:-$(pwd)/externallibs}



function build_externallib() {
    ./bin/astra_update_and_build.sh $1 $EXTERNALLIBS_DIR/$1
    checkresult build_$1 $?
    export PKG_CONFIG_PATH=$EXTERNALLIBS_DIR/$1/lib/pkgconfig:$PKG_CONFIG_PATH
    export LD_LIBRARY_PATH=$EXTERNALLIBS_DIR/$1/lib:$LD_LIBRARY_PATH
    echo "export PKG_CONFIG_PATH=$EXTERNALLIBS_DIR/$1/lib/pkgconfig:\$PKG_CONFIG_PATH" >> locallibs/external_env_file
    echo "export LD_LIBRARY_PATH=$EXTERNALLIBS_DIR/$1/lib:\$LD_LIBRARY_PATH" >> locallibs/external_env_file
}


usage_no_exit()
{
    echo "Usage: `basename $0` CONNECT_STRING [options]"
    echo -e "Options:
    --build_external_libs
    --configlibs
    --buildlibs
    --configastra
    --buildastra
    --createtcl
    --createdb
    --runtests
    --quiet
    --help (-h)"
}

usage()
{
    usage_no_exit
    exit 1
}
checkresult()
{
    if [ $2 -ne 0 ]; then
        echo "$1 failed $2"
        exit 1
    fi
}

if [ $# -eq 0 ]; then
    usage
elif [ $# -eq 1 ]; then
    if [ "$1" = "--help" ] || [ "$1" = "-h" ] ; then
      usage
    fi
    export readonly CONNECT_STRING=$1
    build_external_libs="1"
    configlibs="2"
    buildlibs="1"
    configastra="1"
    buildastra="1"
    createtcl="1"
    createdb="1"
    runtests="1"
    quiet="0"
elif [ $# -eq 2 ] && ( [ "$1" = "--quiet" ] || [ "$2" = "--quiet" ] ); then
    if [ "$1" != "--quiet" ]; then
        export readonly CONNECT_STRING=$1
    elif [ "$2" != "--quiet" ]; then
        export readonly CONNECT_STRING=$2
    fi
    build_external_libs="1"
    configlibs="2"
    buildlibs="1"
    configastra="1"
    buildastra="1"
    createtcl="1"
    createdb="1"
    runtests="1"
    quiet="1"
else
    export CONNECT_STRING=$1
    shift
    for opt in $@; do
        case $opt in
        "-h") usage
            ;;
        "--help") usage
            ;;
        "--build_external_libs") build_external_libs="1"
       	    ;;
        "--configlibs") configlibs="2"
            ;;
        "--buildlibs") buildlibs="1"
            ;;
        "--configastra") configastra="1"
            ;;
        "--buildastra") buildastra="1"
            ;;
        "--createtcl") createtcl="1"
            ;;
        "--createdb") createdb="1";
            ;;
        "--runtests") runtests="1"
            ;;
        "--quiet")  quiet="1"
            ;;
        "--makeiface")  usage_no_exit
           # compatibility
            exit 0
            ;;
        *)  usage
            ;;
        esac
    done
fi

#echo MAKE_J=${MAKE_J} TEST_J=${TEST_J}
if [ `expr match "$CONNECT_STRING" "^-.*\$"` -ne 0 ]; then
    echo "invalid CONNECT_STRING='$CONNECT_STRING'"
    exit 1
fi

if [ "$quiet" = "1" ]; then db_out_stream="/dev/null"; else db_out_stream="/dev/stdout"; fi
if [ "$quiet" = "1" ]; then make_silent="-s"; else make_silent=""; fi

if [ "$build_external_libs" = "1" ]; then
    cat <<EOF > locallibs/external_env_file
if [ -n "\$BOOST_LIB" ] || [ -n "\$BOOST_LIBS_SUFFIX" ] || [ -n "\$PION_LIB" ] ; then echo "unset BOOST_LIB BOOST_LIBS_SUFFIX PION_LIB, then $0 --build_external_libs" 1>&2; exit 2; fi
export readonly CPP_STD_VERSION="c++11"
export readonly BOOST=$EXTERNALLIBS_DIR/boost
export MY_LOCAL_CFLAGS="\$MY_LOCAL_CFLAGS -DBOOST_NO_CXX11_SCOPED_ENUMS -DBOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS -DBOOST_FILESYSTEM_DEPRECATED -DBOOST_NO_AUTO_PTR"
EOF
    build_externallib icu
    build_externallib libxml2
    build_externallib libxslt
    build_externallib boost
    build_externallib check
    build_externallib pion
fi

if [ "$configlibs" = "2" ]; then
    [ "$BUILD_TESTS" = 1 ]
    echo SIRENA_LIBCHECK_BASE=$SIRENA_LIBCHECK_BASE
    export MY_LOCAL_CFLAGS="-O2 $MY_LOCAL_CFLAGS"
    (cd locallibs && make ${make_silent} config clean)
    checkresult configlibs $?
fi
if [ "$buildlibs" = "1" ]; then
    find locallibs -name unit_check.h -exec rm {} \;
    (cd locallibs && make ${make_silent} clean && make ${make_silent} -j ${MAKE_J:-3} all)
    checkresult buildlibs $?
fi
if [ "$configastra" = "1" ]; then
    sh ./bin/config_astra.sh locallibs
    checkresult configastra $?
fi
if [ "$buildastra" = "1" ]; then
    cd src
    find . -name deps -exec rm {} \;
    find . -name unit_check.h -exec rm {} \;
    make ${make_silent} -j3 clean
    #make depend
    time make ${make_silent} -j${MAKE_J:-4}
    checkresult buildastra $?
    cd -
fi
if [ "$createtcl" = "1" ]; then
    (cd src && LIBROOT="$ASTRA_HOME/locallibs" \
        XP_TESTING_FILES="$ASTRA_HOME/src/tests" \
        XP_TESTING_FILES_SERVERLIB="$ASTRA_HOME/locallibs/serverlib/src/testdata" \
        OURNAME='ASTRA' \
        ./create_local_tcl.sh)
    checkresult createtcl $?
fi
if [ "$createdb" = "1" ]; then
    bin/createdb.sh ${TEST_J:-1}
    checkresult bin/createdb.sh $?
fi
if [ "$runtests" = "1" ]; then
    (cd src && time XP_CUTLOGGING=0 ASTRA_SRC=$ASTRA_HOME/src make xp-tests)
    testsresult=$?
    if [ ! -d "src/xplogs" ]; then
        mkdir src/xplogs
    fi
    if [ -f "src/xplogs/xp-tests.log.bfs" ]; then
        cp src/xplogs/xp-tests.log.bfs src/xplogs/xp-tests.log.prev_bfs
    fi
    if [ -f "src/xp-tests.log" ]; then
        if [ -n "${XP_BACKUP_ALL_XP_LOGS}" ]; then
            cp src/xp-tests.log src/xplogs/xp-tests.log.`date -r src/xp-tests.log +%Y%m%dT%H%M%S`
        fi
        cp src/xp-tests.log src/xplogs/xp-tests.log.bfs
    fi
    checkresult runtests $testsresult
fi


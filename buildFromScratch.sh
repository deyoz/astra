#!/bin/bash

export BUILD_TESTS=${BUILD_TESTS:? BUILD_TESTS not set}
export ENABLE_SHARED=${ENABLE_SHARED:? ENABLE_SHARED not set}
export ASTRA_HOME=$(pwd)
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$(pwd)/pkgconfig"
export MAKE_J=${MAKE_J:-`grep -c ^processor /proc/cpuinfo`}
export LOAD_A=${LOAD_A:-`grep -c ^processor /proc/cpuinfo`}
#export PLATFORM='m32'
export CPP_STD_VERSION='c++14'

EXTERNALLIBS_DIR=${EXTERNAL_LIBS:-$(pwd)/externallibs}
export EXTLIB_ENV_FILE=${EXTERNALLIBS_DIR}/external_env_file

# Libs in build order
LIST_EXTLIB="icu libxml2 libxslt boost check pion amqpcpp"

LOCALLIBS_DIR=${LOCAL_LIBS:-$(pwd)/locallibs}

echo LOCALLBS_DIR=$LOCALLIBS_DIR
source ./toolchain/env
echo asss

export CFLAGS="$ASTRA_FLAGS $CFLAGS"
export CXXFLAGS="$ASTRA_FLAGS $CXXFLAGS"

########################  COMPILE CACHE  ########################

if [ -z "$CXX" ]; then
    CC="gcc"
    CXX="g++"
fi

if which ccache; then

    if [ -z "$CCACHE_DIR" ]; then
        export CCACHE_DIR="/tmp/$(pwd | tr \/ _)"
    fi

    CXX="ccache $CXX"
    CC="ccache $CC"
fi


########################  ORACLE  ##############################

#export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${ORACLE_HOME}/lib"

function getOracleRPATH() {
    if [[ -n $ORACLE_LIB ]]; then
        ora_rpath=$ORACLE_LIB
    elif [[ -n $ORACLE_INSTANT ]]; then
        ora_rpath=$ORACLE_INSTANT
    elif [[ -n $ORACLE_HOME ]]; then
        ora_rpath="$ORACLE_HOME/lib"
    fi;

    set_result ${ora_rpath:+"-Wl,-rpath=$ora_rpath"}
}

########################  MESPRO  ###############################

if [[ -n "$WITH_MESPRO" ]] ; then
    MESPRO_HOME="$EXTERNALLIBS_DIR/mespro"

    export MESPRO_CFLAGS="-DUSE_MESPRO -I$MESPRO_HOME/include"
    export MESPRO_CXXFLAGS="-DUSE_MESPRO -I$MESPRO_HOME/include"
	export MESPRO_LDFLAGS="-L$MESPRO_HOME -lmesprox"
    
    if [[ -n $EMBEDDED_RPATH ]] ; then
         MESPRO_LDFLAGS="$MESPRO_LDFLAGS -Wl,-rpath=$MESPRO_HOME"
    fi;
fi;

#################### Debug & Optimization #######################

function termDebug() {
    if [[ -z $DEBUG ]] ; then
        set_result '-g1'
    else
        set_result "$DEBUG"
    fi
}

function termOptimization() {
    local default='-O2'

    if [ -z "$DEBUG" ] ; then
        set_result "${OPTIMIZE:-$default}";
    else
        # В дебаге используем только явно заданные флаги оптимизации
        set_result "$OPTIMIZE";
    fi
}

export DEBUG_FLAGS=$(termDebug)
export OPTIMIZE_FLAGS=$(termOptimization);

####################### C++ STD ################################

function defineGCCRelevantFlags()
{
	GCC_VERSION=$(getGCCVersionInt)
    
	if [[ "$GCC_VERSION" -lt 403 ]] ; then
		USE_CPP_STD='c++98'
	fi;

	if [[ "$GCC_VERSION" -ge 403 && "$GCC_VERSION" -lt 407 ]] ; then
		USE_CPP_STD='c++0x'
	fi;

	if [[ "$GCC_VERSION" -ge 408 && "$GCC_VERSION" -lt 504 ]] ; then
		USE_CPP_STD='c++11'
	fi;

	if [[ "$GCC_VERSION" -ge 601 && "$GCC_VERSION" -le 700 ]] ; then
		USE_CPP_STD='c++14'
	fi;

	export readonly CPP_STD_VERSION=${CPP_STD_VERSION:-$USE_CPP_STD};

    if [[ "$CPP_STD_VERSION" = 'c++11' ]] || [[ "$CPP_STD_VERSION" = 'c++14' ]]  ; then
        WARNINGS="-Wno-unused-local-typedefs"
    fi

    export CFLAGS="$WARNINGS $CFLAGS"
    export CXXFLAGS="${CPP_STD_VERSION:+-std=$CPP_STD_VERSION} $WARNINGS $CXXFLAGS"
}

defineGCCRelevantFlags;

##############################  Final Flags  ###############################

function extRPATH() {
    set_result "-Wl,-rpath=$EXTERNALLIBS_DIR/$1/lib"
}

function externallibs_rpath() {
    local result;
    for lib in $LIST_EXTLIB; do
        result="$result $(extRPATH $lib)"
    done;

    set_result $result
}

function extLibLDFlag() {
    local result;

    if [[ -n $EMBEDDED_RPATH ]] ; then
        case $1 in
            icu)
                result="$(extRPATH icu)";
            ;;
            libxml2)
            ;;
            libxslt)
                result="$(extRPATH libxml2)";
            ;;
            boost)
                result="$(extRPATH boost)";
            ;;
            check)
            ;;
            pion)
                result="$(extRPATH boost)";
            ;;
            amqpcpp)
                result="$(extRPATH amqpcpp)";
            ;;
        esac;
    fi;

    set_result $result;
}

function defineCompileFlags() {
	export CFLAGS="$DEBUG_FLAGS $OPTIMIZE_FLAGS $CFLAGS"
    export CXXFLAGS="$DEBUG_FLAGS $OPTIMIZE_FLAGS $CXXFLAGS"

    # гарантируем единственный вызов
    define_empty_func defineCompileFlags;
}

function getDependRPATH() {
    if [[ -n $EMBEDDED_RPATH ]] ; then
		set_result "$(externallibs_rpath) $(getOracleRPATH)"
	fi
}

defineCompileFlags;

function checksum_SHA1() {
    sha1sum $1 2> /dev/null;
}

function reportChecksum() {
    echo "SHA1 checksum: $(checksum_SHA1 $1)"
}

############################################################################

#if [ "$BUILD_TESTS" == "1" ]; then
#    export CXXFLAGS="-DXP_TESTING ${CXXFLAGS}"
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

function build_externallib() {
    echo "build_externallibs $1"

	local libname=$1
	local libpath=$EXTERNALLIBS_DIR/$libname

    LDFLAGS="$(extLibLDFlag $libname) $LDFLAGS" ./bin/astra_update_and_build.sh $libname $libpath
    checkresult build_$libname $?

    export PKG_CONFIG_PATH=$libpath/lib/pkgconfig:$PKG_CONFIG_PATH
    export LD_LIBRARY_PATH=$libpath/lib:$LD_LIBRARY_PATH
    
    echo "export PKG_CONFIG_PATH=$libpath/lib/pkgconfig:\$PKG_CONFIG_PATH" >> $EXTLIB_ENV_FILE
    echo "export LD_LIBRARY_PATH=$libpath/lib:\$LD_LIBRARY_PATH" >> $EXTLIB_ENV_FILE
}

function create_pkgconfig_amqpcpp() {
    EXTERNALLIBS_DIR=$EXTERNALLIBS_DIR python ./bin/create_pkgconfig_amqpcpp.py
    checkresult configext $?
    echo "create_pkgconfig - ok"
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
    cat <<EOF > $EXTLIB_ENV_FILE
if [ -n "\$BOOST_LIB" ] || [ -n "\$BOOST_LIBS_SUFFIX" ] || [ -n "\$PION_LIB" ] ; then echo "unset BOOST_LIB BOOST_LIBS_SUFFIX PION_LIB, then $0 --build_external_libs" 1>&2; exit 2; fi
export readonly BOOST=$EXTERNALLIBS_DIR/boost
export readonly LIBXML2=$EXTERNALLIBS_DIR/libxml2
export readonly LIBCHECK=$EXTERNALLIBS_DIR/check
export EXTERNAL_CXXFLAGS="-DBOOST_NO_CXX11_SCOPED_ENUMS -DBOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS -DBOOST_FILESYSTEM_DEPRECATED -DBOOST_NO_AUTO_PTR"
EOF
    build_externallib icu
    build_externallib libxml2
    build_externallib libxslt
    build_externallib boost
    build_externallib check
    build_externallib pion
    build_externallib amqpcpp
    create_pkgconfig_amqpcpp
fi

if [ "$configlibs" = "2" ]; then
    (cd $LOCALLIBS_DIR && LDFLAGS="$(getDependRPATH) $LDFLAGS" make ${make_silent} config clean)
    checkresult configlibs $?
fi
if [ "$buildlibs" = "1" ]; then
    find $LOCALLIBS_DIR -name unit_check.h -exec rm {} \;
    (cd $LOCALLIBS_DIR && make ${make_silent} clean && make ${make_silent} -j ${MAKE_J:-3} all)
    checkresult buildlibs $?
fi
if [ "$configastra" = "1" ]; then
    LDFLAGS="$(getDependRPATH) $LDFLAGS" sh ./bin/config_astra.sh $LOCALLIBS_DIR
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
    
    reportChecksum ./astra
    cd -
fi
if [ "$createtcl" = "1" ]; then
    (cd src && LIBROOT="$LOCALLIBS_DIR" \
        XP_TESTING_FILES="$ASTRA_HOME/src/tests" \
        XP_TESTING_FILES_SERVERLIB="${LOCALLIBS_DIR}/serverlib/src/testdata" \
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


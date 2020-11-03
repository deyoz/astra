#/bin/sh
###VERSION=11G
VERSION=12
PREFIX=$1
if [ "x$PREFIX" = "x" ]
then
PREFIX="$HOME/local"
else
shift
fi

if [ "$BUILD_WITHOUT_ORACLE" = 1 ] ; then
    extraopts=( "--without-oracle")
else
INSTANCE=${ic_path:-"$PREFIX/instantclient"}
if [ -d "$INSTANCE" ] ; then
    extraopts=("--with-oci-version=$VERSION" "--with-instant-client=$INSTANCE")
fi
fi

./configure --prefix=$PREFIX --with-boost=$PREFIX "${extraopts[@]}" "$@"




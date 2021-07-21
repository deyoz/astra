#!/bin/bash
set -e

uab_check_version() {
    grep '^Version: 1.0.2u$' $1/lib/pkgconfig/openssl.pc
}

uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1
    ./config --prefix=$prefix -fPIC "$@"
    make -j${MAKE_J:-3}
    make test
    make install
}

uab_pkg_tarball() {
    echo openssl_1_0_2u.tar.gz
}

uab_sha1sum() {
    echo 88c69471746cc1b97a7a688df45b8d211aad750c
}

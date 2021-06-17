FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN echo "deb http://security.ubuntu.com/ubuntu bionic-security main" >> /etc/apt/sources.list && \
    apt update \
    && apt install -yqq gcc-8 g++-8 alien wget subversion libtool libssl1.0-dev tcl \
                        tcl-dev tk tk-dev pkg-config gettext libpq-dev distcc python2.7 \
                        python cmake libpng-dev libaio1 libaio-dev libtool-bin \
                        libxml2-dev postgresql-client postgresql-server-dev-all \
                        libbz2-dev

ENV BUILD_TESTS=1 \
	ENABLE_SHARED=1 \
	BUILD_TESTS=1 \
    ENABLE_ORACLE=0 \
	CPP_STD_VERSION=c++17 \
        XP_NO_RECHECK=1 \
        XP_LIST_EXCLUDE=SqlUtil,Serverlib,httpsrv,httpsrv_ext,ssim \
        PLATFORM=m64 \
        MY_LOCAL_CFLAGS="-g2 -O0" \
        LOCALCC=gcc-8 LOCALCXX=g++-8 \
        PG_HOST=${PG_HOST:-localhost} \
        PG_SYSPAROL=postgres://etick_test:etick@$PG_HOST \
        TZ=Europe/Moscow

RUN env && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN mkdir -p /opt/astra/externallibs && mkdir /opt/astra/locallibs
COPY bin /opt/astra/bin
COPY buildFromScratch.sh /opt/astra
WORKDIR /opt/astra

RUN echo "nameserver 10.1.90.138" > /etc/resolv.conf \
    && echo "search komtex sirena-travel.ru" >> /etc/resolv.conf \
    && ./buildFromScratch.sh astra_docker/astra@oracle1.komtex/build --build_external_libs

COPY . /opt/astra/

RUN echo "nameserver 10.1.90.138" > /etc/resolv.conf \
    && echo "search komtex sirena-travel.ru" >> /etc/resolv.conf \
    && ./buildFromScratch.sh astra_docker/astra@oracle1.komtex/build --configlibs --buildlibs --configastra --buildastra --createtcl

# RUN echo "nameserver 10.1.90.138" > /etc/resolv.conf \
#     && echo "search komtex sirena-travel.ru" >> /etc/resolv.conf \
#     && SYSPAROL=system/nonstop@oracle1.komtex/build ./buildFromScratch.sh astra_docker/astra@oracle1.komtex/build --createdb --runtests

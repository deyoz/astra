FROM ubuntu:18.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -yqq gcc g++ build-essential alien wget subversion libtool libssl-dev tcl8.6-dev tcl pkg-config gettext joe libpq-dev distcc python2.7 \
                        python cmake libpng-dev libaio1 libaio-dev libtool-bin \
                        libxml2-dev postgresql-client \
    && wget https://download.oracle.com/otn_software/linux/instantclient/195000/oracle-instantclient19.5-sqlplus-19.5.0.0.0-1.x86_64.rpm \
    && wget https://download.oracle.com/otn_software/linux/instantclient/195000/oracle-instantclient19.5-basic-19.5.0.0.0-1.x86_64.rpm \
    && wget https://download.oracle.com/otn_software/linux/instantclient/195000/oracle-instantclient19.5-devel-19.5.0.0.0-1.x86_64.rpm \
    && wget https://download.oracle.com/otn_software/linux/instantclient/195000/oracle-instantclient19.5-tools-19.5.0.0.0-1.x86_64.rpm \
    && alien -i oracle-instantclient19.5-basic-19.5.0.0.0-1.x86_64.rpm \
    && alien -i oracle-instantclient19.5-sqlplus-19.5.0.0.0-1.x86_64.rpm \
    && alien -i oracle-instantclient19.5-devel-19.5.0.0.0-1.x86_64.rpm \
    && alien -i oracle-instantclient19.5-tools-19.5.0.0.0-1.x86_64.rpm
ENV BUILD_TESTS=1 \
	ENABLE_SHARED=1 \
	BUILD_TESTS=1 \
	CPP_STD_VERSION=c++14 \
        XP_NO_RECHECK=1 \
        XP_LIST_EXCLUDE=SqlUtil,Serverlib,httpsrv,httpsrv_ext,ssim \
        PLATFORM=m64 \
        MY_LOCAL_CFLAGS="-g2 -O0" \
        LOCALCFLAGS="-I /usr/include/oracle/19.5/client64" \
        NLS_LANG=AMERICAN_CIS.RU8PC866 \
        ORACLE_HOME=/usr/lib/oracle/19.5/client64 \
        ORACLE_OCI_VERSION=12G
        LD_LIBRARY_PATH=/usr/lib/oracle/19.5/client64/lib \
        LOCALCC=gcc-7 LOCALCXX=g++-7 \
        PG_HOST=${PG_HOST:-localhost} \ 
        PG_SYSPAROL=postgres://etick_test:etick@$PG_HOST \
        TZ=Europe/Moscow

RUN env && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt install -yqq libghc-bzlib-dev libssl1.0-dev

RUN mkdir /opt/astra

WORKDIR /opt/astra

COPY . /opt/astra/

RUN echo "nameserver 10.1.9.138" > /etc/resolv.conf \
    && echo "search komtex sirena-travel.ru" >> /etc/resolv.conf \
    && ./buildFromScratch.sh astra_docker/astra@oracle1.komtex/build --build_external_libs --configlibs --buildlibs --configastra --buildastra --createtcl

RUN echo "nameserver 10.1.9.138" > /etc/resolv.conf \
    && echo "search komtex sirena-travel.ru" >> /etc/resolv.conf \
    && SYSPAROL=system/nonstop@oracle1.komtex/build ./buildFromScratch.sh astra_docker/astra@oracle1.komtex/build --createdb --runtests

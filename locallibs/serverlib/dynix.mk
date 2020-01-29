OP_CFLAGS= -DNOT_LINUX -s 

INCLUDE=-I$(ORACLE_HOME)/rdbms/demo -I/usr/local/include 

SHELL=/bin/sh

CC=cc

AWKK=gawk

PROC=$(ORACLE_HOME)/bin/proc

LIBHOME=$(ORACLE_HOME)/lib

LDFLAGS=-L$(LIBHOME) -L/usr/local/lib

PROFLAGS=ireclen=132 oreclen=132 lines=yes mode=oracle \
 release_cursor=yes maxopencursors=20 parse=no

PROLDLIBS= -lsql -lsqlnet -lncr -lsqlnet -lclient -lcommon -lgeneric \
-lsqlnet -lncr -lsqlnet -lclient -lcommon -lgeneric -lepc -lepcpt -lnlsrtl3 \
-lc3v6 -lcore3 -lnlsrtl3 -lcore3 -lnlsrtl3 /usr/lib/abi-socket/libsocket.so \
-lmalloc -lnsl -lseq -lm -lelf

NSL=-lnsl

LIBSOCKET=-lsocket

FINAL_LIBS=$(PROLDLIBS) $(LIBSOCKET)

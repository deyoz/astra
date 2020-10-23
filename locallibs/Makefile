libs=serverlib libtimatic tscriptc jxtlib edilib eticklib libtlg libtypeb shared libssim libjms libcoretypes libnsi

.PHONY: all clean tests $(libs)

all: $(libs)

$(libs):
	./build.sh $@

config:
	./config.sh $(libs)

clean:
	./clean.sh $(libs)

tests:
	./tests.sh $(libs)

rebuild: clean all

serverlib:
libtimatic: serverlib
tscriptc: serverlib
jxtlib: serverlib
edilib: serverlib
shared: serverlib
eticklib: serverlib edilib
libtlg: serverlib edilib
libtypeb: serverlib eticklib
libjms: serverlib
libnsi: serverlib
libcoretypes: serverlib libnsi
libssim: libnsi libcoretypes

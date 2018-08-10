#libs=libssim
#libs=serverlib tscriptc jxtlib edilib eticklib libtlg libtypeb shared libcoretypes
libs=serverlib tscriptc jxtlib edilib eticklib libtlg libtypeb shared libssim libjms libcoretypes libnsi
#libs=edilib
#edilib eticklib libtlg libtypeb shared 

.PHONY: all clean tests $(libs)

all: $(libs)

$(libs):
	$(MAKE) -C $@

config:
	./config.sh $(libs)

clean:
	./clean.sh $(libs)

tests:
	./tests.sh $(libs)

rebuild: clean all

tscriptc edilib jxtlib libtypeb shared libjms: serverlib
eticklib: edilib serverlib
libtlg: edilib serverlib

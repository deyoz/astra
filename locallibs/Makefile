libs=serverlib tscriptc jxtlib edilib eticklib libtlg libtypeb shared 

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

tscriptc edilib jxtlib shared libtypeb: serverlib
eticklib: edilib serverlib
libtlg: edilib serverlib

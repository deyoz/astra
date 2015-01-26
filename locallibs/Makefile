libs=serverlib tscriptc jxtlib edilib eticklib libtlg shared 

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

tscriptc edilib jxtlib shared: serverlib
eticklib: edilib serverlib
libtlg: edilib serverlib

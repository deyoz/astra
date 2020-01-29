.PHONY: all clean cleanall distclean tests

CXX=$(LOCALCXX)

TARGET = tscriptc
CPP_OPTIONS = $(CXXFLAGS) -std=$(CPP_STD_VERSION)
CPP_COMPILE = $(CXX) $(CPP_OPTIONS) -o $@ -c $<
FLEX = $(wildcard /usr/bin/flex)
BISON = $(wildcard /usr/bin/bison)

PARSER = tsc_parser
LEXER = tsc_lexer
OBJS = $(PARSER).tab.o $(LEXER).tab.o tsc_main.o tsc_ast.o\
	   tsc_gen_source.o tsc_gen_vmcode.o\
	   tsc_macro_expand.o tsc_macro_embedded.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CPP_OPTIONS) -o $@ $+

$(PARSER).tab.o: $(PARSER).tab.cc
	$(CPP_COMPILE)
$(LEXER).tab.o: $(LEXER).tab.cc
	$(CPP_COMPILE)
tsc_main.o: tsc_main.cc tsc_ast.h
	$(CPP_COMPILE)
tsc_ast.o: tsc_ast.cc tsc_ast.h
	$(CPP_COMPILE)
tsc_gen_source.o: tsc_gen_source.cc tsc_ast.h
	$(CPP_COMPILE)
tsc_gen_vmcode.o: tsc_gen_vmcode.cc tsc_ast.h
	$(CPP_COMPILE)
tsc_macro_expand.o: tsc_macro_expand.cc tsc_macro.h tsc_ast.h
	$(CPP_COMPILE)
tsc_macro_embedded.o: tsc_macro_embedded.cc tsc_macro.h tsc_ast.h
	$(CPP_COMPILE)

ifneq "$(TSCRIPTC_COMPILE_LEX_YACC)" ""
ifneq "$(BISON)" ""
$(PARSER).tab.cc: tsc_parser.y tsc_ast.h
	$(BISON) -v -Wall -ptsc_yy --defines=$(PARSER).tab.h -o $@ $<
endif

ifneq "$(FLEX)" ""
$(LEXER).tab.cc: tsc_lexer.l $(PARSER).tab.cc
	$(FLEX) -Ptsc_yy -o $(LEXER).tab.cc $<
endif
endif

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)
	rm -f $(PARSER).output

cleanall: clean
	rm -f $(PARSER).tab.cc
	rm -f $(PARSER).tab.h
	rm -f $(LEXER).tab.cc

distclean: clean
tests: all
maintainer-clean:

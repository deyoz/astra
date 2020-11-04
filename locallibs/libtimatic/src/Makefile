.PHONY: all tests clean cleanall distclean showvars lock unlock

#------------------------------------------------

ifneq (,$(wildcard ../include.mk))
include ../include.mk
endif

SRCDIR = $(shell pwd)

TARGETLIB = libtimatic.a
TARGETAPP = testapp

OBJS = timatic_base.o       \
       timatic_exception.o  \
       timatic_http.o       \
       timatic_request.o    \
       timatic_response.o   \
       timatic_service.o    \
       timatic_xml.o

#------------------------------------------------

ifeq ($(LIBTIM_XPTEST),1)
all: $(TARGETLIB) $(TARGETAPP)
else
all: $(TARGETLIB)
endif

$(TARGETLIB): $(OBJS)
	ar rcs $@ $+
	@mkdir -p $(SRCDIR)/../lib
	@ln -sf $(SRCDIR)/$(TARGETLIB) $(SRCDIR)/../lib/$(TARGETLIB)

%.o: %.cc
	$(CXX) $(LIBTIM_CFLAGS) -fPIC -c $< -o $@ $(LIBTIM_LFLAGS) -lpthread -lrt


ifeq ($(LIBTIM_XPTEST),1)
tests: $(TARGETAPP)
	@XP_TESTING=1 LC_ALL=C ./$(TARGETAPP) ./app.tcl -nosir -run_tests

$(TARGETAPP): $(TARGETLIB)
	$(CXX) -I$(SRCDIR) $(LIBTIM_CFLAGS) -o $@ main.cc -L$(SRCDIR) -ltimatic $(LIBTIM_LFLAGS) -lpthread -lrt
endif


clean:
	@rm -f $(TARGETLIB)
	@rm -f $(TARGETAPP)
	@rm -f ../lib/$(TARGETLIB)
	@find $(SRCDIR) -type f -name '*.o' -delete
	@find $(SRCDIR) -type f -name '*.a' -delete
	@find $(SRCDIR) -type f -name '*.log' -delete

cleanall: clean
distclean: clean


showvars:
	@echo 'LIBTIM_XPTEST=$(LIBTIM_XPTEST)'
	@echo 'LIBTIM_SHARED=$(LIBTIM_SHARED)'
	@echo 'LIBTIM_CFLAGS=$(LIBTIM_CFLAGS)'
	@echo 'LIBTIM_LFLAGS=$(LIBTIM_LFLAGS)'
	@echo 'LIBTIM_LIBROOT=$(LIBTIM_LIBROOT)'

lock:
	@echo 'Nothing to be done for "lock"'

unlock:
	@echo 'Nothing to be done for "unlock"'

.NOTPARALLEL:
all-local:
	if [ -f .libs/libetick.a ]; then \
	cp .libs/libetick.a $(top_srcdir)/lib/libetick.a; \
	fi

clean-local:
	-rm -f $(top_srcdir)/lib/libetick.a

include $(top_srcdir)/single.mk

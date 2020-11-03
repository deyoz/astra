.NOTPARALLEL: 
all-local:
	@if [ -f .libs/libtypeb.a ]; then \
		cp -f .libs/libtypeb.a $(top_srcdir)/lib/libtypeb.a; \
	fi

clean-local:
	-rm -f $(top_srcdir)/lib/libtypeb.a

include $(top_srcdir)/single.mk

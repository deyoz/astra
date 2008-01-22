.NOTPARALLEL:
all:
	grep -l -E "(checkunit.h|initstatic.h)" *.c *.cc \
	| LIB_STATIC_INIT_NAME=$(LIB_STATIC_INIT_NAME) \
	$(SERVERLIB_PATH)/make_static_init_name | sort >register.cc1
	diff register.cc register.cc1 >/dev/null 2>&1 || \
        mv register.cc1 register.cc


.NOTPARALLEL:
.PHONY: editypes

SysTarg: editypes
editypes:
	cp $(ASTRA_SRC)/src/tlg/astra_msg_types.dat $(EDILIB_PATH)/
	$(MAKE) -C $(EDILIB_PATH) astra_msg_types.etp

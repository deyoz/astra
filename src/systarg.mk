.NOTPARALLEL:
.PHONY: editypes

SysTarg: editypes
editypes:
	cp $(ASTRA_SRC)/src/tlg/astra_msg_types.dat $(EDILIB_PATH)/; cd $(EDILIB_PATH)/ && \
	make astra_msg_types.etp; 

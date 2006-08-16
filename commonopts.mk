SAVECC:=$(CC)
SAVECXX:=$(CXX)
include $(SRCHOME)/astra_oracle.mk
include $(TCL_MON_PATH)/tclconfig
CC:=$(SAVECC)
CXX:=$(SAVECXX)	
OTHERINCLUDES=$(LIBXML2INCLUDE) $(TCLINCLUDE) -I$(TCL_MON_PATH)  \
-I$(BOOST) -I$(SERVERLIB_PATH) -I$(JXTLIB_PATH) -I$(SHARED_PATH) -I$(ETICKLIB_PATH)/include
COMPFLAGS2=-g2 $(TCLMODE) $(EDILIBMODE) $(EDIINC) $(INCLUDE) -I$(SRCHOME) \
$(ORA_CFLAGS) $(AIR) $(XML) $(OTHERINCLUDES) $(WITH_ZLIB) $(OTHERDEF) \
$(HAS_AIO)
COMPFLAGS=$(COMPFLAGS2)	$(LOCALFLAGS)
CPPCOMPFLAGS=$(CPPINCLUDE) 


include(ts/macro.ts)

# meta: suite iatci

#########################################################################################
# ü1
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ŻŽ¤ŁŽâŽ˘Ş  ŕĽŠá 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ŻŕŽá Ş áâŕĽ
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++7+1027+$(yymmdd)0530+$(yymmdd)0940++"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# ŻŽčńŤ § ŻŕŽá ˘  ­  áŹĽ­ă áâ âăá  ¨§  áâŕë
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:CK"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# Žâ˘Ľâ Žâ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


# Žâ˘Ľâ Žâ áâŕë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD...
PSI++TKNE::29824018416891"
PAP+:::240785++P:400522509::::050225:::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü2

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ŻŕŽá Ş áâŕĽ
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++7+1027+$(yymmdd)0530+$(yymmdd)0940++"
PPD+REPIN+M++IVAN++S71027"
PRD+Y"
PSD++1A"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# ŻŽčńŤ § ŻŕŽá ˘  ­  áŹĽ­ă áâ âăá  ¨§  áâŕë
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:CK"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


# TODO §¤Ľáě ­ăŚ­Ž áíŹăŤ¨ŕŽ˘ âě â ŠŹ ăâ Žâ˘Ľâ  
$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


# Žâ˘Ľâ Žâ áâŕë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+F"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000660001"



# Ľéń § ŻŕŽá Ş áâŕĽ
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000670001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00067"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++7+1027+$(yymmdd)0530+$(yymmdd)0940++"
PPD+REPIN+M++IVAN++S71027"
PRD+Y"
PSD++1A"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000670001"



# ŻŽčńŤ § ŻŕŽá ˘  ­  áŹĽ­ă áâ âăá  ¨§  áâŕë
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:CK"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# Žâ˘Ľâ Žâ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000670001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00067"
FDR++103+$(yymmdd)++++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD...
PSI++TKNE::29824018416891"
PAP+:::240785++P:400522509::::050225:::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000670001"



# ŽâŹĽ­  ŕĽŁ¨áâŕ ć¨¨ ˘ áâŕă
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++T"
PPD+REPIN+M++IVAN++S71027"
UNT+5+1"
UNZ+1+ASTRA000680001"


# ŻŽčŤ¨ ˘  ŽâŞ âë˘ âě áâ âăá ŞăŻŽ­ 
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:I"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# Žâ˘Ľâ Žâ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:I::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


# Žâ˘Ľâ ­  ŽâŹĽ­ă ¨§ áâŕë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR++103+$(yymmdd)++++T"
RAD+X+P"
UNT+4+1"
UNZ+1+ASTRA000680001"


# Ľéń Ž¤­  ŽâŹĽ­  ŕĽŁ¨áâŕ ć¨¨ ˘ áâŕă
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000690001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00069"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++T"
PPD+REPIN+M++IVAN++S71027"
UNT+5+1"
UNZ+1+ASTRA000690001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000690001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00069"
FDR++103+$(yymmdd)++++T"
RAD+X+F"
ERD+1:193:PASSENGER SURNAME NOT CHECKED-IN"
UNT+5+1"
UNZ+1+ASTRA000690001"


%%
#########################################################################################
# ü3

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)

# PLF ¤Ž ŕĽŁ¨áâŕ ć¨¨
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000700001+++O"
UNH+1+DCQPLF:96:2:IA+ASTRA00070"
LOR+UT:SVO"
FDQ++103+$(yymmdd)++++T"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+ASTRA000700001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000700001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00070"
FDR++103+$(yymmdd)++++T"
RAD+P+F"
ERD+1:193:PASSENGER SURNAME NOT CHECKED-IN"
UNT+5+1"
UNZ+1+ASTRA000700001"


# ŕĽŁ¨áâŕ ć¨ď
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++7+1027+$(yymmdd)0530+$(yymmdd)0940++"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# ŻŽčńŤ § ŻŕŽá ˘  ­  áŹĽ­ă áâ âăá  ¨§  áâŕë
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:CK"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# Žâ˘Ľâ Žâ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


# Žâ˘Ľâ Žâ áâŕë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD...
PSI++TKNE::29824018416891"
PAP+:::240785++P:400522509::::050225:::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000660001"


# PLF ŻŽáŤĽ ŕĽŁ¨áâŕ ć¨¨
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000710001+++O"
UNH+1+DCQPLF:96:2:IA+ASTRA00071"
LOR+UT:SVO"
FDQ++103+$(yymmdd)++++T"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+ASTRA000710001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000710001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00071"
FDR++103+$(yymmdd)++++T"
RAD+P+O"
PPD+REPIN+A++IVAN"
PFD+1A+:"
PSI++TKNE::29824018416891"
PAP+:::240785++P:400522509::::050225:::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000710001"

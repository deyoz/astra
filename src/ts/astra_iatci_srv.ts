include(ts/macro.ts)

# meta: suite iatci

$(defmacro ETS_COS_EXCHANGE
    tickno
    cpnno
    status
    pult=
{

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+:++++Y+::RU+$(pult)"
EQN+1:TD"
TKT+$(tickno):T"
CPN+$(cpnno):$(status)"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# ŪâĒĨâ Ūâ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+$(tickno):T::3"
CPN+$(cpnno):$(status)::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

}) #end-of-macro

#########################################################################################
# ü1
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ŊŪĪĢŪâŪĒŠ  āĨĐá 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ŊāŪá Š áâāĨ
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

$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)

# ŪâĒĨâ Ūâ áâāë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD...
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
PAP+:::240785:::TJK++P:400522509:TJK:::050225:M::::::REPIN:IVAN"
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


# § ŊāŪá Š áâāĨ
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


# ŊŪčņŦ § ŊāŪá Ē  ­  áŽĨ­ã áâ âãá  Ļ§  áâāë
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


# TODO §ĪĨáė ­ãĶ­Ū áíŽãŦĻāŪĒ âė â ĐŽ ãâ ŪâĒĨâ  
$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


# ŪâĒĨâ Ūâ áâāë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+F"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000660001"



# Ĩéņ § ŊāŪá Š áâāĨ
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



# ŊŪčņŦ § ŊāŪá Ē  ­  áŽĨ­ã áâ âãá  Ļ§  áâāë
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

# ŪâĒĨâ Ūâ 
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
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
PAP+:::240785:::TJK++P:400522509:TJK:::050225:M::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000670001"



# ŪâŽĨ­  āĨĢĻáâā æĻĻ Ē áâāã
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++T"
PPD+REPIN+M++IVAN++S71027"
UNT+5+1"
UNZ+1+ASTRA000680001"


# ŊŪčŦĻ Ē  ŪâŠ âëĒ âė áâ âãá ŠãŊŪ­ 
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

# ŪâĒĨâ Ūâ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:I::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


# ŪâĒĨâ ­  ŪâŽĨ­ã Ļ§ áâāë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR++103+$(yymmdd)++++T"
RAD+X+P"
UNT+4+1"
UNZ+1+ASTRA000680001"


# Ĩéņ ŪĪ­  ŪâŽĨ­  āĨĢĻáâā æĻĻ Ē áâāã
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

# PLF ĪŪ āĨĢĻáâā æĻĻ
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


# āĨĢĻáâā æĻï
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

$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)

# ŪâĒĨâ Ūâ áâāë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD...
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
PAP+:::240785:::TJK++P:400522509:TJK:::050225:M::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000660001"


# PLF ŊŪáŦĨ āĨĢĻáâā æĻĻ
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
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
PAP+:::240785:::TJK++P:400522509:TJK:::050225:M::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000710001"


%%
#########################################################################################
# ü4
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ŊŪĪĢŪâŪĒŠ  āĨĐá 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ŊāŪá Š áâāĨ
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


$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)


# ŪâĒĨâ Ūâ áâāë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD...
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
PAP+:::240785:::TJK++P:400522509:TJK:::050225:M::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000660001"


# § ŊāŪá Š áâāĨ
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKU:96:2:IA+ASTRA00066"
LOR+:"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A++IVAN"
USI++A:OTHS::::::OTHS FREE TEXT"
UAP+R+:::010576:::RUS++P:99999999999:USA:::311249:M::::::REPIN:IVAN:ABRAMOVICH"
UNT+6+1"
UNZ+1+ASTRA000660001"


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+U+O"
PPD+REPIN+A++IVAN"
PFD+1A+:"
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509+OTHS::::::OTHS FREE TEXT"
PAP+:::010576:::RUS++P:99999999999:USA:::311249:M::::::REPIN:IVAN:ABRAMOVICH"
UNT+8+1"
UNZ+1+ASTRA000660001"


# Ĩéņ § ŊāŪá Š áâāĨ
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKU:96:2:IA+ASTRA00066"
LOR+:"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A++IVAN"
USI++C:OTHS::::::OTHS FREE TEXT+A:FQTV::::::FQTV UT 12121212112"
UNT+6+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+U+O"
PPD+REPIN+A++IVAN"
PFD+1A+:"
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509+FQTV::::::FQTV UT 12121212112"
PAP+:::010576:::RUS++P:99999999999:USA:::311249:M::::::REPIN:IVAN:ABRAMOVICH"
UNT+8+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü5
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ŊŪĪĢŪâŪĒŠ  āĨĐá 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)

# § ŊāŪá Š áâāĨ
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQSMF:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+7+1027+$(yymmdd)++"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRSMF:96:2:IA+ASTRA00066"
FDR+7+1027+$(yymmdd)++++T"
RAD+S+O"
CBD++3:6+++F++:W+:A+:A+:W"
ROD+3++::K+::K+::K+::K"
ROD+6+++:O++"
UNT+7+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü6
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ŊŪĪĢŪâŪĒŠ  āĨĐá 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)

# § ŊāŪá Š áâāĨ
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++++7+1027+$(yymmdd)0530+$(yymmdd)0940++"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD++3"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)

# ŪâĒĨâ Ūâ áâāë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR++103+$(yymmdd)++++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+3A+:"
PSI++TKNE::29824018416891+TKNE::::::TKNE HK1 2982401841689/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
PAP+:::240785:::TJK++P:400522509:TJK:::050225:M::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000660001"


<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKU:96:2:IA+ASTRA00066"
LOR+:"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A++IVAN"
USD++4"
UNT+6+1"
UNZ+1+ASTRA000660001"

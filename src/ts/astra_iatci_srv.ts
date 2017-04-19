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

# ®â¢¥â ®â 
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

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)

# ®â¢¥â ®â áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü2

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN++S71027"
PRD+Y"
PSD++1A"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# ¯®èñ« § ¯à®á ¢  ­  á¬¥­ã áâ âãá  ¨§  áâàë
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


# TODO §¤¥áì ­ã¦­® áí¬ã«¨à®¢ âì â ©¬ ãâ ®â¢¥â  
$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


# ®â¢¥â ®â áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000660001"



# ¥éñ § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000670001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00067"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN++S71027"
PRD+Y"
PSD++1A"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000670001"



# ¯®èñ« § ¯à®á ¢  ­  á¬¥­ã áâ âãá  ¨§  áâàë
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

# ®â¢¥â ®â 
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
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000670001"



# ®â¬¥­  à¥£¨áâà æ¨¨ ¢ áâàã
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++"
PPD+REPIN+M++IVAN++S71027"
UNT+5+1"
UNZ+1+ASTRA000680001"


# ¯®è«¨ ¢  ®âª âë¢ âì áâ âãá ªã¯®­ 
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

# ®â¢¥â ®â 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:I::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


# ®â¢¥â ­  ®â¬¥­ã ¨§ áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+P"
UNT+4+1"
UNZ+1+ASTRA000680001"


# ¥éñ ®¤­  ®â¬¥­  à¥£¨áâà æ¨¨ ¢ áâàã
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000690001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00069"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED"
PPD+REPIN+M++IVAN++S71027"
UNT+5+1"
UNZ+1+ASTRA000690001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000690001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00069"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+X"
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

# PLF ¤® à¥£¨áâà æ¨¨
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000700001+++O"
UNH+1+DCQPLF:96:2:IA+ASTRA00070"
LOR+UT:SVO"
FDQ++103+$(yymmdd)++"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+ASTRA000700001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000700001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00070"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+P+X"
ERD+1:193:PASSENGER SURNAME NOT CHECKED-IN"
UNT+5+1"
UNZ+1+ASTRA000700001"


# à¥£¨áâà æ¨ï
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)

# ®â¢¥â ®â áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"


# PLF ¯®á«¥ à¥£¨áâà æ¨¨
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000710001+++O"
UNH+1+DCQPLF:96:2:IA+ASTRA00071"
LOR+UT:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+ASTRA000710001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000710001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00071"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+P+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000710001"


%%
#########################################################################################
# ü4
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)


# ®â¢¥â ®â áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKU:96:2:IA+ASTRA00066"
LOR+UT:DME"
FDQ+UT+103+$(yymmdd)+DME+LED"
PPD+REPIN+A:N++IVAN"
USI++A:OTHS::::::OTHS FREE TEXT"
UAP+R+:::760501:::RUS++P:99999999999:USA:::491231:M::::::REPIN:IVAN:ABRAMOVICH"
UNT+6+1"
UNZ+1+ASTRA000660001"


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+OTHS::::::OTHS FREE TEXT+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:760501:::RUS++P:99999999999:USA:::491231:M::::::REPIN:IVAN:ABRAMOVICH"
UNT+9+1"
UNZ+1+ASTRA000660001"


# ¥éñ § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKU:96:2:IA+ASTRA00066"
LOR+UT:DME"
FDQ+UT+103+$(yymmdd)+DME+LED"
PPD+REPIN+A:N++IVAN"
USI++C:OTHS::::::OTHS FREE TEXT+A:FQTV::::::FQTV UT 12121212112"
UNT+6+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+FQTV:::::: UT 12121212112+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:760501:::RUS++P:99999999999:USA:::491231:M::::::REPIN:IVAN:ABRAMOVICH"
UNT+9+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü5
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)

# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQSMF:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRSMF:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+S+O"
CBD+Y+001:013+++F++A+B:A+C:A+D"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü6
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)

# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD++3"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)

# ®â¢¥â ®â áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"


<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKU:96:2:IA+ASTRA00066"
LOR+UT:DME"
FDQ+UT+103+$(yymmdd)+DME+LED"
PPD+REPIN+A:N++IVAN"
USD++4"
UNT+6+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"

%%
#########################################################################################
# ü7

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)

# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD++3"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)

# ®â¢¥â ®â áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"



<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQBPR:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
UNT+5+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+B+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"



%%
#########################################################################################
# ü8 «¥¢ë© § ¯à®á

$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UU+103+$(yymmdd)+LAL+LOL++OO+1027+$(yymmdd)0530+$(yymmdd)0940+AAA+BBB"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD++3"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UU+103+$(yymmdd)+LAL+LOL++T"
RAD+I+X"
ERD+1:5:INVALID FLIGHT/DATE"
UNT+5+1"
UNZ+1+ASTRA000660001"

# áç¨â ¥¬, çâ® DCRCKA ­¥ ¤®èñ« ¤®  ¤à¥á â , ¨ í¬ã«¨àã¥¬ ¯®áë«ªã ¨¬ IFM
<<
IFMDCS2
.IFMDCS1
IFM
OO1027/$(ddmon +0 en) AAA PART1
-UU103/$(ddmon +0 en) LALLOL
DEL
1REPIN/IVAN
ENDIFM


%%
#########################################################################################
# ü9 host to host


$(init)
$(init_jxt_pult )
$(login)


# § ¯à®á ª áâà¥
<< h2h=V.\VHLG.WA/E11ADCS1/I11HDCS2/P015ZK3\VGYA\$()
UNB+SIRE:1+U6+U6+161019:0436+1"
UNH+1+DCQSMF:03:1:IA+GUZ2TQPGKY0025"
LOR+U6:LCG"
FDQ+U6+537+$(yymmdd)+SAW+MAD"
SRP+Y"
UNT+5+1"
UNZ+1+1"

>>
UNB+SIRE:1+U6+U6+xxxxxx:xxxx+1"
UNH+1+DCRSMF:03:1:IA+GUZ2TQPGKY0025"
FDR+U6+537+$(yymmdd)+SAW+MAD++T"
RAD+S+X"
ERD+1:5:INVALID FLIGHT/DATE"
UNT+5+1"
UNZ+1+1"


%%
#########################################################################################
# ü10
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_2  103    2278   REPIN IVAN)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


$(ETS_COS_EXCHANGE 2982401841689 1 CK SYSTEM)


# ®â¢¥â ®â áâàë
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"


# ­® ®â¢¥â ¯®â¥àï«áï ¨ ­ ¬ ¯à¨á« «¨ IFM DEL
<<
IFMDCS2
.IFMDCS1
IFM
S71027/$(ddmon +0 en) AER PART1
-UT103/$(ddmon +0 en) DMELED
DEL
1REPIN/IVAN
ENDIFM

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

# ®â¢¥â ®â 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:I::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


%%
#########################################################################################
# ü11
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   REPIN IVAN 2982401841689 1
                   PETROV PETR 2982401841612 1)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
PPD+PETROV+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+12+1"
UNZ+1+ASTRA000660001"


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841612:T"
CPN+1:CK"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref 1)0001"

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


# ®¤¨­ ®â¢¥â ¯à¨èñ«, ¤àã£®© § â ©¬ ãâ¨«áï
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


# ®ª â ãá¯¥è­®© á¬¥­ë áâ âãá 
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

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü12
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   REPIN IVAN 2982401841689 1
                   REPIN PETR 2982401841612 1)


$(deny_ets_interactive  103 )

# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++PETR"
PRD+Y"
PFD...
PSI++TKNE::29824018416121+DOCS::::::DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/REPIN/PETR+FOID::::::FOID PPZB400522510+PSPT::::::PSPT HK1 ZB400522510/TJK/24JUL85/REPIN/PETR/M+TKNE::::::TKNE HK1 2982401841612/1"
PAP+A:REPIN:PETR:850724:::TJK++P:400522510:TJK:::250205:M::::::REPIN:PETR"
UNT+9+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü13
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   REPIN IVAN 2982401841689 1
                   REPIN IVAN 2982401841612 1)

# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+X"
ERD+1:1:PASSENGER SURNAME NOT FOUND"
UNT+5+1"
UNZ+1+ASTRA000660001"


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+X"
ERD+1:6:TOO MANY PASSENGERS WITH SAME SURNAME"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü14
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   REPIN IVAN 2982401841689 1
                   PETROV PETR 2982401841612 1)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
PPD+PETROV+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+12+1"
UNZ+1+ASTRA000660001"


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841612:T"
CPN+1:CK"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref 1)0001"

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

# ­¥ ¯®«ãç¨«¨ ­¨ ®¤­®£® ®â¢¥â  ®â 

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü15
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   REPIN IVAN 2982401841689 1
                   PETROV PETR 2982401841612 1)


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
PPD+PETROV+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+12+1"
UNZ+1+ASTRA000660001"


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841612:T"
CPN+1:CK"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref 1)0001"

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


# ®â¢¥â à §
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

# ®â¢¥â ¤¢ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841612:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
PPD+PETROV+A:N++PETR"
PRD+Y"
PFD...
PSI++TKNE::29824018416121+DOCS::::::DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR+FOID::::::FOID PPZB400522510+PSPT::::::PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M+TKNE::::::TKNE HK1 2982401841612/1"
PAP+A:PETROV:PETR:850724:::TJK++P:400522510:TJK:::250205:M::::::PETROV:PETR"
UNT+14+1"
UNZ+1+ASTRA000660001"


# ¥èñ ®¤¨­ § ¯à®á ª áâà¥ ­  à¥£¨áâà æ¨î
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
PPD+PETROV+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+12+1"
UNZ+1+ASTRA000660001"


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+X"
ERD+1:17:PASSENGER SURNAME ALREADY CHECKED IN"
UNT+5+1"
UNZ+1+ASTRA000660001"



# ®â¬¥­  à¥£¨áâà æ¨¨
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
UNT+6+1"
UNZ+1+ASTRA000680001"



# ¯®è«¨ ®âª âë¢ âì áâ âãáë
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841612:T"
CPN+1:I"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref 1)0001"

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


# ­¥ ¯®«ãç¨«¨ ­¨ ®¤­®£® ®â¢¥â  ®â 

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000680001"


# ®â¬¥­  à¥£¨áâà æ¨¨ á "«¥¢ë¬"(­¥§ à¥£¨áâà¨à®¢ ­­ë¬) ¯ áá ¦¨à®¬
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
PPD+BOKOV+A:N++ANTON"
UNT+7+1"
UNZ+1+ASTRA000680001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+X"
ERD+1:193:PASSENGER SURNAME NOT CHECKED-IN"
UNT+5+1"
UNZ+1+ASTRA000680001"



# ®â¬¥­  à¥£¨áâà æ¨¨
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
UNT+6+1"
UNZ+1+ASTRA000680001"


# ¯®è«¨ ®âª âë¢ âì áâ âãáë
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:142"
ORG+:++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841612:T"
CPN+1:I"
TVL+$(ddmmyy)++++103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref 1)0001"

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


# ®â¢¥â à §
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:I::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

# ®â¢¥â ¤¢ 
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841612:T::3"
CPN+1:I::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+P"
UNT+4+1"
UNZ+1+ASTRA000680001"



# ¨ ¥éñ à § ®â¬¥­  à¥£¨áâà æ¨¨
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
UNT+6+1"
UNZ+1+ASTRA000680001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+X"
ERD+1:193:PASSENGER SURNAME NOT CHECKED-IN"
UNT+5+1"
UNZ+1+ASTRA000680001"


%%
#########################################################################################
# ü16
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   REPIN IVAN 2982401841689 1
                   PETROV PETR 2982401841612 1)

$(deny_ets_interactive  103 )


# § ¯à®á ª áâà¥ ­  à¥£¨áâà æ¨î ®¤­®£® ¯ ªá 
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
UNT+9+1"
UNZ+1+ASTRA000660001"


# § ¯à®á ª áâà¥ ­  à¥£¨áâà æ¨î ¢â®à®£® ¯ ªá 
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+PETROV+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+PETROV+A:N++PETR"
PRD+Y"
PFD...
PSI++TKNE::29824018416121+DOCS::::::DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR+FOID::::::FOID PPZB400522510+PSPT::::::PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M+TKNE::::::TKNE HK1 2982401841612/1"
PAP+A:PETROV:PETR:850724:::TJK++P:400522510:TJK:::250205:M::::::PETROV:PETR"
UNT+9+1"
UNZ+1+ASTRA000660001"


# ®â¬¥­  à¥£¨áâà æ¨¨ ®¡®¨å
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
UNT+6+1"
UNZ+1+ASTRA000680001"

# â ª ­¥«ì§ï
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000680001"


# ¯¥ç âì ®¡®¨å
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQBPR:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
UNT+6+1"
UNZ+1+ASTRA000660001"

# â ª â®¦¥ ­¥«ì§ï
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+B+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################
# ü17
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   REPIN IVAN 2982401841689 1
                   PETROV PETR 2982401841612 1)

$(deny_ets_interactive  103 )


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
PPD+PETROV+M++PETR"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+12+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
PPD+PETROV+A:N++PETR"
PRD+Y"
PFD...
PSI++TKNE::29824018416121+DOCS::::::DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR+FOID::::::FOID PPZB400522510+PSPT::::::PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M+TKNE::::::TKNE HK1 2982401841612/1"
PAP+A:PETROV:PETR:850724:::TJK++P:400522510:TJK:::250205:M::::::PETROV:PETR"
UNT+14+1"
UNZ+1+ASTRA000660001"


# ¯¥ç âì ®¡®¨å
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQBPR:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
UNT+6+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+B+O"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PFD...
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+A:REPIN:IVAN:850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN"
PPD+PETROV+A:N++PETR"
PRD+Y"
PFD...
PSI++TKNE::29824018416121+DOCS::::::DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR+FOID::::::FOID PPZB400522510+PSPT::::::PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M+TKNE::::::TKNE HK1 2982401841612/1"
PAP+A:PETROV:PETR:850724:::TJK++P:400522510:TJK:::250205:M::::::PETROV:PETR"
UNT+14+1"
UNZ+1+ASTRA000660001"


# ®â¬¥­  ®¡®¨å
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ++103+$(yymmdd)++"
PPD+REPIN+A:N++IVAN"
PPD+PETROV+A:N++PETR"
UNT+6+1"
UNZ+1+ASTRA000680001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+UT+103+$(yymmdd)+DME+LED++T"
RAD+X+P"
UNT+4+1"
UNZ+1+ASTRA000680001"


%%
#########################################################################################
# ü18
$(init)
$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

# ¯®¤£®â®¢ª  à¥©á 
$(PREPARE_FLIGHT_5  103    2278  
                   ONE CHILD 2982401841689 1
                   PETROV PETR 2982401841612 1)

$(deny_ets_interactive  103 )


# § ¯à®á ª áâà¥
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+U6:SVX:RU"
FDQ+UT+103+$(yymmdd)+DME+LED++S7+1027+$(yymmdd)0530+$(yymmdd)0940+AER+DME"
PPD+ONE+C:N++CHILD++0013949607"
PRD+Y+OK++KKKNKZ"
PSD+N"
PBD+0"
PSI++TKNE::2982401841689"
CRI+PP:12345678"
PAP+A:ONE:CHILD H:080101:::RUS++P:1234566:RUS:::200101:M:701:::::ONE:CHILD H"
UNT+11+1"
UNZ+1+1"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+UT+103+170419+DME+LED++T"
RAD+I+O"
PPD+ONE+C:N++CHILD"
PRD+Y"
PFD+001A+:Y+1"
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/ONE/CHILD+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/ONE/CHILD/M+TKNE::::::TKNE HK1 2982401841689/1"
PAP+C:ONE:CHILD:850724:::TJK++P:400522509:TJK:::250205:M::::::ONE:CHILD"
UNT+9+1"
UNZ+1+ASTRA000660001"

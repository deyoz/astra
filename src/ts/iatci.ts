include(ts/macro.ts)

# meta: suite iatci

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <InitialRequest/>
   </query>
 </term>}


>>
UNB+SIRE:1+xx+xx+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER++UT+100+150220+150220+SVO+LED"
PPD+PETROV+M++ALEX++UT100"
PRD+Y"
PSD+N"
PBD+1:20"
PSI++TKNE::29812121212121+FQTV:UT:121313454::1::I"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150215:2222+$(last_edifact_ref)0001+++O"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+SU+200+1502211300+LED+AER+1502211500+T"
RAD+I+O"
PPD+PETROV+M+SU200+ALEX++UT100"
PFD+05A+N:C:2:::::Y:W+0040"
PSI++TKNE::29812121212121+FQTV:UT:121313454"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick...

$(lastRedisplay)


%%
#########################################################################################

$(init)
$(init_dcs UT TA OA)

<<
UNB+SIRE:1+OA+TA+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+UT:SVO"
FDQ+LH+200+1502170000+LED+AER++UT+555+1502170530+1502171140+SVO+LED"
PPD+PETROV+M+LH200+ALEX++UT555"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+7+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+TA+OA+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+LH+200+1502171000+LED+AER+1502171330+T"
RAD+I+O"
FSD+0930"
PPD+PETROV+M+LH200+ALEX++UT555"
PFD+03A+N:C+0030"
UNT+7+1"
UNZ+1+ASTRA000660001"

%%
#########################################################################################

$(init)
$(init_dcs UT DCS3 DCS2)


# �����_1
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+U6:SVO"
FDQ+SU+200+150217+LED+AER++U6+100+1502170530+1502171140+SVO+LED"
PPD+PETROV+M++ALEX++U6100"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# ��������_1
>>
UNB+SIRE:1+DCS2+DCS3+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+SU"
CHD+U6:SVO++++++++H::SU+H::U6"
FDQ+UT+300+150222+AER+SVO++SU+200+150217++LED+AER"
PPD+PETROV+M++ALEX++SU200"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

# �⢥� �� ��������_1
<<
UNB+SIRE:1+DCS3+DCS2+150217:0747+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+UT+300+1502221500+AER+SVO+1502221945+T"
RAD+I+O"
CHD+++++++++H::UT"
FSD+1430"
PPD+PETROV+M+UT300+ALEX++SU200"
PFD+04A+N:C+0040"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


# �⢥� �� ��ࢮ��砫�� �����_1
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+SU+200+1502171000+LED+AER+1502171330+T"
RAD+I+O"
CHD+++++++++H::SU"
FSD+0930"
PPD+PETROV+M+SU200+ALEX++U6100"
PFD+03A+N:C+0030"
FDR+UT+300+1502221500+AER+SVO+1502221945+T"
RAD+I+O"
CHD+++++++++H::UT"
FSD+1430"
PPD+PETROV+M+UT300+ALEX++SU200"
PFD+04A+N:C+0040"
UNT+14+1"
UNZ+1+ASTRA000660001"

%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <CancelRequest/>
   </query>
 </term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
PPD+IVANOV+M++SERGEI++UT100"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150215:2222+$(last_edifact_ref)0001+++O"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+LH+200+1502211300+LED+AER+1502211500+T"
RAD+X+P"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick...

$(lastRedisplay)

%%
#########################################################################################
######
# �஢�ઠ ��ࠡ�⪨ �訡�� �� 㤠�񭭮� DCS
######

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <InitialRequest/>
   </query>
 </term>}


>>
UNB+SIRE:1+xx+xx+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER++UT+100+1502200530+1502201140+SVO+LED"
PPD+PETROV+M++ALEX++UT100"
PRD+Y"
PSD+N"
PBD+1:20"
PSI++TKNE::29812121212121+FQTV:UT:121313454::1::I"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150215:2222+$(last_edifact_ref)0001+++O"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+SU+200+1502211300+LED+AER+1502211500+T"
RAD+I+X"
ERD+1:102"
FDR+SU+400+1602211300+LED+AER+1602211500+T"
RAD+I+O"
ERD+1:102"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick...

$(lastRedisplay)

%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <InitialRequest/>
   </query>
 </term>}


>>
UNB+SIRE:1+xx+xx+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER++UT+100+1502200530+1502201140+SVO+LED"
PPD+PETROV+M++ALEX++UT100"
PRD+Y"
PSD+N"
PBD+1:20"
PSI++TKNE::29812121212121+FQTV:UT:121313454::1::I"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

# ���� ����� �� ��ࠡ��稪� ⠩���� edifact
>> lines=auto
    <kick...

!!
$(lastRedisplay)


%%
#########################################################################################

$(init)
$(init_dcs UT DCS3 DCS2)


# �����_1
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+U6:SVO"
FDQ+SU+200+150217+LED+AER++U6+100+1502170530+1502171140+SVO+LED"
PPD+PETROV+M++ALEX++U6100"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# ��������_1
>>
UNB+SIRE:1+DCS2+DCS3+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+SU"
CHD+U6:SVO++++++++H::SU+H::U6"
FDQ+UT+300+150222+AER+SVO++SU+200+150217++LED+AER"
PPD+PETROV+M++ALEX++SU200"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+SU+200+150217+LED+AER++T"
RAD+I+F"
ERD+1:196"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <PasslistRequest/>
   </query>
 </term>}


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
SPD+IVANOV:SERGEI:Y:1+05A+++21+RECLOC++++2982145646345"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150215:2222+$(last_edifact_ref)0001+++O"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+SU+200+1502211300+LED+AER+1502211500+T"
RAD+P+O"
PPD+IVANOV+M+SERGEI+ALEX"
PFD+05A+N:C:2:::::Y:W+0040"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick...

$(lastRedisplay)

%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <PasslistRequest/>
   </query>
 </term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
SPD+IVANOV:SERGEI:Y:1+05A+++21+RECLOC++++2982145646345"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


# ���� ����� �� ��ࠡ��稪� ⠩���� edifact
>> lines=auto
    <kick...

!!
$(lastRedisplay)


%%
#########################################################################################

$(init)
$(init_dcs UT TA OA)


<<
UNB+SIRE:1+OA+TA+150217:0747+ASTRA000660001+++O"
UNH+1+DCQPLF:96:2:IA+ASTRA00066"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
SPD+IVANOV:SERGEI:Y:1+05A+++21+RECLOC++++2982145646345"
UNT+5+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+TA+OA+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+SU+200+150221+LED+AER++T"
RAD+P+O"
PPD+IVANOV+A++SERGEI"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################

$(init)
$(init_dcs UT TA OA)


<<
UNB+SIRE:1+OA+TA+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00066"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
PPD+IVANOV+M++SERGEI++UT100"
UNT+5+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+TA+OA+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+SU+200+150221+LED+AER++T"
RAD+X+P"
UNT+4+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <UpdateRequest/>
   </query>
 </term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
PPD+IVANOV+A++SERG"
UPD+R+IVANOV++SERGEI"
USD++15B"
UBD+R:1:20"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150215:2222+$(last_edifact_ref)0001+++O"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+SU+200+1502211300+LED+AER+1502211500+T"
RAD+U+O"
PPD+PETROV+M+SU200+ALEX++UT100"
PFD+05A+N:C:2:::::Y:W+0040"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick...

$(lastRedisplay)


%%
#########################################################################################

$(init)
$(init_dcs UT TA OA)

<<
UNB+SIRE:1+OA+TA+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKU:96:2:IA+ASTRA00066"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
PPD+IVANOV+M++SERGEI++UT100"
USD+N"
UBD+R:1:20"
UNT+7+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+TA+OA+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+SU+200+150221+LED+AER++T"
RAD+U+O"
PPD+IVANOV+M++SERGEI++UT100"
UNT+5+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU RCV SND)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SeatmapRequest/>
   </query>
 </term>}

>>
UNB+SIRE:1+SND+RCV+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQSMF:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER"
SRP+F:N"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+RCV+SND+150217:0747+$(last_edifact_ref)0001+++T"
UNH+1+DCRSMF:96:2:IA+$(last_edifact_ref)"
FDR+SU+200+150221+LED+AER++T"
RAD+S+O"
SRP+F"
EQD++++++D09"
CBD+F+3:6+++F++A:W+B:A+E:A+F:W"
ROD+3++A::K+B::K+E::K+F::K"
ROD+6++A+B:O+E+F"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick...

$(lastRedisplay)


%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <ReprintRequest/>
   </query>
 </term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQBPR:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER++UT+100+1502200530+1502201140+SVO+LED"
PPD+PETROV+M++ALEX++UT100"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150215:2222+$(last_edifact_ref)0001+++O"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+SU+200+1502211300+LED+AER+1502211500+T"
RAD+B+O"
PPD+PETROV+M+SU200+ALEX++UT100"
PFD+05A+N:C:2:::::Y:W+0040"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick...

$(lastRedisplay)


%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_dcs SU TA OA)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='IactiInterface' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <ReprintRequest/>
   </query>
 </term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQBPR:96:2:IA+$(last_edifact_ref)"
LOR+UT:SVO"
FDQ+SU+200+150221+LED+AER++UT+100+1502200530+1502201140+SVO+LED"
PPD+PETROV+M++ALEX++UT100"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# �஢�ਬ, �� ��ࠡ��稪 ⠩���� ��뢠����
$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


>> lines=auto
    <kick...

$(lastRedisplay)

%%
#########################################################################################

$(init)
$(init_dcs UT TA OA)


<<
UNB+SIRE:1+OA+TA+150215:2222+ASTRA000660001+++O"
UNH+1+DCQBPR:96:2:IA+ASTRA00066"
LOR+UT:SVO"
FDQ+SU+255+150221+LED+AER++T"
PPD+PETROV+M++ALEX"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+TA+OA+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+SU+255+1502171000+LED+AER+1502171330+T"
RAD+B+O"
FSD+0930"
PPD+PETROV+M+SU255+ALEX"
PFD+03A+N:C+0030"
UNT+7+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################

$(init)
$(init_dcs UT TA OA)


<<
UNB+SIRE:1+OA+TA+150215:2222+ASTRA000660001+++O"
UNH+1+DCQSMF:96:2:IA+ASTRA00066"
LOR+UT:SVO"
FDQ+SU+560+150221+LED+AER"
SRP+F:N"
UNT+5+1"
UNZ+1+ASTRA000660001"

>>
UNB+SIRE:1+TA+OA+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRSMF:96:2:IA+ASTRA00066"
FDR+SU+560+150221+LED+AER++T"
RAD+S+O"
SRP+F:N"
CBD+F+1:9+++F++A:W+B:A"
CBD+Y+9:29++10:11+F+15:20+A:W+B:A+C:A+D:W"
ROD+1++A:O+B:F+C:F"
ROD+2++A:O+B:F+C:F"
ROD+5++A:O+B:F+C:F"
UNT+10+1"
UNZ+1+ASTRA000660001"

include(ts/macro.ts)

# meta: suite iatci

$(init_jxt_pult МОВРОМ)
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
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150215:2222+$(last_edifact_ref)0001+++O"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+SU+200+1502211300+LED+AER+1502211500+T"
RAD+I+O"
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


# запрос_1
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


# подзапрос_1
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

# ответ на подзапрос_1
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


# ответ на первоначальный запрос_1
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

$(init_jxt_pult МОВРОМ)
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
# Проверка обработки ошибок от удалённой DCS
######

$(init_jxt_pult МОВРОМ)
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
UNT+8+1"
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

$(init_jxt_pult МОВРОМ)
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
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

# пришёл пинок от обработчика таймаута edifact
>> lines=auto
    <kick...

!!
$(lastRedisplay)


%%
#########################################################################################

$(init)
$(init_dcs UT DCS3 DCS2)


# запрос_1
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


# подзапрос_1
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

$(init_jxt_pult МОВРОМ)
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

$(init_jxt_pult МОВРОМ)
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


# пришёл пинок от обработчика таймаута edifact
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

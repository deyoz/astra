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

!!
$(lastRedisplay)


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

!!
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

!!
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
PSI++TKNE::29812121212121+FQTV:UT:121313454::1::I"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

# пришёл пинок от обработчика таймаута edifact
>> lines=auto
    <kick...

!!
$(lastRedisplay)

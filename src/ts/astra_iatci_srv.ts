include(ts/macro.ts)

# meta: suite iatci

$(defmacro INBOUND_PNL
{MOWKB1H
.MOWRMUT 020815
PNL
UT103/28AUG DME PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 DME  LED
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1REPIN/IVAN
.L/0840Z6/UT
.L/09T1B3/1H
.O/SU2278Y28LEDAER2315AR
.R/TKNE HK1 2982401841689/1-1REPIN/IVAN
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN
.RN//IVAN-1REPIN/IVAN
.R/PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M
.RN/-1REPIN/IVAN
.R/FOID PPZB400522509-1REPIN/IVAN
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro


#########################################################################################


$(init)

# следующие 2 строчки нужны для подготовки рейса (scd,spp и т.п.)
$(init_jxt_pult МОВРОМ)
$(login)

# создание сезонного расписания на рейс
$(PREPARE_SEASON_SCD UT DME LED 103)

# генерация суточного плана полёта на дату
$(create_spp 28082015 ddmmyyyy)

# подаём на вход PNL по рейсу
<<
$(INBOUND_PNL)

$(dump_table CRS_PAX)
$(dump_table CRS_TRANSFER)

$(set dep_point_id $(get_dep_point_id ДМД ЮТ 103 150828))
$(dump_table POINTS fields="POINT_ID, AIRP, POINT_NUM, MOVE_ID, PR_DEL, PR_REG, TIME_OUT")
$(create_random_trip_comp $(get dep_point_id) Э)

$(dump_table TRIP_CLASSES)
$(dump_table TRIP_COMP_ELEMS)


# запрос к Астре
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+ЮТ+103+150828+ДМД+ПЛК++С7+1027+1508280530+1508280940+СОЧ+ДМД"
PPD+REPIN+M++IVAN"
PRD+Y"
PSD+N"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# пошёл запрос в СЭБ на смену статуса из астры
>>
UNB+SIRE:1+ASTRA+ETICK+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:CK"
TVL+280815+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# ответ от СЭБ
<<
UNB+SIRE:1+ETICK+ASTRA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

# ответ от Астры
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+ЮТ+103+150828+ДМД+ПЛК++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:Э"
PSI++TKNE::29824018416891"
PAP+:::240785++P:400522509::::050225:::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000660001"


%%
#########################################################################################

$(init)

# следующие 2 строчки нужны для подготовки рейса (scd,spp и т.п.)
$(init_jxt_pult МОВРОМ)
$(login)

# создание сезонного расписания на рейс
$(PREPARE_SEASON_SCD UT DME LED 103)

# генерация суточного плана полёта на дату
$(create_spp 28082015 ddmmyyyy)

# подаём на вход PNL по рейсу
<<
$(INBOUND_PNL)

$(set dep_point_id $(get_dep_point_id ДМД ЮТ 103 150828))
$(create_random_trip_comp $(get dep_point_id) Э)

$(dump_table POINTS fields="POINT_ID, AIRP, POINT_NUM, MOVE_ID, PR_DEL, PR_REG, TIME_OUT")

$(dump_table CRS_TRANSFER)


# запрос к Астре
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000660001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00066"
LOR+S7:SVO"
FDQ+ЮТ+103+150828+ДМД+ПЛК++S7+1027+1508280530+1508280940+ШРМ+ПЛК"
PPD+REPIN+M++IVAN++S71027"
PRD+Y"
PSD++1A"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000660001"


# пошёл запрос в СЭБ на смену статуса из астры
>>
UNB+SIRE:1+ASTRA+ETICK+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:CK"
TVL+280815+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


# TODO здесь нужно сэмулировать таймаут ответа СЭБ

# ответ от СЭБ
<<
UNB+SIRE:1+ETICK+ASTRA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

# ответ от Астры
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000660001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00066"
FDR+ЮТ+103+150828+ДМД+ПЛК++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:Э"
PSI++TKNE::29824018416891"
PAP+:::240785++P:400522509::::050225:::::::REPIN:IVAN"
UNT+8+1"
UNZ+1+ASTRA000660001"


# ещё запрос к Астре
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000670001+++O"
UNH+1+DCQCKI:96:2:IA+ASTRA00067"
LOR+S7:SVO"
FDQ+ЮТ+103+150828+ДМД+ПЛК++S7+1027+1508280530+1508280940+ШРМ+ПЛК"
PPD+REPIN+M++IVAN++S71027"
PRD+Y"
PSD++1А"
PBD+1:20"
UNT+8+1"
UNZ+1+ASTRA000670001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000670001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00067"
FDR+ЮТ+103+150828+ДМД+ПЛК++T"
RAD+I+F"
ERD+1:17:PASSENGER SURNAME ALREADY CHECKED IN"
UNT+5+1"
UNZ+1+ASTRA000670001"


# отмена регистрации в Астру
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000680001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00068"
LOR+S7:SVO"
FDQ+ЮТ+103+150828+ДМД+ПЛК++T"
PPD+REPIN+M++IVAN++S71027"
UNT+5+1"
UNZ+1+ASTRA000680001"


# пошли в СЭБ откатывать статус купона
>>
UNB+SIRE:1+ASTRA+ETICK+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2982401841689:T"
CPN+1:I"
TVL+280815+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# ответ от СЭБ
<<
UNB+SIRE:1+ETICK+ASTRA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2982401841689:T::3"
CPN+1:I::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


# ответ на отмену из Астры
>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000680001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00068"
FDR+ЮТ+103+150828+ДМД+ПЛК++T"
RAD+X+P"
UNT+4+1"
UNZ+1+ASTRA000680001"


# ещё одна отмена регистрации в Астру
<<
UNB+SIRE:1+DCS1+DCS2+150217:0747+ASTRA000690001+++O"
UNH+1+DCQCKX:96:2:IA+ASTRA00069"
LOR+S7:SVO"
FDQ+ЮТ+103+150828+ДМД+ПЛК++T"
PPD+REPIN+M++IVAN++S71027"
UNT+5+1"
UNZ+1+ASTRA000690001"

>>
UNB+SIRE:1+DCS2+DCS1+xxxxxx:xxxx+ASTRA000690001+++T"
UNH+1+DCRCKA:96:2:IA+ASTRA00069"
FDR+ЮТ+103+150828+ДМД+ПЛК++T"
RAD+X+F"
ERD+1:193:PASSENGER SURNAME NOT CHECKED-IN"
UNT+5+1"
UNZ+1+ASTRA000690001"

include(ts/macro.ts)
include(ts/spp/write_dests_macro.ts)

# meta: suite iapi

$(defmacro PREPARE_CN_EXCHANGE_SETTINGS
  airline
{
$(set country_id $(get_elem_id etCountry CN))
$(settcl OWN_CANON_NAME TSTDC)
$(sql

{INSERT INTO rot(canon_name,own_canon_name,ip_address,ip_port,h2h,our_h2h_addr,h2h_addr,h2h_rem_addr_num,resp_timeout,router_translit,loopback,id)
 VALUES('IAPIT','TSTDC','0.0.0.0',8888,1,'1HCNIAPIQ','1HCNIAPIR',1,NULL,NULL,NULL,id__seq.nextval)}
"insert into EDI_ADDRS(ADDR, CANON_NAME) values ('NIAC', 'IAPIT')"
"insert into EDIFACT_PROFILES (NAME, VERSION, SUB_VERSION, CTRL_AGENCY, SYNTAX_NAME, SYNTAX_VER) values ('IAPI', 'D', '05B', 'UN', 'UNOA', 4)"
{INSERT INTO apis_sets(id,airline,country_dep,country_arv,country_control,format,transport_type,transport_params,edi_addr,edi_own_addr,pr_denial)
 VALUES(id__seq.nextval,'$(get_elem_id etAirline $(airline))','$(get country_id)',NULL,'$(get country_id)','IAPI_CN','FILE','apis/IAPI_CN','NIAC','$(get_lat_code awk $(airline))',0)}
{INSERT INTO apis_sets(id,airline,country_dep,country_arv,country_control,format,transport_type,transport_params,edi_addr,edi_own_addr,pr_denial)
 VALUES(id__seq.nextval,'$(get_elem_id etAirline $(airline))',NULL,'$(get country_id)','$(get country_id)','IAPI_CN','FILE','apis/IAPI_CN','NIAC','$(get_lat_code awk $(airline))',0)}
"insert into AIRLINE_OFFICES(ID, AIRLINE, COUNTRY_CONTROL, CONTACT_NAME, PHONE, FAX, TO_APIS) values(id__seq.nextval, '$(get_elem_id etAirline $(airline))', '$(get country_id)', 'SIRENA-TRAVEL', '4959504991', '4959504973', 1)"

)

$(init_iapi_request_id 100)
})

$(defmacro PREPARE_SPP_FLIGHT_SETTINGS
  airline
  airp_dep
{
$(set point_dep $(last_point_id_spp))

$(CHANGE_TRIP_SETS $(get point_dep) pr_free_seating=1 apis_manual_input=1)

$(OPEN_CHECKIN $(get point_dep))

$(sql "INSERT INTO misc_set(id, type, airline, flt_no, airp_dep, pr_misc) VALUES(id__seq.nextval, 11, '$(get_elem_id etAirline $(airline))', NULL, NULL, 1)")
$(sql "INSERT INTO halls2(id, airp, terminal, name, name_lat, rpt_grp, pr_vip) VALUES(777, '$(get_elem_id etAirp $(airp_dep))', NULL, '$(airp_dep)', NULL, NULL, 0)")
})

### Вызываем этот макрос строго после загрузки PNL
$(defmacro PREPARE_SPP_FLIGHT_AFTER_PNL
  airline
  flt_no
  airp_dep
  time_dep  #формат даты: dd.mm.yyyy hh:nn
  time_arv  #формат даты: dd.mm.yyyy hh:nn
  airp_arv
{

$(set_user_time_type LocalAirp PIKE)

$(NEW_SPP_FLIGHT_ONE_LEG $(airline) $(flt_no) "" $(airp_dep) $(time_dep) $(time_arv) $(airp_arv))

$(PREPARE_SPP_FLIGHT_SETTINGS $(airline) $(airp_dep))

})

### Вызываем этот макрос строго после загрузки PNL
$(defmacro PREPARE_SPP_TRANSIT_FLIGHT_AFTER_PNL
  airline
  flt_no
  airp_dep
  time_dep       #формат даты: dd.mm.yyyy hh:nn
  time_arv_trst  #формат даты: dd.mm.yyyy hh:nn
  airp_trst
  time_dep_trst  #формат даты: dd.mm.yyyy hh:nn
  time_arv       #формат даты: dd.mm.yyyy hh:nn
  airp_arv
{

$(set_user_time_type LocalAirp PIKE)

$(NEW_SPP_FLIGHT_TWO_LEGS $(airline) $(flt_no) "" $(airp_dep) $(time_dep) $(time_arv_trst) $(airp_trst) $(time_dep_trst) $(time_arv) $(airp_arv))

$(PREPARE_SPP_FLIGHT_SETTINGS $(airline) $(airp_dep))

})

$(defmacro BOARDING_REQUEST
  point_dep
  pax_id
  hall
{

!! capture=on err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByPaxId>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <pax_id>$(pax_id)</pax_id>
      <boarding>1</boarding>
      <dev_model/>
      <fmt_type/>
    </PaxByPaxId>
  </query>
</term>}

})

$(defmacro OK_TO_BOARD
  point_dep
  pax_id
  hall=777
{

$(BOARDING_REQUEST $(point_dep) $(pax_id) $(hall))

>> lines=auto
      <updated>
        <pax_id>$(pax_id)</pax_id>
      </updated>

})

$(defmacro NO_BOARD
  point_dep
  pax_id
  hall=777
{

$(BOARDING_REQUEST $(point_dep) $(pax_id) $(hall))

$(USER_ERROR_RESPONSE MSG.PASSENGER.APPS_PROBLEM)

})

$(defmacro APIS_INCOMPLETE_ON_BOARDING
  point_dep
  pax_id
  hall=777
{

$(BOARDING_REQUEST $(point_dep) $(pax_id) $(hall))

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.APIS_INCOMPLETE 128)

})

### test 1 - Inbound Single Passenger / Inbound Two Passengers
### case 1001: Chinese-Normal Information-OK TO BOARD
### case 1108: Two Chinese-Normal Information-OK TO BOARD
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep ICN)
$(set time_dep "$(date_format %d.%m.%Y) 16:20")
$(set time_arv "$(date_format %d.%m.%Y) 20:35")
$(set airp_arv CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)002Y
1ONE/TWO WI
.L/MWZRDQ
.R/TKNE HK1 7842836432717/2
.R/DOCS HK1/P/CHN/E6397444822/CHN/20JUN56/M/15DEC$(date_format %y +1y)/ONE/TWO WI
.R/DOCA HK1/R/CHN
1ONE/UBE WI
.L/MWZRCY
.R/TKNE HK1 7842836432722/2
.R/DOCS HK1/P/CHN/E6397444756/CHN/12JAN96/M/09SEP$(date_format %y +1y)/ONE/UBE WI
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id1 $(get_pax_id $(get point_dep) ONE "TWO WI"))
$(set pax_id2 $(get_pax_id $(get point_dep) ONE "UBE WI"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id1)</pax_id>
    <surname>ONE</surname>
    <name>TWO WI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432717</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>20.06.1956 00:00:00</birth_date>
      <expiry_date>15.12.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>TWO WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id2)</pax_id>
    <surname>ONE</surname>
    <name>UBE WI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432722</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444756</no>
      <nationality>CHN</nationality>
      <birth_date>12.01.1996 00:00:00</birth_date>
      <expiry_date>09.09.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>UBE WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++ONE:TWO WI"
ATT+2++M"
DTM+329:560620"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id1)"
RFF+YZY:7842836432717C2"
DOC+P:110:ZZZ+E6397444822"
DTM+36:$(date_format %y +1y)1215"
LOC+91+CHN"
NAD+FL+++ONE:UBE WI"
ATT+2++M"
DTM+329:960112"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRCY"
RFF+ABO:$(get pax_id2)"
RFF+YZY:7842836432722C2"
DOC+P:110:ZZZ+E6397444756"
DTM+36:$(date_format %y +1y)0909"
LOC+91+CHN"
CNT+42:2"
UNT+38+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id1)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRCY"
RFF+ABO:$(get pax_id2)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id2))
$(OK_TO_BOARD $(get point_dep) $(get pax_id1))

%%

### test 2 - Inbound Single Passenger / Inbound Two Passengers
### case 1002: Foreigner-Normal Information-OK TO BOARD
### case 1109: Two Foreigners-Normal Information-OK TO BOARD
#########################################################################################

$(init_term)

$(set airline MU)
$(set flt_no 2599)
$(set airp_dep PNH)
$(set time_dep "$(date_format %d.%m.%Y) 15:05")
$(set time_arv "$(date_format %d.%m.%Y) 16:45")
$(set airp_arv KMG)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)002Y
1ONE/QIAOYAO
.L/MLCKPK
.R/TKNE HK1 7812315763779/1
.R/DOCS HK1/P/CHN/4825552534/KOR/11MAR93/F/30JAN$(date_format %y +1y)/ONE/QIAOYAO
.R/DOCO HK1//V/5576
.R/DOCA HK1/R/CHN
1ONE/QIAOYUN
.L/MLCKPM
.R/TKNE HK1 7812315763809/1
.R/DOCS HK1/P/CHN/4825552122/KOR/11MAR63/F/30JAN$(date_format %y +1y)/ONE/QIAOYUN
.R/DOCO HK1//V/5555
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id1 $(get_pax_id $(get point_dep) ONE QIAOYAO))
$(set pax_id2 $(get_pax_id $(get point_dep) ONE QIAOYUN))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id1)</pax_id>
    <surname>ONE</surname>
    <name>QIAOYAO</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763779</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552534</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1993 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>ONE</surname>
      <first_name>QIAOYAO</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>5576</no>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id2)</pax_id>
    <surname>ONE</surname>
    <name>QIAOYUN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763809</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552122</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>ONE</surname>
      <first_name>QIAOYUN</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>5555</no>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+MU+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+MU+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+MU2599+++MU"
LOC+125+PNH"
DTM+189:$(yymmdd)1505:201"
LOC+87+KMG"
DTM+232:$(yymmdd)1645:201"
NAD+FL+++ONE:QIAOYAO"
ATT+2++F"
DTM+329:930311"
GEI+4+173"
LOC+178+PNH"
LOC+179+KMG"
NAT+2+KOR"
RFF+AVF:MLCKPK"
RFF+ABO:$(get pax_id1)"
RFF+YZY:7812315763779C1"
DOC+P:110:ZZZ+4825552534"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+5576"
NAD+FL+++ONE:QIAOYUN"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+PNH"
LOC+179+KMG"
NAT+2+KOR"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id2)"
RFF+YZY:7812315763809C1"
DOC+P:110:ZZZ+4825552122"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+5555"
CNT+42:2"
UNT+40+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+MU+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+MU+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:MU2599"
DTM+189:$(yymmdd)1505:201"
DTM+232:$(yymmdd)1645:201"
LOC+125+PNH"
LOC+87+KMG"
ERP+2"
RFF+AVF:MLCKPK"
RFF+ABO:$(get pax_id1)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id2)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id1))
$(OK_TO_BOARD $(get point_dep) $(get pax_id2))

%%

### test 3 - Inbound Single Passenger
### case 1003: Change the Information-OK TO BOARD
#########################################################################################

$(init_term)

$(set airline ZH)
$(set flt_no 9073)
$(set airp_dep TPE)
$(set time_dep "$(date_format %d.%m.%Y) 14:20")
$(set time_arv "$(date_format %d.%m.%Y) 16:15")
$(set airp_arv SZX)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)001Y
1ONE/JIA DUNG
.L/MXRLBK
.R/TKNE HK1 9993007667827/2
.R/DOCS HK1/T/CHN/303856640/CHN/07MAR76/M/15NOV$(date_format %y +1y)/ONE/JIA DUNG
.R/DOCA HK1/R/TWN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) ONE "JIA DUNG"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>ONE</surname>
    <name>JIA DUNG</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>9993007667827</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>T</type>
      <issue_country>CHN</issue_country>
      <no>303856640</no>
      <nationality>CHN</nationality>
      <birth_date>07.03.1976 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>JIA DUNG</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>TWN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(dump_table events_bilingual fields="msg" where="lang='EN' AND type='ПАС'" order="ev_order")

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+ZH+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+ZH+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+ZH9073+++ZH"
LOC+125+TPE"
DTM+189:$(yymmdd)1420:201"
LOC+87+SZX"
DTM+232:$(yymmdd)1615:201"
NAD+FL+++ONE:JIA DUNG"
ATT+2++M"
DTM+329:760307"
GEI+4+173"
LOC+178+TPE"
LOC+179+SZX"
NAT+2+CHN"
RFF+AVF:MXRLBK"
RFF+ABO:$(get pax_id)"
RFF+YZY:9993007667827C2"
DOC+T:110:ZZZ+303856640"
DTM+36:$(date_format %y +1y)1115"
LOC+91+CHN"
CNT+42:1"
UNT+25+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+ZH+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+ZH+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:ZH9073"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)1615:201"
LOC+125+TPE"
LOC+87+SZX"
ERP+2"
RFF+AVF:MXRLBK"
RFF+ABO:$(get pax_id)"
ERC+1Z"
FTX+AAP+++NO BOARD, if there is a question, please contact with NIA of China, tel861056095288."
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(set grp_id $(get_single_grp_id $(get point_dep) ONE "JIA DUNG"))
$(set grp_tid $(get_single_tid $(get point_dep) ONE "JIA DUNG"))
$(set pax_tid $(get_single_pax_tid $(get point_dep) ONE "JIA DUNG"))

$(CHANGE_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>ONE</surname>
    <name>JIA DUNG</name>
    <pers_type>ВЗ</pers_type>
    <refuse/>
    <ticket_no>9993007667827</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>T</type>
      <issue_country>CHN</issue_country>
      <no>303856640</no>
      <nationality>CHN</nationality>
      <birth_date>07.03.1976 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +2y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>JIA DUNG</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <bag_pool_num/>
    <subclass>Э</subclass>
    <tid>$(get pax_tid)</tid>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+ZH+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+ZH+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745+CP"
RFF+TN:0000103"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+ZH9073+++ZH"
LOC+125+TPE"
DTM+189:$(yymmdd)1420:201"
LOC+87+SZX"
DTM+232:$(yymmdd)1615:201"
NAD+FL+++ONE:JIA DUNG"
ATT+2++M"
DTM+329:760307"
GEI+4+173"
LOC+178+TPE"
LOC+179+SZX"
NAT+2+CHN"
RFF+AVF:MXRLBK"
RFF+ABO:$(get pax_id)"
RFF+YZY:9993007667827C2"
DOC+T:110:ZZZ+303856640"
DTM+36:$(date_format %y +2y)1115"
LOC+91+CHN"
CNT+42:1"
UNT+25+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+ZH+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+ZH+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000103"
RFF+AF:ZH9073"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)1615:201"
LOC+125+TPE"
LOC+87+SZX"
ERP+2"
RFF+AVF:MXRLBK"
RFF+ABO:$(get pax_id)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id))

%%

### test 4 - Inbound Single Passenger / Inbound Two Passengers
### case 1004: Replace the Name with FNU-OK TO BOARD
### case 1110: Two Passengers-the one is changed the information, the other one replaces his name with FNU - OK TO BOARD
#########################################################################################

$(init_term)

$(set airline MU)
$(set flt_no 2911)
$(set airp_dep TPE)
$(set time_dep "$(date_format %d.%m.%Y) 14:10")
$(set time_arv "$(date_format %d.%m.%Y) 16:15")
$(set airp_arv SZX) #HIA в китайских тестах

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)002Y
1ONE/F
.L/PD1WJW
.R/TKNE HK1 7812315788313/1
.R/DOCS HK1/P/CHN/L11182829/CHN/21JUL57/F/23OCT$(date_format %y +1y)/ONE/F
.R/DOCA HK1/R/CHN
1ONE/JIA DUNG
.L/MXRLBK
.R/TKNE HK1 9993007667827/2
.R/DOCS HK1/T/CHN/303856640/CHN/07MAR76/M/15NOV$(date_format %y +1y)/ONE/JIA DUNG
.R/DOCA HK1/R/TWN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id1 $(get_pax_id $(get point_dep) ONE F))
$(set pax_id2 $(get_pax_id $(get point_dep) ONE "JIA DUNG"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id1)</pax_id>
    <surname>ONE</surname>
    <name>F</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315788313</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>L11182829</no>
      <nationality>CHN</nationality>
      <birth_date>21.07.1957 00:00:00</birth_date>
      <expiry_date>23.10.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>ONE</surname>
      <first_name>FNU</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id2)</pax_id>
    <surname>ONE</surname>
    <name>JIA DUNG</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>9993007667827</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>T</type>
      <issue_country>CHN</issue_country>
      <no>303856640</no>
      <nationality>CHN</nationality>
      <birth_date>07.03.1976 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +2y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>JIA DUNG</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+MU+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+MU+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+MU2911+++MU"
LOC+125+TPE"
DTM+189:$(yymmdd)1410:201"
LOC+87+SZX"
DTM+232:$(yymmdd)1615:201"
NAD+FL+++ONE:FNU"
ATT+2++F"
DTM+329:570721"
GEI+4+173"
LOC+178+TPE"
LOC+179+SZX"
NAT+2+CHN"
RFF+AVF:PD1WJW"
RFF+ABO:$(get pax_id1)"
RFF+YZY:7812315788313C1"
DOC+P:110:ZZZ+L11182829"
DTM+36:$(date_format %y +1y)1023"
LOC+91+CHN"
NAD+FL+++ONE:JIA DUNG"
ATT+2++M"
DTM+329:760307"
GEI+4+173"
LOC+178+TPE"
LOC+179+SZX"
NAT+2+CHN"
RFF+AVF:MXRLBK"
RFF+ABO:$(get pax_id2)"
RFF+YZY:9993007667827C2"
DOC+T:110:ZZZ+303856640"
DTM+36:$(date_format %y +2y)1115"
LOC+91+CHN"
CNT+42:2"
UNT+38+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+MU+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+MU+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:MU2911"
DTM+189:$(yymmdd)1410:201"
DTM+232:$(yymmdd)1615:201"
LOC+125+TPE"
LOC+87+SZX"
ERP+2"
RFF+AVF:PD1WJW"
RFF+ABO:$(get pax_id1)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MXRLBK"
RFF+ABO:$(get pax_id2)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id1))
$(OK_TO_BOARD $(get point_dep) $(get pax_id2))

%%

### test 5
### не введен номер билета - должны ругаться при регистрации
#########################################################################################

$(init_term)

$(set airline ZH)
$(set flt_no 9073)
$(set airp_dep TPE)
$(set time_dep "$(date_format %d.%m.%Y) 14:20")
$(set time_arv "$(date_format %d.%m.%Y) 16:15")
$(set airp_arv SZX)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)001Y
1ONE/JIA DUNG
.L/MXRLBK
.R/DOCS HK1/T/CHN/303856640/CHN/07MAR76/M/15NOV$(date_format %y +1y)/ONE/JIA DUNG
.R/DOCA HK1/R/TWN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) ONE "JIA DUNG"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
capture=on
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>ONE</surname>
    <name>JIA DUNG</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no/>
    <coupon_no/>
    <ticket_rem/>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>T</type>
      <issue_country>CHN</issue_country>
      <no>303856640</no>
      <nationality>CHN</nationality>
      <birth_date>07.03.1976 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>JIA DUNG</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>TWN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
}

)

$(USER_ERROR_RESPONSE MSG.CHECKIN.PASSENGERS_TICKETS_NOT_SET)

%%

### test 6 - Inbound Single Passenger
### case 1038: The AVF Missing-Data Deficient - ABO OR AVF MISSING VALUE
#########################################################################################

$(init_term)

$(set airline CZ)
$(set flt_no 393)
$(set airp_dep KIX)
$(set time_dep "$(date_format %d.%m.%Y) 15:50")
$(set time_arv "$(date_format %d.%m.%Y) 20:30")
$(set airp_arv CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)001Y
1TWO/MAKOTO
.R/TKNE HK1 7842795845347/1
.R/DOCS HK1/P/CHN/TR1804563/JPN/16APR72/M/30MAY$(date_format %y +1y)/TWO/MAKOTO
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TWO MAKOTO))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
capture=on
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>TWO</surname>
    <name>MAKOTO</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842795845347</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>TR1804563</no>
      <nationality>JPN</nationality>
      <birth_date>16.04.1972 00:00:00</birth_date>
      <expiry_date>30.05.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>TWO</surname>
      <first_name>MAKOTO</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
}

)

$(USER_ERROR_RESPONSE MSG.CHECKIN.PNR_ADDR_REQUIRED)

%%

### test 7 - Inbound Single Passenger
### case 1039: Multi-leg flight - inbound - Two foreign terminals - Take off and land on the same day - OK TO BOARD
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep      SSN) #SEL в китайских тестах
$(set time_dep      "$(date_format %d.%m.%Y) 16:20")
$(set time_arv_trst "$(date_format %d.%m.%Y) 20:35")
$(set airp_trst     ICN)
$(set time_dep_trst "$(date_format %d.%m.%Y) 20:50")
$(set time_arv      "$(date_format %d.%m.%Y) 23:35")
$(set airp_arv      CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)001Y
1ONE/TWO WI
.L/MWZRDQ
.R/TKNE HK1 7842836432717/2
.R/DOCS HK1/P/CHN/E6397444822/CHN/20JUN56/M/15DEC$(date_format %y +1y)/ONE/TWO WI
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_TRANSIT_FLIGHT_AFTER_PNL
  $(get airline) $(get flt_no)
  $(get airp_dep) $(get time_dep)
  $(get time_arv_trst) $(get airp_trst) $(get time_dep_trst)
  $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_trst $(get_next_trip_point_id $(get point_dep)))
$(set point_arv $(get_next_trip_point_id $(get point_trst)))
$(set pax_id $(get_pax_id $(get point_dep) ONE "TWO WI"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>ONE</surname>
    <name>TWO WI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432717</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>20.06.1956 00:00:00</birth_date>
      <expiry_date>15.12.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>TWO WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+92+SSN"
DTM+189:$(yymmdd)1620:201"
LOC+92+ICN"
DTM+232:$(yymmdd)2035:201"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)2050:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2335:201"
NAD+FL+++ONE:TWO WI"
ATT+2++M"
DTM+329:560620"
GEI+4+173"
LOC+178+SSN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id)"
RFF+YZY:7842836432717C2"
DOC+P:110:ZZZ+E6397444822"
DTM+36:$(date_format %y +1y)1215"
LOC+91+CHN"
CNT+42:1"
UNT+30+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+92+SSN"
LOC+92+ICN"
RFF+AF:CA339"
DTM+189:$(yymmdd)2050:201"
DTM+232:$(yymmdd)2335:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id))

%%

### test 8 - Inbound Single Passenger / Inbound Two Passengers
### case 1051: Inbound- NO BOARD
### case 1134: Two Passengers- one's information is normal-OK TO BOARD, the other one checks failed-NO BOARD
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep ICN)
$(set time_dep "$(date_format %d.%m.%Y) 16:20")
$(set time_arv "$(date_format %d.%m.%Y) 20:35")
$(set airp_arv CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)002Y
1ONE/TWO WI
.L/MWZRDQ
.R/TKNE HK1 7842836432717/2
.R/DOCS HK1/P/CHN/E63974448/CHN/20JUN56/M/15NOV$(date_format %y +1y)/ONE/TWO WI
.R/DOCA HK1/R/CHN
1ONE/NNN NN
.L/MWZRCQ
.R/TKNE HK1 7842836432722/2
.R/DOCS HK1/P/CHN/X11111111/CHN/01JAN01/M/15NOV$(date_format %y +1y)/ONE/NNN NN
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id1 $(get_pax_id $(get point_dep) ONE "TWO WI"))
$(set pax_id2 $(get_pax_id $(get point_dep) ONE "NNN NN"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id1)</pax_id>
    <surname>ONE</surname>
    <name>TWO WI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432717</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E63974448</no>
      <nationality>CHN</nationality>
      <birth_date>20.06.1956 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>TWO WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id2)</pax_id>
    <surname>ONE</surname>
    <name>NNN NN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432722</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>X11111111</no>
      <nationality>CHN</nationality>
      <birth_date>01.01.2001 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>NNN NN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++ONE:TWO WI"
ATT+2++M"
DTM+329:560620"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id1)"
RFF+YZY:7842836432717C2"
DOC+P:110:ZZZ+E63974448"
DTM+36:$(date_format %y +1y)1115"
LOC+91+CHN"
NAD+FL+++ONE:NNN NN"
ATT+2++M"
DTM+329:010101"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id2)"
RFF+YZY:7842836432722C2"
DOC+P:110:ZZZ+X11111111"
DTM+36:$(date_format %y +1y)1115"
LOC+91+CHN"
CNT+42:2"
UNT+38+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id1)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id2)"
ERC+1Z"
FTX+AAP+++NO BOARD, if there is a question, please contact with NIA of China, tel861056095288."
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(NO_BOARD $(get point_dep) $(get pax_id2))
$(OK_TO_BOARD $(get point_dep) $(get pax_id1))

%%

### test 9 - Inbound Single Passenger
### case 1061: Unsolicited Message to Change the Status-NO BOARD 0Z-1Z
### case 1060: Unsolicited Message to Change the Status-OK TO BOARD 1Z-0Z
#########################################################################################

$(init_term)

$(set airline MU)
$(set flt_no 589)
$(set airp_dep SFO)
$(set time_dep "$(date_format %d.%m.%Y) 14:20")
$(set time_arv "$(date_format %d.%m.%Y) 09:30")
$(set airp_arv PVG)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)001Y
1TWO/SHIMEI
.L/NY7HZZ
.R/TKNE HK1 7817127993397/2
.R/DOCS HK1/T/CHN/TB6236266/CHN/08MAR42/F/14NOV$(date_format %y +1y)/TWO/SHIMEI
.R/DOCA HK1/R/TWN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TWO SHIMEI))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>TWO</surname>
    <name>SHIMEI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7817127993397</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>T</type>
      <issue_country>CHN</issue_country>
      <no>TB6236266</no>
      <nationality>CHN</nationality>
      <birth_date>08.03.1942 00:00:00</birth_date>
      <expiry_date>14.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>TWO</surname>
      <first_name>SHIMEI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+MU+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+MU+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+MU589+++MU"
LOC+125+SFO"
DTM+189:$(yymmdd)1420:201"
LOC+87+PVG"
DTM+232:$(yymmdd)0930:201"
NAD+FL+++TWO:SHIMEI"
ATT+2++F"
DTM+329:420308"
GEI+4+173"
LOC+178+SFO"
LOC+179+PVG"
NAT+2+CHN"
RFF+AVF:NY7HZZ"
RFF+ABO:$(get pax_id)"
RFF+YZY:7817127993397C2"
DOC+T:110:ZZZ+TB6236266"
DTM+36:$(date_format %y +1y)1114"
LOC+91+CHN"
CNT+42:1"
UNT+25+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+MU+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+MU+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:MU589"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)0930:201"
LOC+125+SFO"
LOC+87+PVG"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:$(get pax_id)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+MU+$(yymmdd):$(hhmi)+1569312526531++IAPI"
UNG+CUSRES+NIAC+MU+$(yymmdd):$(hhmi)+15693125265312+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA"
BGM+132"
RFF+TN:1909240821556284716"
RFF+AF:MU589"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)0930:201"
LOC+125+SFO"
LOC+87+PVG"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:$(get pax_id)"
ERC+1Z"
UNT+13+11085B94E1F8FA"
UNE+1+15693125265312"
UNZ+1+1569312526531"

>>
UNB+SIRE:4+MU+NIAC+xxxxxx:xxxx+1569312526531++IAPI"
UNG+CUSRES+MU+NIAC+xxxxxx:xxxx+15693125265312+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN"
BGM+312"
RFF+TN:1909240821556284716"
RFF+AF:MU589"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)0930:201"
LOC+125+SFO"
LOC+87+PVG"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:$(get pax_id)"
ERC+1Z"
UNT+13+11085B94E1F8FA"
UNE+1+15693125265312"
UNZ+1+1569312526531"

$(NO_BOARD $(get point_dep) $(get pax_id))

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+MU+$(yymmdd):$(hhmi)+1569312526531++IAPI"
UNG+CUSRES+NIAC+MU+$(yymmdd):$(hhmi)+15693125265312+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA"
BGM+132"
RFF+TN:1909240821556284716"
RFF+AF:MU589"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)0930:201"
LOC+125+SFO"
LOC+87+PVG"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:$(get pax_id)"
ERC+0Z"
UNT+13+11085B94E1F8FA"
UNE+1+15693125265312"
UNZ+1+1569312526531"

>>
UNB+SIRE:4+MU+NIAC+xxxxxx:xxxx+1569312526531++IAPI"
UNG+CUSRES+MU+NIAC+xxxxxx:xxxx+15693125265312+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN"
BGM+312"
RFF+TN:1909240821556284716"
RFF+AF:MU589"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)0930:201"
LOC+125+SFO"
LOC+87+PVG"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:$(get pax_id)"
ERC+0Z"
UNT+13+11085B94E1F8FA"
UNE+1+15693125265312"
UNZ+1+1569312526531"

$(OK_TO_BOARD $(get point_dep) $(get pax_id))

%%

### test 10 - Out Bound Single Passenger
### case 1103: Outbound-OK TO BOARD
### case 1032: Inbound - Take off and land not on the same day-OK TO BOARD
### не совсем 1032, потому что Outbound вместо Inbound
#########################################################################################

$(init_term)

$(set airline CZ)
$(set flt_no 329)
$(set airp_dep CAN)
$(set time_dep "$(date_format %d.%m.%Y) 14:00")
$(set time_arv "$(date_format %d.%m.%Y +1) 04:00")
$(set airp_arv YVR)


<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)001Y
1TWO/WEIJIE
.L/MDC8D4
.R/TKNE HK1 7844492892669/2
.R/DOCS HK1/P/CHN/G44815193/CHN/15AUG71/M/09AUG$(date_format %y +1y)/TWO/WEIJIE
.R/DOCA HK1/R/CAN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TWO WEIJIE))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>TWO</surname>
    <name>WEIJIE</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7844492892669</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G44815193</no>
      <nationality>CHN</nationality>
      <birth_date>15.08.1971 00:00:00</birth_date>
      <expiry_date>09.08.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>TWO</surname>
      <first_name>WEIJIE</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+CZ+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CZ+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CZ329+++CZ"
LOC+125+CAN"
DTM+189:$(yymmdd)1400:201"
LOC+87+YVR"
DTM+232:$(yymmdd +1)0400:201"
NAD+FL+++TWO:WEIJIE"
ATT+2++M"
DTM+329:710815"
GEI+4+173"
LOC+178+CAN"
LOC+179+YVR"
NAT+2+CHN"
RFF+AVF:MDC8D4"
RFF+ABO:$(get pax_id)"
RFF+YZY:7844492892669C2"
DOC+P:110:ZZZ+G44815193"
DTM+36:$(date_format %y +1y)0809"
LOC+91+CHN"
CNT+42:1"
UNT+25+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CZ+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+CZ+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CZ329"
DTM+189:$(yymmdd)1400:201"
DTM+232:$(yymmdd +1)0400:201"
LOC+125+CAN"
LOC+87+YVR"
ERP+2"
RFF+AVF:MDC8D4"
RFF+ABO:$(get pax_id)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id))

%%

### test 11 - Out Bound Single Passenger
### case 1107: Multi-leg flight - outbound - Two domestic terminals - Take off and land not on the same day - OK TO BOARD
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep      CAN)
$(set time_dep      "$(date_format %d.%m.%Y) 20:50")
$(set time_arv_trst "$(date_format %d.%m.%Y) 21:35")
$(set airp_trst     PEK)
$(set time_dep_trst "$(date_format %d.%m.%Y) 21:50")
$(set time_arv      "$(date_format %d.%m.%Y +1) 04:35")
$(set airp_arv      ICN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)001Y
1ONE/TWO WI
.L/MWZRDQ
.R/TKNE HK1 7842836432717/2
.R/DOCS HK1/P/CHN/E6397444822/CHN/20JUN56/M/15DEC$(date_format %y +1y)/ONE/TWO WI
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_TRANSIT_FLIGHT_AFTER_PNL
  $(get airline) $(get flt_no)
  $(get airp_dep) $(get time_dep)
  $(get time_arv_trst) $(get airp_trst) $(get time_dep_trst)
  $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_trst $(get_next_trip_point_id $(get point_dep)))
$(set point_arv $(get_next_trip_point_id $(get point_trst)))
$(set pax_id $(get_pax_id $(get point_dep) ONE "TWO WI"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id)</pax_id>
    <surname>ONE</surname>
    <name>TWO WI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432717</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>20.06.1956 00:00:00</birth_date>
      <expiry_date>15.12.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>TWO WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+92+CAN"
DTM+189:$(yymmdd)2050:201"
LOC+92+PEK"
DTM+232:$(yymmdd)2135:201"
TDT+20+CA339+++CA"
LOC+125+PEK"
DTM+189:$(yymmdd)2150:201"
LOC+87+ICN"
DTM+232:$(yymmdd +1)0435:201"
NAD+FL+++ONE:TWO WI"
ATT+2++M"
DTM+329:560620"
GEI+4+173"
LOC+178+CAN"
LOC+179+ICN"
NAT+2+CHN"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id)"
RFF+YZY:7842836432717C2"
DOC+P:110:ZZZ+E6397444822"
DTM+36:$(date_format %y +1y)1215"
LOC+91+CHN"
CNT+42:1"
UNT+30+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)2050:201"
DTM+232:$(yymmdd)2135:201"
LOC+92+CAN"
LOC+92+PEK"
RFF+AF:CA339"
DTM+189:$(yymmdd)2150:201"
DTM+232:$(yymmdd +1)0435:201"
LOC+125+PEK"
LOC+87+ICN"
ERP+2"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id))

%%

### test 12 - Inbound Two Passengers
### case 1129: One adult-OK TO BOARD - One infant-OK TO BOARD
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep ICN)
$(set time_dep "$(date_format %d.%m.%Y) 16:20")
$(set time_arv "$(date_format %d.%m.%Y) 20:35")
$(set airp_arv CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)002Y
1ONE/TWO WI
.L/MWZRDQ
.R/TKNE HK1 7842836432717/2
.R/DOCS HK1/P/CHN/E6397444822/CHN/20JUN56/M/15DEC$(date_format %y +1y)/ONE/TWO WI
.R/DOCA HK1/R/CHN
.R/INFT HK1 12APR(date_format %y -1y) ONE/UBE WI
.R/TKNE HK1 INF7842836432722/2
.R/DOCS HK1/P/CHN/E6397444756/CHN/12APR$(date_format %y -1y)/MI/09SEP$(date_format %y +1y)/ONE/UBE WI
.R/DOCA HK1/R/CHN/////I
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id1 $(get_pax_id $(get point_dep) ONE "TWO WI"))
$(set pax_id2 $(get_pax_id $(get point_dep) ONE "UBE"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id1)</pax_id>
    <surname>ONE</surname>
    <name>TWO WI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432717</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>20.06.1956 00:00:00</birth_date>
      <expiry_date>15.12.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>TWO WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id2)</pax_id>
    <surname>ONE</surname>
    <name>UBE</name>
    <pers_type>РМ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>0</seats>
    <ticket_no>7842836432722</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444756</no>
      <nationality>CHN</nationality>
      <birth_date>12.04.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>09.09.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>UBE WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++ONE:TWO WI"
ATT+2++M"
DTM+329:560620"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id1)"
RFF+YZY:7842836432717C2"
DOC+P:110:ZZZ+E6397444822"
DTM+36:$(date_format %y +1y)1215"
LOC+91+CHN"
NAD+FL+++ONE:UBE WI"
ATT+2++M"
DTM+329:$(date_format %y -1y)0412"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id2)"
RFF+YZY:7842836432722C2"
DOC+P:110:ZZZ+E6397444756"
DTM+36:$(date_format %y +1y)0909"
LOC+91+CHN"
CNT+42:2"
UNT+38+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id1)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id2)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id1))
$(OK_TO_BOARD $(get point_dep) $(get pax_id2))

%%

### test 13 - Outbound Two Passengers
### case 1167: Two Passengers-one's information is normal-OK TO BOARD, the other one checks failed-NO BOARD
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep CAN)
$(set time_dep "$(date_format %d.%m.%Y) 16:20")
$(set time_arv "$(date_format %d.%m.%Y) 20:35")
$(set airp_arv ICN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)002Y
1ONE/TWO TT
.L/MWZRCQ
.R/TKNE HK1 7842836432717/2
.R/DOCS HK1/P/CHN/3254353454/CHN/24OCT99/M/15NOV$(date_format %y +1y)/ONE/TWO TT
.R/DOCA HK1/R/CHN
1ONE/NNN NN
.L/MWZRCQ
.R/TKNE HK1 7842836432722/2
.R/DOCS HK1/P/CHN/X11111111/CHN/01JAN01/M/15NOV$(date_format %y +1y)/ONE/NNN NN
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id1 $(get_pax_id $(get point_dep) ONE "TWO TT"))
$(set pax_id2 $(get_pax_id $(get point_dep) ONE "NNN NN"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id1)</pax_id>
    <surname>ONE</surname>
    <name>TWO TT</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432717</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>3254353454</no>
      <nationality>CHN</nationality>
      <birth_date>24.10.1999 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>TWO TT</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id2)</pax_id>
    <surname>ONE</surname>
    <name>NNN NN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432722</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>X11111111</no>
      <nationality>CHN</nationality>
      <birth_date>01.01.2001 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>NNN NN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))
$(set tpr $(substr $(get ediref_paxlst) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+CAN"
DTM+189:$(yymmdd)1620:201"
LOC+87+ICN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++ONE:TWO TT"
ATT+2++M"
DTM+329:991024"
GEI+4+173"
LOC+178+CAN"
LOC+179+ICN"
NAT+2+CHN"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id1)"
RFF+YZY:7842836432717C2"
DOC+P:110:ZZZ+3254353454"
DTM+36:$(date_format %y +1y)1115"
LOC+91+CHN"
NAD+FL+++ONE:NNN NN"
ATT+2++M"
DTM+329:010101"
GEI+4+173"
LOC+178+CAN"
LOC+179+ICN"
NAT+2+CHN"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id2)"
RFF+YZY:7842836432722C2"
DOC+P:110:ZZZ+X11111111"
DTM+36:$(date_format %y +1y)1115"
LOC+91+CHN"
CNT+42:2"
UNT+38+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+CAN"
LOC+87+ICN"
ERP+2"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id1)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id2)"
ERC+1Z"
FTX+AAP+++NO BOARD, if there is a question, please contact with NIA of China, tel861056095288."
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

$(NO_BOARD $(get point_dep) $(get pax_id2))
$(OK_TO_BOARD $(get point_dep) $(get pax_id1))

%%

### test 14 - Inbound Ten Passengers Situation
### case 1208: Ten passengers-Five are Chinese, five are foreigners. Normal data-OK TO BOARD
### case 1215: Ten Passengers-Nine passengers are OK TO BOARD; one passenger is NO BOARD
### чтобы совместить два кейса в один тест, будем регистрировать 11 пассажиров (в китайских кейсах различается только десятый)
### 10 пассажиров - OK TO BOARD, 1 пассажир - NO BOARD
### заодно проверим разбиение обмена по частям
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep ICN)
$(set time_dep "$(date_format %d.%m.%Y) 16:20")
$(set time_arv "$(date_format %d.%m.%Y) 20:35")
$(set airp_arv CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)011Y
1ONE/TWO WI
.L/MWZRDQ
.R/TKNE HK1 7842836432715/2
.R/DOCS HK1/P/CHN/E6397444822/CHN/20JUN56/M/15DEC$(date_format %y +1y)/ONE/TWO WI
.R/DOCA HK1/R/CHN
1ZHANG/DING YI
.L/MWZRDZ
.R/TKNE HK1 7842836432716/3
.R/DOCS HK1/P/CHN/E6397444831/CHN/20OCT86/M/15FEB$(date_format %y +1y)/ZHANG/DING YI
.R/DOCA HK1/R/CHN
1ZHANG/ZHAO
.L/MWZRDX
.R/TKNE HK1 7842836432717/2
.R/DOCS HK1/P/CHN/G6397444822/CHN/20JUN84/M/15JAN$(date_format %y +1y)/ZHANG/ZHAO
.R/DOCA HK1/R/CHN
1XING/YA WEI
.L/MWZRDY
.R/TKNE HK1 7842836432718/3
.R/DOCS HK1/P/CHN/G6397444831/CHN/25OCT86/M/10OCT$(date_format %y +1y)/XING/YA WEI
.R/DOCA HK1/R/CHN
1QI/HUAN HUAN
.L/MWZRDW
.R/TKNE HK1 7842836432719/2
.R/DOCS HK1/P/CHN/G6397444822/CHN/16JUN88/F/20JAN$(date_format %y +1y)/QI/HUAN HUAN
.R/DOCA HK1/R/CHN
1KIM/HUI HSIN
.L/MLCKPM
.R/TKNE HK1 7812315763805/1
.R/DOCS HK1/P/CHN/4825552122/KOR/11MAR63/F/30JAN$(date_format %y +1y)/KIM/HUI HSIN
.R/DOCO HK1//V/C7155231//25OCT(date_format %y +1y)
.R/DOCA HK1/R/CHN
1JAMES/SHUMIT
.L/MLCKPM
.R/TKNE HK1 7812315763806/2
.R/DOCS HK1/P/CHN/4825552156/USA/11MAR63/F/30JAN$(date_format %y +1y)/JAMES/SHUMIT
.R/DOCO HK1//V/T1//12NOV(date_format %y +1y)
.R/DOCA HK1/R/CHN
1ADACHI/AYUM
.L/MLCKXX
.R/TKNE HK1 7812315763807/1
.R/DOCS HK1/P/CHN/4827892126/JPN/11MAR63/F/30JAN$(date_format %y +1y)/ADACHI/AYUM
.R/DOCO HK1//V/T1111//12NOV(date_format %y +1y)
.R/DOCA HK1/R/CHN
1RAUL/GONZALEZ BLANCO
.L/MLCKPM
.R/TKNE HK1 7812315763808/2
.R/DOCS HK1/P/CHN/AA2555215/ESP/11MAR63/F/30JAN$(date_format %y +1y)/RAUL/GONZALEZ BLANCO
.R/DOCA HK1/R/CHN
1ONE/NNN NN
.L/MWZRCQ
.R/TKNE HK1 7842836432722/2
.R/DOCS HK1/P/CHN/X11111111/CHN/01JAN01/M/15NOV$(date_format %y +1y)/ONE/NNN NN
.R/DOCA HK1/R/CHN
1GIUSEPPE/BUCCI
.L/MLCKXX
.R/TKNE HK1 7812317638809/1
.R/DOCS HK1/P/CHN/4827892126/ITA/11MAR00/F/30JAN$(date_format %y +1y)/GIUSEPPE/BUCCI
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_01 $(get_pax_id $(get point_dep) ONE "TWO WI"))
$(set pax_id_02 $(get_pax_id $(get point_dep) ZHANG "DING YI"))
$(set pax_id_03 $(get_pax_id $(get point_dep) ZHANG "ZHAO"))
$(set pax_id_04 $(get_pax_id $(get point_dep) XING "YA WEI"))
$(set pax_id_05 $(get_pax_id $(get point_dep) QI "HUAN HUAN"))
$(set pax_id_06 $(get_pax_id $(get point_dep) KIM "HUI HSIN"))
$(set pax_id_07 $(get_pax_id $(get point_dep) JAMES "SHUMIT"))
$(set pax_id_08 $(get_pax_id $(get point_dep) ADACHI "AYUM"))
$(set pax_id_09 $(get_pax_id $(get point_dep) RAUL "GONZALEZ BLANCO"))
$(set pax_id_10 $(get_pax_id $(get point_dep) GIUSEPPE "BUCCI"))
$(set pax_id_11 $(get_pax_id $(get point_dep) ONE "NNN NN"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>ONE</surname>
    <name>TWO WI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432715</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>20.06.1956 00:00:00</birth_date>
      <expiry_date>15.12.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>TWO WI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>ZHANG</surname>
    <name>DING YI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432716</ticket_no>
    <coupon_no>3</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>E6397444831</no>
      <nationality>CHN</nationality>
      <birth_date>20.10.1986 00:00:00</birth_date>
      <expiry_date>15.02.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ZHANG</surname>
      <first_name>DING YI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_03)</pax_id>
    <surname>ZHANG</surname>
    <name>ZHAO</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432717</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>20.06.1984 00:00:00</birth_date>
      <expiry_date>15.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ZHANG</surname>
      <first_name>ZHAO</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_04)</pax_id>
    <surname>XING</surname>
    <name>YA WEI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432718</ticket_no>
    <coupon_no>3</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444831</no>
      <nationality>CHN</nationality>
      <birth_date>25.10.1986 00:00:00</birth_date>
      <expiry_date>10.10.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>XING</surname>
      <first_name>YA WEI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_05)</pax_id>
    <surname>QI</surname>
    <name>HUAN HUAN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432719</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>16.06.1988 00:00:00</birth_date>
      <expiry_date>20.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>QI</surname>
      <first_name>HUAN HUAN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_06)</pax_id>
    <surname>KIM</surname>
    <name>HUI HSIN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763805</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552122</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KIM</surname>
      <first_name>HUI HSIN</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>C7155231</no>
      <expiry_date>25.10.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_07)</pax_id>
    <surname>JAMES</surname>
    <name>SHUMIT</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763806</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552156</no>
      <nationality>USA</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>JAMES</surname>
      <first_name>SHUMIT</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>T1</no>
      <expiry_date>12.11.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_08)</pax_id>
    <surname>ADACHI</surname>
    <name>AYUM</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763807</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4827892126</no>
      <nationality>JPN</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>ADACHI</surname>
      <first_name>AYUM</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>T1111</no>
      <expiry_date>12.11.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_09)</pax_id>
    <surname>RAUL</surname>
    <name>GONZALEZ BLANCO</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763808</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>AA2555215</no>
      <nationality>ESP</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>RAUL</surname>
      <first_name>GONZALEZ BLANCO</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_10)</pax_id>
    <surname>GIUSEPPE</surname>
    <name>BUCCI</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812317638809</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4827892126</no>
      <nationality>ITA</nationality>
      <birth_date>11.03.2000 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>GIUSEPPE</surname>
      <first_name>BUCCI</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_11)</pax_id>
    <surname>ONE</surname>
    <name>NNN NN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432722</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>X11111111</no>
      <nationality>CHN</nationality>
      <birth_date>01.01.2001 00:00:00</birth_date>
      <expiry_date>15.11.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ONE</surname>
      <first_name>NNN NN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst_01 $(last_edifact_ref 1))
$(set ediref_paxlst_02 $(last_edifact_ref 0))
$(set tpr $(substr $(get ediref_paxlst_01) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_01)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_01)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++ONE:TWO WI"
ATT+2++M"
DTM+329:560620"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id_01)"
RFF+YZY:7842836432715C2"
DOC+P:110:ZZZ+E6397444822"
DTM+36:$(date_format %y +1y)1215"
LOC+91+CHN"
NAD+FL+++ZHANG:DING YI"
ATT+2++M"
DTM+329:861020"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDZ"
RFF+ABO:$(get pax_id_02)"
RFF+YZY:7842836432716C3"
DOC+P:110:ZZZ+E6397444831"
DTM+36:$(date_format %y +1y)0215"
LOC+91+CHN"
NAD+FL+++ZHANG:ZHAO"
ATT+2++M"
DTM+329:840620"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDX"
RFF+ABO:$(get pax_id_03)"
RFF+YZY:7842836432717C2"
DOC+P:110:ZZZ+G6397444822"
DTM+36:$(date_format %y +1y)0115"
LOC+91+CHN"
NAD+FL+++XING:YA WEI"
ATT+2++M"
DTM+329:861025"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDY"
RFF+ABO:$(get pax_id_04)"
RFF+YZY:7842836432718C3"
DOC+P:110:ZZZ+G6397444831"
DTM+36:$(date_format %y +1y)1010"
LOC+91+CHN"
NAD+FL+++QI:HUAN HUAN"
ATT+2++F"
DTM+329:880616"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDW"
RFF+ABO:$(get pax_id_05)"
RFF+YZY:7842836432719C2"
DOC+P:110:ZZZ+G6397444822"
DTM+36:$(date_format %y +1y)0120"
LOC+91+CHN"
NAD+FL+++KIM:HUI HSIN"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+KOR"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_06)"
RFF+YZY:7812315763805C1"
DOC+P:110:ZZZ+4825552122"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+C7155231"
DTM+36:$(date_format %y +1y)1025"
NAD+FL+++JAMES:SHUMIT"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+USA"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_07)"
RFF+YZY:7812315763806C2"
DOC+P:110:ZZZ+4825552156"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+T1"
DTM+36:$(date_format %y +1y)1112"
NAD+FL+++ADACHI:AYUM"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+JPN"
RFF+AVF:MLCKXX"
RFF+ABO:$(get pax_id_08)"
RFF+YZY:7812315763807C1"
DOC+P:110:ZZZ+4827892126"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+T1111"
DTM+36:$(date_format %y +1y)1112"
NAD+FL+++RAUL:GONZALEZ BLANCO"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+ESP"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_09)"
RFF+YZY:7812315763808C2"
DOC+P:110:ZZZ+AA2555215"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
NAD+FL+++ONE:NNN NN"
ATT+2++M"
DTM+329:010101"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id_11)"
RFF+YZY:7842836432722C2"
DOC+P:110:ZZZ+X11111111"
DTM+36:$(date_format %y +1y)1115"
LOC+91+CHN"
CNT+42:10"
UNT+148+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_01)0001"

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_02)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_02)+01:F"
BGM+745"
RFF+TN:0000102"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++GIUSEPPE:BUCCI"
ATT+2++F"
DTM+329:000311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+ITA"
RFF+AVF:MLCKXX"
RFF+ABO:$(get pax_id_10)"
RFF+YZY:7812317638809C1"
DOC+P:110:ZZZ+4827892126"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
CNT+42:1"
UNT+25+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_02)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst_01)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst_01)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MWZRCQ"
RFF+ABO:$(get pax_id_11)"
ERC+1Z"
FTX+AAP+++NO BOARD, if there is a question, please contact with NIA of China, tel861056095288."
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_09)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MLCKXX"
RFF+ABO:$(get pax_id_08)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_07)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_06)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRDW"
RFF+ABO:$(get pax_id_05)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRDY"
RFF+ABO:$(get pax_id_04)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRDX"
RFF+ABO:$(get pax_id_03)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRDZ"
RFF+ABO:$(get pax_id_02)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MWZRDQ"
RFF+ABO:$(get pax_id_01)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+59+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_01)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst_02)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst_02)+01:F"
BGM+962"
RFF+TN:0000102"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MLCKXX"
RFF+ABO:$(get pax_id_10)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_02)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id_10))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_08))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_06))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_04))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_02))
$(NO_BOARD    $(get point_dep) $(get pax_id_11))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_01))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_03))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_05))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_07))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_09))

### case 1228: Flight Close
### case 1229: Flight Cancel
### в каталог apis/IAPI_CN формируется
#########################################################################################

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) $(get time_dep) "" $(get airline) $(get flt_no) "" $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(CHANGE_SPP_FLIGHT_ONE_LEG $(get point_dep) $(get time_dep) "1" $(get airline) $(get flt_no) "" $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

%%

### test 15
### План такой:
### 1. Регистрируем двоих - получаем CHECK(BGM+745)
### 2. Отменяем двоим регистрацию по ошибке агента
### 3. Регистрируем четверых, двое предыдущих среди них - получаем CHANGE(BGM+745+CP) по предыдущим и CHECK(BGM+745) по новым
###    Это происходит потому, что при регистрации заново мы не меняем pax_id(RFF+ABO), соответственно второй раз при регистрации заново посылаем уже CHANGE
### 4. Сохраняем изменения, указывая данные всех четверых. У первого не меняем ничего.
###    У второго меняем страну проживания. У третьего - удаляем визу. У четвертого - данные документа
###    Получаем CHANGE(BGM+745+CP) только по третьему и четвертому, потому что именно у них значимые для IAPI изменения
### 5. Первому меняем данные документа, добавляем ремарку RXIA (переспрос IAPI), отменяем регистрацию не по ошибке агента
###    Второму только добавляем ремарку RXIA.
###    Третьему меняем данные документа, визы, добавляем ремарку RXIA.
###    Четвертому добавляем ничего не значащую ремарку.
###    В итоге имеем CHANGE(BGM+745+CP) по второму и третьему
### 6. До этого момента ответы от китайцев на запросы не приходили и у первого пассажира отмена регистрации
###    Затем приходит ответ для п.4. В итоге можем посадить только четвертого, так как для третьего послали новый запрос в п.5
###    Потом приходит ответ для п.5. И вот теперь то мы можем посадить второго и третьего
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep ICN)
$(set time_dep "$(date_format %d.%m.%Y) 16:20")
$(set time_arv "$(date_format %d.%m.%Y) 20:35")
$(set airp_arv CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)004Y
1QI/HUAN HUAN
.L/MWZRDW
.R/TKNE HK1 7842836432719/2
.R/DOCS HK1/P/CHN/G6397444822/CHN/16JUN88/F/20JAN$(date_format %y +1y)/QI/HUAN HUAN
.R/DOCA HK1/R/CHN
1KIM/HUI HSIN
.L/MLCKPM
.R/TKNE HK1 7812315763805/1
.R/DOCS HK1/P/CHN/4825552122/KOR/11MAR63/F/30JAN$(date_format %y +1y)/KIM/HUI HSIN
.R/DOCO HK1//V/C7155231//25OCT(date_format %y +1y)
.R/DOCA HK1/R/CHN
1JAMES/SHUMIT
.L/MLCKPM
.R/TKNE HK1 7812315763806/2
.R/DOCS HK1/P/CHN/4825552156/USA/11MAR63/F/30JAN$(date_format %y +1y)/JAMES/SHUMIT
.R/DOCO HK1//V/T1//12NOV(date_format %y +1y)
.R/DOCA HK1/R/CHN
1ADACHI/AYUM
.L/MLCKXX
.R/TKNE HK1 7812315763807/1
.R/DOCS HK1/P/CHN/4827892126/JPN/11MAR63/F/30JAN$(date_format %y +1y)/ADACHI/AYUM
.R/DOCO HK1//V/T1111//12NOV(date_format %y +1y)
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_01 $(get_pax_id $(get point_dep) QI "HUAN HUAN"))
$(set pax_id_02 $(get_pax_id $(get point_dep) KIM "HUI HSIN"))
$(set pax_id_03 $(get_pax_id $(get point_dep) JAMES "SHUMIT"))
$(set pax_id_04 $(get_pax_id $(get point_dep) ADACHI "AYUM"))

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>QI</surname>
    <name>HUAN HUAN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432719</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>16.06.1988 00:00:00</birth_date>
      <expiry_date>20.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>QI</surname>
      <first_name>HUAN HUAN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>KIM</surname>
    <name>HUI HSIN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763805</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552122</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KIM</surname>
      <first_name>HUI HSIN</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>C7155231</no>
      <expiry_date>25.10.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst $(last_edifact_ref))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++QI:HUAN HUAN"
ATT+2++F"
DTM+329:880616"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDW"
RFF+ABO:$(get pax_id_01)"
RFF+YZY:7842836432719C2"
DOC+P:110:ZZZ+G6397444822"
DTM+36:$(date_format %y +1y)0120"
LOC+91+CHN"
NAD+FL+++KIM:HUI HSIN"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+KOR"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_02)"
RFF+YZY:7812315763805C1"
DOC+P:110:ZZZ+4825552122"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+C7155231"
DTM+36:$(date_format %y +1y)1025"
CNT+42:2"
UNT+40+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst)0001"

#отменяем регистрацию обоих пассажиров по ошибке агента

$(set grp_id $(get_single_grp_id $(get point_dep) QI "HUAN HUAN"))
$(set grp_tid $(get_single_tid $(get point_dep) QI "HUAN HUAN"))
$(set pax_tid_01 $(get_single_pax_tid $(get point_dep) QI "HUAN HUAN"))
$(set pax_tid_02 $(get_single_pax_tid $(get point_dep) KIM "HUI HSIN"))

$(CHANGE_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid)
capture=on
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>QI</surname>
    <name>HUAN HUAN</name>
    <pers_type>ВЗ</pers_type>
    <refuse>А</refuse>
    <ticket_no>7842836432719</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>16.06.1988 00:00:00</birth_date>
      <expiry_date>20.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>QI</surname>
      <first_name>HUAN HUAN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_01)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>KIM</surname>
    <name>HUI HSIN</name>
    <pers_type>ВЗ</pers_type>
    <refuse>А</refuse>
    <ticket_no>7812315763805</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552122</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KIM</surname>
      <first_name>HUI HSIN</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>C7155231</no>
      <expiry_date>25.10.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_02)</tid>
  </pax>
</passengers>
})

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
    <segments/>
  </answer>
</term>

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>QI</surname>
    <name>HUAN HUAN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7842836432719</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>16.06.1988 00:00:00</birth_date>
      <expiry_date>20.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>QI</surname>
      <first_name>HUAN HUAN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>KIM</surname>
    <name>HUI HSIN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763805</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552122</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KIM</surname>
      <first_name>HUI HSIN</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>C7155231</no>
      <expiry_date>25.10.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_03)</pax_id>
    <surname>JAMES</surname>
    <name>SHUMIT</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763806</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552156</no>
      <nationality>USA</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>JAMES</surname>
      <first_name>SHUMIT</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>T1</no>
      <expiry_date>12.11.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_04)</pax_id>
    <surname>ADACHI</surname>
    <name>AYUM</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763807</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4827892126</no>
      <nationality>JPN</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>ADACHI</surname>
      <first_name>AYUM</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>T1111</no>
      <expiry_date>12.11.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(set ediref_paxlst_01 $(last_edifact_ref 1))
$(set ediref_paxlst_02 $(last_edifact_ref 0))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_01)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_01)+01:F"
BGM+745"
RFF+TN:0000102"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++JAMES:SHUMIT"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+USA"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_03)"
RFF+YZY:7812315763806C2"
DOC+P:110:ZZZ+4825552156"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+T1"
DTM+36:$(date_format %y +1y)1112"
NAD+FL+++ADACHI:AYUM"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+JPN"
RFF+AVF:MLCKXX"
RFF+ABO:$(get pax_id_04)"
RFF+YZY:7812315763807C1"
DOC+P:110:ZZZ+4827892126"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+T1111"
DTM+36:$(date_format %y +1y)1112"
CNT+42:2"
UNT+42+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_01)0001"

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_02)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_02)+01:F"
BGM+745+CP"
RFF+TN:0000103"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++QI:HUAN HUAN"
ATT+2++F"
DTM+329:880616"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+CHN"
RFF+AVF:MWZRDW"
RFF+ABO:$(get pax_id_01)"
RFF+YZY:7842836432719C2"
DOC+P:110:ZZZ+G6397444822"
DTM+36:$(date_format %y +1y)0120"
LOC+91+CHN"
NAD+FL+++KIM:HUI HSIN"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+KOR"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_02)"
RFF+YZY:7812315763805C1"
DOC+P:110:ZZZ+4825552122"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+C7155231"
DTM+36:$(date_format %y +1y)1025"
CNT+42:2"
UNT+40+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_02)0001"

$(set grp_id $(get_single_grp_id $(get point_dep) QI "HUAN HUAN"))
$(set grp_tid $(get_single_tid $(get point_dep) QI "HUAN HUAN"))
$(set pax_tid_01 $(get_single_pax_tid $(get point_dep) QI "HUAN HUAN"))
$(set pax_tid_02 $(get_single_pax_tid $(get point_dep) KIM "HUI HSIN"))
$(set pax_tid_03 $(get_single_pax_tid $(get point_dep) JAMES "SHUMIT"))
$(set pax_tid_04 $(get_single_pax_tid $(get point_dep) ADACHI "AYUM"))

$(CHANGE_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>QI</surname>
    <name>HUAN HUAN</name>
    <pers_type>ВЗ</pers_type>
    <refuse/>
    <ticket_no>7842836432719</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>16.06.1988 00:00:00</birth_date>
      <expiry_date>20.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>QI</surname>
      <first_name>HUAN HUAN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_01)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>KIM</surname>
    <name>HUI HSIN</name>
    <pers_type>ВЗ</pers_type>
    <refuse/>
    <ticket_no>7812315763805</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552122</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KIM</surname>
      <first_name>HUI HSIN</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>C7155231</no>
      <expiry_date>25.10.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>KOR</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_02)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_03)</pax_id>
    <surname>JAMES</surname>
    <name>SHUMIT</name>
    <pers_type>ВЗ</pers_type>
    <refuse/>
    <ticket_no>7812315763806</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552156</no>
      <nationality>USA</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>JAMES</surname>
      <first_name>SHUMIT</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_03)</tid>
  </pax>
  <pax>
    <pax_id>$(get pax_id_04)</pax_id>
    <surname>ADACHI</surname>
    <name>AYUM</name>
    <pers_type>ВЗ</pers_type>
    <refuse/>
    <ticket_no>7812315763807</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4827892126</no>
      <nationality>JPN</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ADACHI</surname>
      <first_name>AYUM</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>T1111</no>
      <expiry_date>12.11.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_04)</tid>
  </pax>
</passengers>
})

$(set ediref_paxlst_01 $(last_edifact_ref))
$(set tpr_01 $(substr $(get ediref_paxlst_01) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_01)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_01)+01:F"
BGM+745+CP"
RFF+TN:0000105"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++JAMES:SHUMIT"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+USA"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_03)"
RFF+YZY:7812315763806C2"
DOC+P:110:ZZZ+4825552156"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
NAD+FL+++ADACHI:AYUM"
ATT+2++M"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+JPN"
RFF+AVF:MLCKXX"
RFF+ABO:$(get pax_id_04)"
RFF+YZY:7812315763807C1"
DOC+P:110:ZZZ+4827892126"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+T1111"
DTM+36:$(date_format %y +1y)1112"
CNT+42:2"
UNT+40+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_01)0001"

$(set grp_id $(get_single_grp_id $(get point_dep) QI "HUAN HUAN"))
$(set grp_tid $(get_single_tid $(get point_dep) QI "HUAN HUAN"))
$(set pax_tid_01 $(get_single_pax_tid $(get point_dep) QI "HUAN HUAN"))
$(set pax_tid_02 $(get_single_pax_tid $(get point_dep) KIM "HUI HSIN"))
$(set pax_tid_03 $(get_single_pax_tid $(get point_dep) JAMES "SHUMIT"))
$(set pax_tid_04 $(get_single_pax_tid $(get point_dep) ADACHI "AYUM"))

$(CHANGE_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv) $(get grp_id) $(get grp_tid)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>QI</surname>
    <name>HUAN HUAN</name>
    <pers_type>ВЗ</pers_type>
    <refuse>З</refuse>
    <ticket_no>7842836432719</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>16.06.1988 00:00:00</birth_date>
      <expiry_date>20.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>QI</surname>
      <first_name>HUAN HUAN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_01)</tid>
    <rems>
      <rem>
        <rem_code>RSIA</rem_code>
        <rem_text>RSIA</rem_text>
      </rem>
    </rems>
    <fqt_rems/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>KIM</surname>
    <name>HUI HSIN</name>
    <tid>$(get pax_tid_02)</tid>
    <rems>
      <rem>
        <rem_code>RSIA</rem_code>
        <rem_text>RSIA</rem_text>
      </rem>
    </rems>
    <fqt_rems/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_03)</pax_id>
    <surname>JAMES</surname>
    <name>SHUMIT</name>
    <pers_type>ВЗ</pers_type>
    <refuse/>
    <ticket_no>7812315763806</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552156</no>
      <nationality>USA</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>JAMES</surname>
      <first_name>SHUMIT</first_name>
    </document>
    <doco>
      <type>V</type>
      <no>T1</no>
      <expiry_date>12.11.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <addresses>
      <doca>
        <type>R</type>
        <country>CHN</country>
      </doca>
    </addresses>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(get pax_tid_03)</tid>
    <rems>
      <rem>
        <rem_code>RSIA</rem_code>
        <rem_text>RSIA</rem_text>
      </rem>
    </rems>
    <fqt_rems/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_04)</pax_id>
    <surname>ADACHI</surname>
    <name>AYUM</name>
    <tid>$(get pax_tid_04)</tid>
    <rems>
      <rem>
        <rem_code>XAXA</rem_code>
        <rem_text>XAXA</rem_text>
      </rem>
    </rems>
    <fqt_rems/>
  </pax>
</passengers>
})

$(set ediref_paxlst_02 $(last_edifact_ref))
$(set tpr_02 $(substr $(get ediref_paxlst_02) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_02)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_02)+01:F"
BGM+745+CP"
RFF+TN:0000107"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++KIM:HUI HSIN"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+KOR"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_02)"
RFF+YZY:7812315763805C1"
DOC+P:110:ZZZ+4825552122"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+C7155231"
DTM+36:$(date_format %y +1y)1025"
NAD+FL+++JAMES:SHUMIT"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+USA"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_03)"
RFF+YZY:7812315763806C2"
DOC+P:110:ZZZ+4825552156"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+T1"
DTM+36:$(date_format %y +1y)1112"
CNT+42:2"
UNT+42+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_02)0001"

#сначала как бы с задержкой приходит ответ на ediref_paxlst_01
#до него вообще как бы ответы не приходили

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr_01)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst_01)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst_01)+01:F"
BGM+962"
RFF+TN:0000105"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_03)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MLCKXX"
RFF+ABO:$(get pax_id_04)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_01)0001"

$(NO_BOARD $(get point_dep) $(get pax_id_02))
$(NO_BOARD $(get point_dep) $(get pax_id_03))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_04))

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr_02)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst_02)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst_02)+01:F"
BGM+962"
RFF+TN:0000107"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_02)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_03)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+19+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_02)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id_02))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_03))

%%

### test 16
### Проверяем доввод реквизитов на посадке и запрос CHANGE(BGM+745+CP), если они по какой-то причине не введены на регистрации
### Регистрируем трех до того, как на рейс настроен IAPI_CN. У второго нет документа, у третьего нет визы.
### Включаем настройку IAPI_CN.
### Пытаемся посадить - у второго и третьего ошибка при посадке: неполные данные APIS
### Довводим на посадке нехватающие данные. После этого успешно сажаем
#########################################################################################

$(init_term)

$(set airline CA)
$(set flt_no 339)
$(set airp_dep ICN)
$(set time_dep "$(date_format %d.%m.%Y) 16:20")
$(set time_arv "$(date_format %d.%m.%Y) 20:35")
$(set airp_arv CAN)

<<
XXXXXXX
.XXXXXXX $(dd -1)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +0 en) $(get airp_dep) PART1
-$(get airp_arv)004Y
1QI/HUAN HUAN
.L/MWZRDW
.R/TKNE HK1 7842836432719/2
.R/DOCS HK1/P/CHN/G6397444822/CHN/16JUN88/F/20JAN$(date_format %y +1y)/QI/HUAN HUAN
.R/DOCA HK1/R/CHN
1KIM/HUI HSIN
.L/MLCKPM
.R/TKNE HK1 7812315763805/1
.R/DOCS HK1/P/CHN/4825552122/KOR/11MAR63/F/30JAN$(date_format %y +1y)/KIM/HUI HSIN
.R/DOCO HK1//V/C7155231//25OCT(date_format %y +1y)
.R/DOCA HK1/R/CHN
1JAMES/SHUMIT
.L/MLCKPM
.R/TKNE HK1 7812315763806/2
.R/DOCS HK1/P/CHN/4825552156/USA/11MAR63/F/30JAN$(date_format %y +1y)/JAMES/SHUMIT
.R/DOCO HK1//V/T1//12NOV(date_format %y +1y)
.R/DOCA HK1/R/CHN
1ADACHI/AYUM
.L/MLCKXX
.R/TKNE HK1 7812315763807/1
.R/DOCS HK1/P/CHN/4827892126/JPN/11MAR63/F/30JAN$(date_format %y +1y)/ADACHI/AYUM
.R/DOCO HK1//V/T1111//12NOV(date_format %y +1y)
.R/DOCA HK1/R/CHN
ENDPNL

$(PREPARE_SPP_FLIGHT_AFTER_PNL $(get airline) $(get flt_no) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id_01 $(get_pax_id $(get point_dep) QI "HUAN HUAN"))
$(set pax_id_02 $(get_pax_id $(get point_dep) KIM "HUI HSIN"))
$(set pax_id_03 $(get_pax_id $(get point_dep) JAMES "SHUMIT"))
$(set pax_id_04 $(get_pax_id $(get point_dep) ADACHI "AYUM"))

$(NEW_CHECKIN_REQUEST $(get point_dep) $(get point_arv) $(get airp_dep) $(get airp_arv)
{
<passengers>
  <pax>
    <pax_id>$(get pax_id_01)</pax_id>
    <surname>QI</surname>
    <name>HUAN HUAN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no/>
    <coupon_no/>
    <ticket_rem/>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>G6397444822</no>
      <nationality>CHN</nationality>
      <birth_date>16.06.1988 00:00:00</birth_date>
      <expiry_date>20.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>QI</surname>
      <first_name>HUAN HUAN</first_name>
    </document>
    <doco>
      <type>-</type>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_02)</pax_id>
    <surname>KIM</surname>
    <name>HUI HSIN</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763805</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document/>
    <doco>
      <type>V</type>
      <no>C7155231</no>
      <expiry_date>25.10.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
  <pax>
    <pax_id>$(get pax_id_03)</pax_id>
    <surname>JAMES</surname>
    <name>SHUMIT</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>7812315763806</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552156</no>
      <nationality>USA</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>JAMES</surname>
      <first_name>SHUMIT</first_name>
    </document>
    <doco/>
    <subclass>Э</subclass>
    <bag_pool_num/>
    <transfer/>
  </pax>
</passengers>
})

$(PREPARE_CN_EXCHANGE_SETTINGS $(get airline))

$(OK_TO_BOARD $(get point_dep) $(get pax_id_01))
$(APIS_INCOMPLETE_ON_BOARDING $(get point_dep) $(get pax_id_02))
$(APIS_INCOMPLETE_ON_BOARDING $(get point_dep) $(get pax_id_03))

$(set pax_tid_02 $(get_single_pax_tid $(get point_dep) KIM "HUI HSIN"))
$(set pax_tid_03 $(get_single_pax_tid $(get point_dep) JAMES "SHUMIT"))

$(SAVE_APIS_REQUEST $(get point_dep) $(get pax_id_02) $(get pax_tid_02)
{
<apis>
    <document>
      <type>P</type>
      <issue_country>CHN</issue_country>
      <no>4825552122</no>
      <nationality>KOR</nationality>
      <birth_date>11.03.1963 00:00:00</birth_date>
      <expiry_date>30.01.$(date_format %Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KIM</surname>
      <first_name>HUI HSIN</first_name>
    </document>
</apis>
})

$(set ediref_paxlst_01 $(last_edifact_ref))
$(set tpr_01 $(substr $(get ediref_paxlst_01) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_01)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_01)+01:F"
BGM+745"
RFF+TN:0000100"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++KIM:HUI HSIN"
ATT+2++U"  ### !!! F
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+KOR"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_02)"
RFF+YZY:7812315763805C1"
DOC+P:110:ZZZ+4825552122"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+C7155231"
DTM+36:$(date_format %y +1y)1025"
CNT+42:1"
UNT+27+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_01)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr_01)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst_01)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst_01)+01:F"
BGM+962"
RFF+TN:0000100"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_02)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_01)0001"

$(SAVE_APIS_REQUEST $(get point_dep) $(get pax_id_03) $(get pax_tid_03)
{
<apis>
    <doco>
      <type>V</type>
      <no>T1</no>
      <expiry_date>12.11.$(date_format %Y +1y) 00:00:00</expiry_date>
    </doco>
</apis>
})

$(set ediref_paxlst_02 $(last_edifact_ref))
$(set tpr_02 $(substr $(get ediref_paxlst_02) 6, 4))

>>
UNB+SIRE:4+CA+NIAC+xxxxxx:xxxx+$(get ediref_paxlst_02)0001++IAPI+O"
UNG+PAXLST+CA+NIAC+xxxxxx:xxxx+1+UN+D:05B"
UNH+11085B94E1F8FA+PAXLST:D:05B:UN:IATA+$(get ediref_paxlst_02)+01:F"
BGM+745"
RFF+TN:0000102"
NAD+MS+++SIRENA-TRAVEL"
COM+4959504991:TE+4959504973:FX"
TDT+20+CA339+++CA"
LOC+125+ICN"
DTM+189:$(yymmdd)1620:201"
LOC+87+CAN"
DTM+232:$(yymmdd)2035:201"
NAD+FL+++JAMES:SHUMIT"
ATT+2++F"
DTM+329:630311"
GEI+4+173"
LOC+178+ICN"
LOC+179+CAN"
NAT+2+USA"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_03)"
RFF+YZY:7812315763806C2"
DOC+P:110:ZZZ+4825552156"
DTM+36:$(date_format %y +1y)0130"
LOC+91+CHN"
DOC+V:110:ZZZ+T1"
DTM+36:$(date_format %y +1y)1112"
CNT+42:1"
UNT+27+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_02)0001"

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P$(get tpr_02)\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+CA+$(yymmdd):$(hhmi)+$(get ediref_paxlst_02)0001++IAPI"
UNG+CUSRES+NIAC+CA+$(yymmdd):$(hhmi)+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA+$(get ediref_paxlst_02)+01:F"
BGM+962"
RFF+TN:0000102"
RFF+AF:CA339"
DTM+189:$(yymmdd)1620:201"
DTM+232:$(yymmdd)2035:201"
LOC+125+ICN"
LOC+87+CAN"
ERP+2"
RFF+AVF:MLCKPM"
RFF+ABO:$(get pax_id_03)"
ERC+0Z"
FTX+AAP+++OK TO BOARD"
UNT+14+11085B94E1F8FA"
UNE+1+1"
UNZ+1+$(get ediref_paxlst_02)0001"

$(OK_TO_BOARD $(get point_dep) $(get pax_id_02))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_03))


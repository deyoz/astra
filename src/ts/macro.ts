$(defmacro login
  user=PIKE
  passwd=PIKE
{
{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='MainDCS' ver='1' opr='' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <UserLogon>
       <term_version>201311-0154253</term_version>
       <userr>PIKE</userr>
       <passwd>PIKE</passwd>
       <airlines/>
       <devices/>
       <command_line_params>
         <param>RESTART</param>
         <param>NOCUTE</param>
         <param>LANGRU</param>
       </command_line_params>
     </UserLogon>
   </query>
</term>}
}
) # end-of-defmacro

$(defmacro login2
  user=PIKE
  passwd=PIKE
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='MainDCS' ver='1' opr='' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <UserLogon>
      <term_version>201311-0154253</term_version>
      <lang dictionary_lang='RU' dictionary_checksum='622046546'>RU</lang>
      <userr>PIKE</userr>
      <passwd>PIKE</passwd>
      <airlines/>
      <devices/>
      <command_line_params>
        <param>RESTART</param>
        <param>NOCUTE</param>
        <param>LANGRU</param>
      </command_line_params>
    </UserLogon>
  </query>
</term>}
}
) # end-of-defmacro


$(defmacro KICK_IN
{

>> lines=auto
    <kick req_ctxt_id...


!! capture=on
$(lastRedisplay)

}) #end-of-macro


$(defmacro KICK_IN_SILENT
{

>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

}) #end-of-macro


$(defmacro PREPARE_SEASON_SCD
  airl=UT
  depp=DME
  arrp=AER
  fltno=747
  craft=TU5
{
{<?xml version='1.0' encoding='CP866'?>
 <term>
  <query handle='0' id='season' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <write>
      <filter>
        <season>2</season>
      </filter>
      <SubrangeList>
        <subrange>
          <modify>insert</modify>
          <move_id>-1</move_id>
          <first>$(date_format %d.%m.%Y -1mon) 12:00:00</first>
          <last>$(date_format %d.%m.%Y +1mon) 12:00:00</last>
          <days>1234567</days>
          <dests  >
            <dest>
              <cod>$(depp)</cod>
              <company>$(airl)</company>
              <trip>$(fltno)</trip>
              <bc>$(craft)</bc>
              <takeoff>30.12.1899 10:00:00</takeoff>
              <y>-1</y>
            </dest>
            <dest>
              <cod>$(arrp)</cod>
              <land>30.12.1899 12:00:00</land>
            </dest>
          </dests  >
        </subrange>
      </SubrangeList>
    </write>
  </query>
</term>}
}
) # end-of-macro


$(defmacro INBOUND_PNL_1
    airl
    depp
    arrp
    flt
    surname
    name
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl)$(flt)/$(ddmon +0 en) $(depp) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp)  $(arrp)
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1$(surname)/$(name)
.L/0840Z6/$(airl)
.L/09T1B3/1H
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro


$(defmacro PREPARE_FLIGHT_1
    airl=ЮТ
    flt=103
    depp=ДМД
    arrp=ПЛК
    surname=REPIN
    name=IVAN
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl))
                     $(get_lat_code aer $(depp))
                     $(get_lat_code aer $(arrp))
                     $(flt))
$(create_spp $(ddmmyyyy +0))

<<
$(INBOUND_PNL_1 $(get_lat_code awk $(airl))
                $(get_lat_code aer $(depp))
                $(get_lat_code aer $(arrp))
                $(flt)
                $(surname) $(name))

$(create_random_trip_comp $(get_dep_point_id $(depp) $(airl) 103 $(yymmdd +0)) Э)

}) #end-of-macro


$(defmacro INBOUND_PNL_2
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    surname
    name
    tickno
    cpnno
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1$(surname)/$(name)
.L/0840Z6/$(airl1)
.L/09T1B3/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
.R/TKNE HK1 $(tickno)/$(cpnno)-1$(surname)/$(name)
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/$(surname)
.RN//$(name)-1$(surname)/$(name)
.R/PSPT HK1 ZB400522509/TJK/24JUL85/$(surname)/$(name)/M
.RN/-1$(surname)/$(name)
.R/FOID PPZB400522509-1$(surname)/$(name)
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro


$(defmacro PREPARE_FLIGHT_2
    airl1=ЮТ
    flt1=103
    depp1=ДМД
    arrp1=ПЛК
    airl2=СУ
    flt2=2278
    depp2=ПЛК
    arrp2=СОЧ
    surname=REPIN
    name=IVAN
    tickno=2982401841689
    cpnno=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(create_spp $(ddmmyyyy +0))

<<
$(INBOUND_PNL_2 $(airl1) $(depp1) $(arrp1) $(flt1)
                $(airl2) $(depp2) $(arrp2) $(flt2) $(surname) $(name)
                $(tickno) $(cpnno))

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
TKT+2982401841689"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+$(surname)+$(name)"
TAI+0162"
RCI+UA:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+$(depp1)+$(arrp1)"
ORG+UT:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+2982401841689:T:1:3"
CPN+1:I"
TVL+$(ddmmyy):2205+$(depp1)+$(arrp1)+$(airl1)+$(flt1):Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+$(last_edifact_ref)0001"

$(create_random_trip_comp $(get_dep_point_id ДМД ЮТ 103 $(yymmdd +0)) Э)

}) #end-of-macro


$(defmacro INBOUND_PNL_3
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    surname
    name
    tickno
    cpnno
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1$(surname)/$(name)
.L/0840Z6/$(airl1)
.L/09T1B3/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro


$(defmacro PREPARE_FLIGHT_3
    airl1=ЮТ
    flt1=103
    depp1=ДМД
    arrp1=ПЛК
    airl2=С7
    flt2=1027
    depp2=ПЛК
    arrp2=СОЧ
    surname=REPIN
    name=IVAN
    tickno=2982401841689
    cpnno=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(create_spp $(ddmmyyyy +0))

<<
$(INBOUND_PNL_3 $(airl1) $(depp1) $(arrp1) $(flt1)
                $(airl2) $(depp2) $(arrp2) $(flt2) $(surname) $(name)
                $(tickno) $(cpnno))

$(create_random_trip_comp $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)) Э)

}) #end-of-macro


$(defmacro INBOUND_PNL_4
    airl1
    depp1
    arrp1
    flt1
    surname1
    name1
    surname2
    name2
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/Y
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y058
-LED000F
-LED000C
-LED002Y
1$(surname1)/$(name1)
.L/0840Z6/$(airl1)
.L/09T1B3/1H
1$(surname2)/$(name2)
.L/0840Z7/$(airl1)
.L/09T1B4/1H
ENDPNL}
) #end-of-macro


$(defmacro PREPARE_FLIGHT_4
    airl=ЮТ
    flt=103
    depp=ДМД
    arrp=ПЛК
    surname1=РЕПИН
    name1=ИВАН
    surname2=РЕПИНА
    name2=АННА
{

$(set airl_lat $(get_lat_code awk $(airl)))
$(set depp_lat $(get_lat_code aer $(depp)))
$(set arrp_lat $(get_lat_code aer $(arrp)))

$(PREPARE_SEASON_SCD $(get airl_lat)
                     $(get depp_lat)
                     $(get arrp_lat)
                     $(flt))
$(create_spp $(ddmmyyyy +0))

<<
$(INBOUND_PNL_4 $(get airl_lat)
                $(get depp_lat)
                $(get arrp_lat)
                $(flt)
                $(surname1) $(name1)
                $(surname2) $(name2))

$(create_random_trip_comp $(get_dep_point_id $(depp) $(airl) $(flt) $(yymmdd +0)) Э)

}) #end-of-macro


$(defmacro INBOUND_PNL_5
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    surname1
    name1
    tickno1
    cpnno1
    surname2
    name2
    tickno2
    cpnno2
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED002Y
1$(surname1)/$(name1)
.L/0840Z7/$(airl1)
.L/09T1B4/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
.R/TKNE HK1 $(tickno1)/$(cpnno1)-1$(surname1)/$(name1)
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/$(surname1)
.RN//$(name1)-1$(surname1)/$(name1)
.R/PSPT HK1 ZB400522509/TJK/24JUL85/$(surname1)/$(name1)/M
.RN/-1$(surname1)/$(name1)
.R/FOID PPZB400522509-1$(surname1)/$(name1)
1$(surname2)/$(name2)
.L/0840Z6/$(airl1)
.L/09T1B3/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
.R/TKNE HK1 $(tickno2)/$(cpnno2)-1$(surname2)/$(name2)
.R/DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/$(surname2)
.RN//$(name2)-1$(surname2)/$(name2)
.R/PSPT HK1 ZB400522510/TJK/24JUL85/$(surname2)/$(name2)/M
.RN/-1$(surname2)/$(name2)
.R/FOID PPZB400522510-1$(surname2)/$(name2)
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro


$(defmacro PREPARE_FLIGHT_5
    airl1=ЮТ
    flt1=103
    depp1=ДМД
    arrp1=ПЛК
    airl2=С7
    flt2=1027
    depp2=ПЛК
    arrp2=СОЧ
    surname1=REPIN
    name1=IVAN
    tickno1=2982401841689
    cpnno1=1
    surname2=PETROV
    name2=PETR
    tickno2=2982401841612
    cpnno2=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(create_spp $(ddmmyyyy +0))

<<
$(INBOUND_PNL_5 $(airl1) $(depp1) $(arrp1) $(flt1)
                $(airl2) $(depp2) $(arrp2) $(flt2)
                $(surname1) $(name1) $(tickno1) $(cpnno1)
                $(surname2) $(name2) $(tickno2) $(cpnno2))

$(create_random_trip_comp $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)) Э)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
TKT+$(tickno1)"
UNT+5+1"
UNZ+1+$(last_edifact_ref 1)0001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 0)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 0)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
TKT+$(tickno2)"
UNT+5+1"
UNZ+1+$(last_edifact_ref 0)0001"

<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+$(surname1)+$(name1)"
TAI+0162"
RCI+UA:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+$(depp1)+$(arrp1)"
ORG+UT:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+$(tickno1):T:1:3"
CPN+$(cpnno1):I"
TVL+$(ddmmyy):2205+$(depp1)+$(arrp1)+$(airl1)+$(flt1):Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+$(surname2)+$(name2)"
TAI+0162"
RCI+UA:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+$(depp1)+$(arrp1)"
ORG+UT:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+$(tickno2):T:1:3"
CPN+$(cpnno2):I"
TVL+$(ddmmyy):2205+$(depp1)+$(arrp1)+$(airl1)+$(flt1):Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+$(last_edifact_ref)0001"


}) #end-of-macro


$(defmacro OPEN_CHECKIN
    point_id
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(point_id)</point_id>
          <tripstages>
            <stage>
              <stage_id>10</stage_id>
              <act>$(date_format %d.%m.%Y -24h) 00:41:00</act>
            </stage>
            <stage>
              <stage_id>20</stage_id>
              <act>$(date_format %d.%m.%Y -24h) 00:41:00</act>
            </stage>
          </tripstages>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>}

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED' code='0'>Данные успешно сохранены</message>
    </command>

}) #end-of-macro


$(defmacro SEARCH_EMD_BY_DOC_NO
    point_dep
    emd_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='EMDSearch' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchEMDByDocNo>
       <point_id>$(point_dep)</point_id>
       <EmdNoEdit>$(emd_no)</EmdNoEdit>
     </SearchEMDByDocNo>
   </query>
 </term>}

}) #end-of-macro


$(defmacro SEARCH_ET_BY_TICK_NO
    point_dep
    tick_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchETByTickNo>
      <point_id>$(point_dep)</point_id>
      <TickNoEdit>$(tick_no)</TickNoEdit>
    </SearchETByTickNo>
  </query>
</term>}

}) #end-of-macro


$(defmacro EMD_TEXT_VIEW
    point_dep
    tick_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='EMDSearch' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <EMDTextView>
       <point_id>$(point_dep)</point_id>
       <ticket_no>$(tick_no)</ticket_no>
       <pax_id/>
       <coupon_no/>
       <ticket_rem/>
     </EMDTextView>
   </query>
 </term>}

}) #end-of-macro


$(defmacro SAVE_ET_DISP
    point_id
    tickno=2986120030297
    surname=REPIN
    name=IVAN
    airl=ЮТ
    dcs_addr=UTDC
    ets_addr=UTET
    recloc=G4LK6W
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchETByTickNo>
      <point_id>$(point_id)</point_id>
      <TickNoEdit>$(tickno)</TickNoEdit>
    </SearchETByTickNo>
  </query>
</term>}

>>
UNB+SIRE:1+$(dcs_addr)+$(ets_addr)+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+$(airl):МОВ++++Y+::RU+МОВРОМ"
TKT+$(tickno)"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+$(ets_addr)+$(dcs_addr)+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+$(surname)+$(name)"
TAI+0162"
RCI+$(airl):$(recloc):1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+DME+LED"
ORG+UT:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+$(tickno):T:1:3"
CPN+1:I"
TVL+$(ddmmyy):2205+DME+LED+$(airl)+103:Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

}
) #end-of-macro

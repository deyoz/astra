include(ts/macro.ts)

# meta: suite eticket

$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_ONE_FLIGHT UT DME LED 103)


{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchETByTickNo>
       <point_id>$(last_point_id_spp)</point_id>
       <TickNoEdit>2982348111616</TickNoEdit>
     </SearchETByTickNo>
   </query>
 </term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+ANDERSON:766+DYLAN"
TAI+0162"
RCI+UA:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++131212+++:US"
ODI+DME+DME"
ORG+UT:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+2982348111616:T:1:3"
CPN+1:AL"
TVL+201212:2205+DME+SGC+UT+257:Y+J"
RPI++NS"
PTS++YINF"
CPN+2:AL"
TVL+271212:0710+SGC+DME+UT+258:Y+J"
RPI++NS"
PTS++YINF"
TKT+2982348111616:T:1:4::2982121212132"
CPN+1:::::::1::702"
PTS++Y2"
TKT+2982348111616:T:1:4::2982121212122"
CPN+1:::::::1::703"
PTS++Y2"
UNT+29+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...


!! capture=on err=ignore
$(lastRedisplay)

>> lines = auto
{        <ticket2>
          <tick_num>2982348111616</tick_num>
          <connected_doc_num>2982121212132</connected_doc_num>
          <coupon>
            <row index='0'>
              <num index='0'>1</num>
              <connected_num index='1'>1</connected_num>
              <connection_status index='2'>702</connection_status>
            </row>
          </coupon>
        </ticket2>
        <ticket3>
          <tick_num>2982348111616</tick_num>
          <connected_doc_num>2982121212122</connected_doc_num>
          <coupon>
            <row index='0'>
              <num index='0'>1</num>
              <connected_num index='1'>1</connected_num>
              <connection_status index='2'>703</connection_status>
            </row>
          </coupon>
        </ticket3>}



%%
###############################################################################

$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_ONE_FLIGHT UT DME LED 103)


{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchETByTickNo>
       <point_id>$(last_point_id_spp)</point_id>
       <TickNoEdit>2982348111616</TickNoEdit>
     </SearchETByTickNo>
   </query>
 </term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+ANDERSON:766+DYLAN"
TAI+0162"
RCI+UA:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++131212+++:US"
ODI+DME+DME"
ORG+UT:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+2982348111616:T:1:3"
CPN+1:AL"
TVL+201212:2205+DME+SGC+UT+257:Y+J"
RPI++NS"
PTS++YINF"
CPN+2:AL"
TVL+271212:0710+SGC+DME+UT+258:Y+J"
RPI++NS"
PTS++YINF"
CPN+3:AL"
TVL+281212:0710+DME+LED+UT+268:Y+J"
RPI++NS"
PTS++YINF"
CPN+4:AL"
TVL+271212:0710+LED+SGC+UT+278:Y+J"
RPI++NS"
PTS++YINF"
TKT+2982348111616:T:1:4::2982121212132"
CPN+1:::::::1::702"
PTS++Y2"
TKT+2982348111616:T:1:4::2982121212133"
CPN+2:::::::1::702"
PTS++Y2"
TKT+2982348111616:T:1:4::2982121212134"
CPN+3:::::::1::702"
PTS++Y2"
TKT+2982348111616:T:1:4::2982121212135"
CPN+3:::::::1::702"
PTS++Y2"
UNT+45+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...


!!
$(lastRedisplay)


%%
#########################################################################################

$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_ONE_FLIGHT UT DME LED 103)

{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchETByTickNo>
       <point_id>$(last_point_id_spp)</point_id>
       <TickNoEdit>2982348111616</TickNoEdit>
     </SearchETByTickNo>
   </query>
 </term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+160409:1627+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
EQN+1:TD"
TIF+REPIN:IN+IVAN"
TAI+4465+887:B"
RCI+1H:T777W7:1+UT:1613F6:1"
MON+B:FREE+T:FREE"
FOP+MS:3:::::::::::PK/CA5469670000006655"
PTK+++040416"
ODI+SGC+SGC"
ORG+1H:MOW+92123301:03СУР+SGC++T+RU+887+СУРМ28"
EQN+2:TF"
IFT+4:15:0+SGC UT TJM UT SGC0.00RUB0.00END"
IFT+4:10+НДСА/К0.00 НДСZZ0.00/UT-119 ПРОХЛ НАПИТКИ/UT-102 ПРОХЛ НАПИТКИ"
IFT+4:39+СУРГУТ+ООО ЗАПАДНО СИБИРСКОЕ АВС"
IFT+4:5+-734677004+79222548183"
TKT+2986147221707:T:1:3"
CPN+1:I::E"
TVL+090416:2220+SGC+TJM+UT+119:Q++1"
RPI++NS"
PTS++QUT/IN"
EBD++1::N"
CPN+3:I::E"
TVL++TJM+SGC+UT+OPEN:Q++2"
RPI++NS"
PTS++QUT/IN"
EBD++1::N"
UNT+28+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...


!! capture=on
$(lastRedisplay)

>> lines=auto
              <dep_date index='1'>OPEN</dep_date>
              <dep_time index='2'>----</dep_time>
              <dep index='3'>TJM</dep>
              <arr index='4'>SGC</arr>
              <codea index='5'>UT</codea>
              <codea_oper index='6'>UT</codea_oper>
              <flight index='7'>OPEN</flight>
              <flight_oper index='8'>OPEN</flight_oper>

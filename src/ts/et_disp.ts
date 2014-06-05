# meta: suite eticket

$(defmacro login
  user=PIKE
  passwd=PIKE
{
{<?xml version='1.0' encoding='UTF-8'?>
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
) # end of defmacro

$(init_jxt_pult åéÇêéå)
$(login)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchETByTickNo>
       <point_id>2276899</point_id>
       <TickNoEdit>0162348111616</TickNoEdit>
     </SearchETByTickNo>
   </query>
 </term>}

>>
UNB+SIRE:1+ASTRA+ETICK+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ûí:åéÇ++++Y+::RU+åéÇêéå"
TKT+0162348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UT+1H+000000:0000+$(last_edifact_ref)0001+++T"
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
TKT+0162348111616:T:1:3"
CPN+1:AL"
TVL+201212:2205+DME+SGC+UT+257:Y+J"
RPI++NS"
PTS++YINF"
CPN+2:AL"
TVL+271212:0710+SGC+DME+UT+258:Y+J"
RPI++NS"
PTS++YINF"
TKT+0162348111616:T:1:4::2982121212132"
CPN+1:::::::1::702"
PTS++Y2"
TKT+0162348111616:T:1:4::2982121212122"
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
          <tick_num>0162348111616</tick_num>
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
          <tick_num>0162348111616</tick_num>
          <connected_doc_num>2982121212122</connected_doc_num>
          <coupon>
            <row index='0'>
              <num index='0'>1</num>
              <connected_num index='1'>1</connected_num>
              <connection_status index='2'>703</connection_status>
            </row>
          </coupon>
        </ticket3>}

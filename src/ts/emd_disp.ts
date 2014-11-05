include(ts/macro.ts)

#meta: suite emd

$(init_jxt_pult åéÇêéå)
$(login)
$(init_eds ûí UTET UTDC)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='EMDSearch' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchEMDByDocNo>
       <point_id>2276899</point_id>
       <EmdNoEdit>2982348111616</EmdNoEdit>
     </SearchEMDByDocNo>
   </query>
 </term>}


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+ûí:åéÇ++++Y+::RU+åéÇêéå"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++O"
UNH+1+TKCRES:03:2:IA+$(last_edifact_ref)"
MSG+:791+3"
TIF+TESTOVA:F+ANNA"
TAI+5235+6VT:B"
RCI+1H:00D5LW:1+UT:045PCC:1"
MON+B:150:TWD+T:165:TWD"
FOP+CA:3"
PTK+++$(ddmmyy)"
ORG+1H:MOW+00117165:01TCH+MOW++N+RU+7+TCH08"
EQN+1:TD"
TXD++15:::TW"
IFT+4:15:0+KHH UT X/BKI KUL 150 END"
IFT+4:39+MOSCOW+TCH"
IFT+4:733:0"
PTS+++++C"
TKT+2982121212122:J:1"
CPN+1:I::E"
TVL+090512+KHH+BKI+UT+121"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
CPN+2:I::E"
TVL+100512+BKI+KUL+UT+212"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
TKT+2982121212122:J::4::2981614567890"
CPN+1:::::::1::702"
PTS++Y2"
CPN+2:::::::2::702"
PTS++Y2"
UNT+32+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick...


!! capture=on err=ignore
$(lastRedisplay)

>> lines=auto
    <passenger>
      <surname>TESTOVA</surname>
      <kkp>F</kkp>
      <age/>
      <name>ANNA</name>
    </passenger>
    <recloc>
      <awk>UT</awk>
      <regnum>00D5LW</regnum>
    </recloc>
    <origin>
      <date_of_issue>...
      <sys_addr>MOW 1H</sys_addr>
      <ppr>00117165</ppr>
      <agn>01TCH</agn>
      <opr_flpoint>MOW</opr_flpoint>
      <authcode>7</authcode>
      <pult>TCH08</pult>
      <city_name>MOSCOW</city_name>
      <agency_name>TCH</agency_name>
    </origin>
    <foid/>
    <payment>
      <fare>0.00</fare>
      <total>0.00</total>
      <payment>0.00 CA</payment>
      <tax>TW15</tax>
      <fare_calc>KHH UT X/BKI KUL 150 END</fare_calc>
    </payment>
    <rfic>C</rfic>
    <emd_type>A</emd_type>
    <emd1>
      <emd_num>2982121212122</emd_num>
      <coupon refresh='true'>
        <row index='0'>
          <num index='0'>1</num>
          <associated_num index='1'>1</associated_num>
          <associated_doc_num index='2'>2981614567890</associated_doc_num>
          <association_status index='3'>702</association_status>
          <dep_date index='4'>090512</dep_date>
          <dep_time index='5'>----</dep_time>
          <dep index='6'>KHH</dep>
          <arr index='7'>BKI</arr>
          <codea index='8'>UT</codea>
          <flight index='9'>121</flight>
          <amount index='10'>0.00</amount>
          <rfisc_code index='11'>99K</rfisc_code>
          <rfisc_desc index='12'>BAGGAGE - EXCESS WEIGHT</rfisc_desc>
          <sac index='13'/>
          <coup_status index='14'>O</coup_status>
        </row>
        <row index='1'>
          <num index='0'>2</num>
          <associated_num index='1'>2</associated_num>
          <associated_doc_num index='2'>2981614567890</associated_doc_num>
          <association_status index='3'>702</association_status>
          <dep_date index='4'>100512</dep_date>
          <dep_time index='5'>----</dep_time>
          <dep index='6'>BKI</dep>
          <arr index='7'>KUL</arr>
          <codea index='8'>UT</codea>
          <flight index='9'>212</flight>
          <amount index='10'>0.00</amount>
          <rfisc_code index='11'>99K</rfisc_code>
          <rfisc_desc index='12'>BAGGAGE - EXCESS WEIGHT</rfisc_desc>
          <sac index='13'/>
          <coup_status index='14'>O</coup_status>
        </row>
      </coupon>
    </emd1>

%%
###############################################################################


$(init_jxt_pult åéÇêéå)
$(login)
$(init_eds ûí UTET UTDC)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='EMDSearch' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchEMDByDocNo>
       <point_id>2276899</point_id>
       <EmdNoEdit>2982348111616</EmdNoEdit>
     </SearchEMDByDocNo>
   </query>
 </term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+ûí:åéÇ++++Y+::RU+åéÇêéå"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

# Ø‡®ËÒ´ Ø®≠Æ™ Æ‚ Æ°‡†°Æ‚Á®™† ‚†©¨†„‚† edifact
>> lines=auto
    <kick...

#!! capture=on err=ignore
#$(lastRedisplay)

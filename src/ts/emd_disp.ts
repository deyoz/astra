include(ts/macro.ts)

#meta: suite emd

$(init_jxt_pult ������)
$(login)
$(init_eds �� UTET UTDC TADDR OADDR)

$(PREPARE_FLIGHT_1 �� 103 ��� ��� REPIN IVAN)

$(SEARCH_EMD_BY_DOC_NO $(last_point_id_spp) 2982348111616)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+��:���++++Y+::RU+������"
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
FOP+CA:3:165"
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
      <fare>150TWD</fare>
      <total>165TWD</total>
      <payment>165 CA</payment>
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
          <service_quantity index='12'>1</service_quantity>
          <luggage index='13'>25��</luggage>
          <rfisc_desc index='14'>BAGGAGE - EXCESS WEIGHT</rfisc_desc>
          <sac index='15'/>
          <coup_status index='16'>O</coup_status>
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
          <service_quantity index='12'>1</service_quantity>
          <luggage index='13'>25��</luggage>
          <rfisc_desc index='14'>BAGGAGE - EXCESS WEIGHT</rfisc_desc>
          <sac index='15'/>
          <coup_status index='16'>O</coup_status>
        </row>
      </coupon>
    </emd1>

%%
###############################################################################


$(init_jxt_pult ������)
$(login)
$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1 �� 103 ��� ��� REPIN IVAN)

$(SEARCH_EMD_BY_DOC_NO $(last_point_id_spp) 2982348111616)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+��:���++++Y+::RU+������"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

# ���� ����� �� ��ࠡ��稪� ⠩���� edifact
>> lines=auto
    <kick...

#!! capture=on err=ignore
#$(lastRedisplay)


%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1 �� 103 ��� ��� REPIN IVAN)

$(SEARCH_EMD_BY_DOC_NO $(last_point_id_spp) 2988200000386)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+��:���++++Y+::RU+������"
TKT+2988200000386"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+150323:1342+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:791+3"
TIF+IVANO+VIA"
TAI+2984+99���:B"
RCI+1H:09SKR6:1"
MON+B:500.00:RUB+T:500.00:RUB"
FOP+CA:3:500.00"
PTK+++230315"
ORG+1H:MOW+00117165:01TCH+MOW++N+RU+7+TCH08"
EQN+1:TD"
IFT+4:15:1"
IFT+4:41+01���76"
IFT+4:733:0"
PTS+++++A"
TKT+2988200000386:Y:1"
CPN+1:I:500.00:E"
PTS++++++0BW"
IFT+4:47+REGISTRATION"
UNT+19+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <passenger>
      <surname>IVANO</surname>
      <kkp/>
      <age/>
      <name>VIA</name>
    </passenger>
    <recloc>
      <awk>1H</awk>
      <regnum/>
    </recloc>
    <origin>
      <date_of_issue>...
      <sys_addr>MOW 1H</sys_addr>
      <ppr>00117165</ppr>
      <agn>01TCH</agn>
      <opr_flpoint>MOW</opr_flpoint>
      <authcode>7</authcode>
      <pult>TCH08</pult>
    </origin>
    <foid/>
    <payment>
      <fare>500.00RUB</fare>
      <total>500.00RUB</total>
      <payment>500.00 CA</payment>
      <tax/>
      <fare_calc/>
    </payment>
    <rfic>A</rfic>
    <emd_type>S</emd_type>
    <emd1>
      <emd_num>2988200000386</emd_num>
      <coupon refresh='true'>
        <row index='0'>
          <num index='0'>1</num>
          <dep_date index='1'> </dep_date>
          <dep_time index='2'> </dep_time>
          <dep index='3'> </dep>
          <arr index='4'> </arr>
          <codea index='5'> </codea>
          <flight index='6'> </flight>
          <amount index='7'>500.00</amount>
          <rfisc_code index='8'>0BW</rfisc_code>
          <service_quantity index='9'>1</service_quantity>
          <luggage index='10'>-</luggage>
          <rfisc_desc index='11'>REGISTRATION</rfisc_desc>
          <sac index='12'/>
          <coup_status index='13'>O</coup_status>
        </row>
      </coupon>
    </emd1>
  </answer>
</term>


%%
#########################################################################################

$(init_jxt_pult ������)
$(login)
$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1 �� 103 ��� ��� REPIN IVAN)

$(SEARCH_EMD_BY_DOC_NO $(last_point_id_spp) 2988200000386)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+��:���++++Y+::RU+������"
TKT+2988200000386"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+160527:1125+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:791+3"
TIF+����+����"
TAI+2984+99���:B"
RCI+1H:09FB3W:1+UT:084M5S:1"
MON+B:0.00:RUB+T:0.00:RUB"
FOP+CA:3"
PTK+++160516"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1471+������"
EQN+1:TD"
IFT+4:15:0"
IFT+4:45+2982408014079/���/16���16/29842300"
IFT+4:8+71+������� �� ������������� �������(��)"
IFT+4:10+���"
IFT+4:41+01���80"
IFT+4:5+74951234567"
IFT+4:23+���-��� YQ=650���+ZZ=0.00���+���-���-��� �����=1100���"
PTS+++++I"
TKT+2988200000386:Y:1"
CPN+1:B::E::::::6"
TVL+180516:0815+VKO+TJM+UT+700"
PTS++++++FNA"
IFT+4:47+������� �� ���. �������(��)"
TKT+2988200000386:Y::4::2982408014079"
UNT+25+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick...

!! capture=on
$(lastRedisplay)

>> lines=auto
    <passenger>
      <surname>����</surname>
      <kkp/>
      <age/>
      <name>����</name>
    </passenger>
    <recloc>
      <awk>UT</awk>
      <regnum>09FB3W</regnum>
    </recloc>
    <origin>
      <date_of_issue>16���16</date_of_issue>
      <sys_addr>MOW 1H</sys_addr>
      <ppr>29842300</ppr>
      <agn>99���</agn>
      <opr_flpoint>MOW</opr_flpoint>
      <authcode>1471</authcode>
      <pult>������</pult>
    </origin>
    <foid/>
    <payment>
      <fare>0.00RUB</fare>
      <total>0.00RUB</total>
      <payment>0.00 CA</payment>
      <tax/>
      <fare_calc/>
    </payment>
    <rfic>I</rfic>
    <emd_type>S</emd_type>
    <emd1>
      <emd_num>2988200000386</emd_num>
      <coupon refresh='true'>
        <row index='0'>
          <num index='0'>1</num>
          <dep_date index='1'>180516</dep_date>
          <dep_time index='2'>0815</dep_time>
          <dep index='3'>VKO</dep>
          <arr index='4'>TJM</arr>
          <codea index='5'>UT</codea>
          <flight index='6'>700</flight>
          <amount index='7'>0.00</amount>
          <rfisc_code index='8'>FNA</rfisc_code>
          <service_quantity index='9'>1</service_quantity>
          <luggage index='10'>-</luggage>
          <rfisc_desc index='11'>������� �� ���. �������(��)</rfisc_desc>
          <sac index='12'/>
          <coup_status index='13'>F</coup_status>
        </row>
      </coupon>
    </emd1>

include(ts/macro.ts)

# meta: suite eticket

$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103   REPIN IVAN)

$(EMD_TEXT_VIEW $(last_point_id_spp) 2982348111616)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+1H:++++Y+::RU+"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+121212:1212+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:I+$(last_edifact_ref)"
MSG+:131+3"
ORG+1H:MOW+00117165:01+MOW++N+RU+7+08"
TAI+2984"
RCI+1H:00D4LW:1+UT:043PCC:1"
PTK+++$(ddmmyy)"
EQN+1:TD"
IFT+4:39++"
TIF+:F+"
MON+B:60.00:RUB+T:220.00:RUB"
FOP+MS:3:220.00"
EQN+1:TF"
TXD++160.00:::"
IFT+4:15:0+MOW UT SGC30.00UT MOW30.00RUB60.00END"
TKT+2982348111616:T:1:3"
CPN+1:I::E"
TVL+090512:1925+KHH+BKI+UT+121:N++1"
RPI++OK"
PTS++"
EBD++20::W:K"
TKT+2982348111616:T::4::2982121212122"
CPN+1:::::::1::703"
PTS++Y2++++99K"
TKT+2982348111616:T:1:4::2982121212132"
CPN+1:::::::1::702"
PTS++Y2++++99K"
UNT+29+1"
UNZ+1+$(last_edifact_ref)0001"


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:791"
ORG+1H:++++Y+::RU+"
TKT+2982121212122"
UNT+5+1"
UNZ+1+$(last_edifact_ref 1)0001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+1H:++++Y+::RU+"
TKT+2982121212132"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+121212:1212+$(last_edifact_ref 1)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref 1)"
MSG+:791+3"
TIF+:F+"
TAI+5235+6VT:B"
RCI+1H:00D5LW:1+UT:045PCC:1"
MON+B:150:TWD+T:165:TWD"
FOP+CA:3"
PTK+++$(ddmmyy)"
ORG+1H:MOW+00117165:01+MOW++N+RU+7+08"
EQN+1:TD"
TXD++15:::TW"
IFT+4:15:0+KHH UT X/BKI KUL 150 END"
IFT+4:39++"
IFT+4:733:0"
PTS+++++C"
TKT+2982121212122:J:1"
CPN+1:I::E"
TVL+$(ddmmyy +1)+KHH+BKI+UT+121"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
CPN+2:I::E"
TVL+$(ddmmyy +2)+BKI+KUL+UT+212"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
TKT+2982121212122:J::4::2982348111616"
CPN+1:::::::1::702"
PTS++Y2"
CPN+2:::::::2::702"
PTS++Y2"
UNT+32+1"
UNZ+1+$(last_edifact_ref 1)0001"

<<
UNB+SIRE:1+UTET+UTDC+121212:1212+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:791+3"
TIF+:F+"
TAI+5235+6VT:B"
RCI+1H:00D5LW:1+UT:045PCC:1"
MON+B:150:TWD+T:165:TWD"
FOP+CA:3"
PTK+++$(ddmmyy)"
ORG+1H:MOW+00117165:01+MOW++N+RU+7+08"
EQN+1:TD"
TXD++15:::TW"
IFT+4:15:0+KHH UT X/BKI KUL 150 END"
IFT+4:39++"
IFT+4:733:0"
PTS+++++C"
TKT+2982121212132:J:1"
CPN+1:I::E"
TVL+$(ddmmyy +1)+KHH+BKI+UT+121"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
CPN+2:I::E"
TVL+$(ddmmyy +2)+BKI+KUL+UT+212"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
TKT+2982121212132:J::4::2982348111616"
CPN+1:::::::1::702"
PTS++Y2"
CPN+2:::::::2::702"
PTS++Y2"
UNT+32+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)


>> lines=auto
    <text>PNR: 045PCC/UT
āŪĪ Ķ : $()
   â : $(ddmonyy +0 ru)
  ĪāĨá áĻáâĨŽë: MOW 1H
  ĢĨ­âáâĒŪ: 01 
  ã­Šâ ŊāŪĪ ĶĻ: 00117165
  ŪāŪĪ: MOW 
  ŊĨā âŪā:...
  ãŦėâ:...
 áá ĶĻā: $()
   ŽĻŦĻï: 
  Žï: 
   âĨĢŪāĻï: F
  Ū§ā.: $()
ĻŊ EMD: A
RFIC: C
$()
EMD$(sharp)2982121212122: $()
  ü ŠãŊ.  â    āĨŽï âŊā.  §­. /Š ĨĐá ãŽŽ  RFISC ŪŦ-ĒŪ ŊŦ.  §Ē ­ĻĨ ãáŦãĢĻ         âãŊ. ááŪæ.          âááŪæ. $()
  1      $(ddmmyy +1) ----  KHH   BKI   UT  121  0.00  99K   1      25 BAGGAGE - EXCESS WEIGHT O      2982348111616/1 702      $()
  2      $(ddmmyy +2) ----  BKI   KUL   UT  212  0.00  99K   1      25 BAGGAGE - EXCESS WEIGHT O      2982348111616/2 702      $()
ŊŦ â : $()
   āĻä: 150TWD
  ĄŪāë: TW15
  áĨĢŪ: 165TWD
  ŊŦ â : 0.00 CA
====================================================================================================
PNR: 045PCC/UT
āŪĪ Ķ : $()
   â : $(ddmonyy +0 ru)
  ĪāĨá áĻáâĨŽë: MOW 1H
  ĢĨ­âáâĒŪ: 01 
  ã­Šâ ŊāŪĪ ĶĻ: 00117165
  ŪāŪĪ: MOW 
  ŊĨā âŪā:...
  ãŦėâ:...
 áá ĶĻā: $()
   ŽĻŦĻï: 
  Žï: 
   âĨĢŪāĻï: F
  Ū§ā.: $()
ĻŊ EMD: A
RFIC: C
$()
EMD$(sharp)2982121212132: $()
  ü ŠãŊ.  â    āĨŽï âŊā.  §­. /Š ĨĐá ãŽŽ  RFISC ŪŦ-ĒŪ ŊŦ.  §Ē ­ĻĨ ãáŦãĢĻ         âãŊ. ááŪæ.          âááŪæ. $()
  1      $(ddmmyy +1) ----  KHH   BKI   UT  121  0.00  99K   1      25 BAGGAGE - EXCESS WEIGHT O      2982348111616/1 702      $()
  2      $(ddmmyy +2) ----  BKI   KUL   UT  212  0.00  99K   1      25 BAGGAGE - EXCESS WEIGHT O      2982348111616/2 702      $()
ŊŦ â : $()
   āĻä: 150TWD
  ĄŪāë: TW15
  áĨĢŪ: 165TWD
  ŊŦ â : 0.00 CA
====================================================================================================
</text>


#########################################################################################
%%
# test 2 - âŪŦėŠŪ 

$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103   REPIN IVAN)

$(EMD_TEXT_VIEW $(last_point_id_spp) 2982348111616)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+1H:++++Y+::RU+"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+121212:1212+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:I+$(last_edifact_ref)"
MSG+:131+3"
ORG+1H:MOW+00117165:01+MOW++N+RU+7+08"
TAI+2984"
RCI+1H:00D4LW:1+UT:043PCC:1"
PTK+++$(ddmmyy)"
EQN+1:TD"
IFT+4:39++"
TIF+:F+"
MON+B:60.00:RUB+T:220.00:RUB"
FOP+MS:3:220.00"
EQN+1:TF"
TXD++160.00:::"
IFT+4:15:0+MOW UT SGC30.00UT MOW30.00RUB60.00END"
TKT+2982348111616:T:1:3"
CPN+1:I::E"
TVL+090512:1925+KHH+BKI+UT+121:N++1"
RPI++OK"
PTS++"
EBD++20::W:K"
UNT+23+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)


# ŊãáâŪ
>> lines=auto
    <text/>


#########################################################################################
%%
# test 3 - â ĐŽ ãâ ­  íâ ŊĨ ĪĻáŊŦĨï 

$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103   REPIN IVAN)

$(EMD_TEXT_VIEW $(last_point_id_spp) 2982348111616)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+1H:++++Y+::RU+"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...


!! capture=on err=ignore
$(lastRedisplay)


>> lines=auto
      <error lexema_id='MSG.ETS_EDS_CONNECT_ERROR' code='0'>Ĩâ áĒï§Ļ á áĨāĒĨāŪŽ íŦ. ĄĻŦĨâŪĒ ĻŦĻ áĨāĒĨāŪŽ íŦ. ĪŪŠãŽĨ­âŪĒ</error>...


#########################################################################################
%%
# test 4 - â ĐŽ ãâ ­  íâ ŊĨ ĪĻáŊŦĨï 


$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103   REPIN IVAN)

$(EMD_TEXT_VIEW $(last_point_id_spp) 2982348111616)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+1H:++++Y+::RU+"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+121212:1212+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:I+$(last_edifact_ref)"
MSG+:131+3"
ORG+1H:MOW+00117165:01+MOW++N+RU+7+08"
TAI+2984"
RCI+1H:00D4LW:1+UT:043PCC:1"
PTK+++$(ddmmyy)"
EQN+1:TD"
IFT+4:39++"
TIF+:F+"
MON+B:60.00:RUB+T:220.00:RUB"
FOP+MS:3:220.00"
EQN+1:TF"
TXD++160.00:::"
IFT+4:15:0+MOW UT SGC30.00UT MOW30.00RUB60.00END"
TKT+2982348111616:T:1:3"
CPN+1:I::E"
TVL+090512:1925+KHH+BKI+UT+121:N++1"
RPI++OK"
PTS++"
EBD++20::W:K"
TKT+2982348111616:T:1:4::2982121212122"
CPN+1:::::::1::703"
PTS++Y2++++99K"
TKT+2982348111616:T:1:4::2982121212132"
CPN+1:::::::1::702"
PTS++Y2++++99K"
UNT+29+1"
UNZ+1+$(last_edifact_ref)0001"


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:791"
ORG+1H:++++Y+::RU+"
TKT+2982121212122"
UNT+5+1"
UNZ+1+$(last_edifact_ref 1)0001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:791"
ORG+1H:++++Y+::RU+"
TKT+2982121212132"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UT+1H+121212:1212+$(last_edifact_ref 1)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref 1)"
MSG+:791+3"
TIF+:F+"
TAI+5235+6VT:B"
RCI+1H:00D5LW:1+UT:045PCC:1"
MON+B:150:TWD+T:165:TWD"
FOP+CA:3"
PTK+++$(ddmmyy)"
ORG+1H:MOW+00117165:01+MOW++N+RU+7+08"
EQN+1:TD"
TXD++15:::TW"
IFT+4:15:0+KHH UT X/BKI KUL 150 END"
IFT+4:39++"
IFT+4:733:0"
PTS+++++C"
TKT+2982121212122:J:1"
CPN+1:I::E"
TVL+$(ddmmyy +1)+KHH+BKI+UT+121"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
CPN+2:I::E"
TVL+$(ddmmyy +2)+BKI+KUL+UT+212"
PTS++++++99K"
EBD++25::W:K"
IFT+4:47+BAGGAGE - EXCESS WEIGHT"
TKT+2982121212122:J::4::2982348111616"
CPN+1:::::::1::702"
PTS++Y2"
CPN+2:::::::2::702"
PTS++Y2"
UNT+32+1"
UNZ+1+$(last_edifact_ref 1)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...


!! capture=on err=ignore
$(lastRedisplay)


>> lines=auto
      <error lexema_id='MSG.ETS_EDS_CONNECT_ERROR' code='0'>Ĩâ áĒï§Ļ á áĨāĒĨāŪŽ íŦ. ĄĻŦĨâŪĒ ĻŦĻ áĨāĒĨāŪŽ íŦ. ĪŪŠãŽĨ­âŪĒ</error>...


#########################################################################################
%%
# test 5 - ĄĻŦĨâ ­Ĩ ­ ĐĪĨ­


$(init_jxt_pult )
$(login)
$(init_eds  UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG  103   REPIN IVAN)

$(EMD_TEXT_VIEW $(last_point_id_spp) 2982348111616)


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+1H:++++Y+::RU+"
TKT+2982348111616"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+121212:1212+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:I+$(last_edifact_ref)"
MSG+:131+7"
ERC+401"
IFT+3+/     "
UNT+5+1"
UNZ+1+0001U2VWHL0001"

$(KICK_IN)

>> lines=auto
    <command>
      <user_error lexema_id='MSG.ETICK.ETS_ERROR' code='0'>:  /     </user_error>
    </command>

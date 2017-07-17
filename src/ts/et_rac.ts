include(ts/macro.ts)

# meta: suite eticket

$(init_jxt_pult Œ‚Œ)
$(login)
$(init_eds ’ UTET UTDC)

$(PREPARE_FLIGHT_1 ’ 103 „Œ„ ‹Š REPIN IVAN)


$(REQUEST_AC_BY_TICK_NO_CPN_NO $(last_point_id_spp) 2982348111616 1)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:734"
ORG+’:Œ‚++++Y+::RU+Œ‚Œ"
EQN+1:TD"
TKT+2982348111616:T:1:3"
CPN+1:AL::E"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+170706:0859+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:751+3"
EQN+1:TD"
TIF+KOLLEROV:A+DANILA"
TAI+7906+WS/SU:B"
RCI+1A:SKWZAE:1+5N:033DC7:1"
MON+G:00010+T:4150:RUB+B:2450:RUB"
FOP+CA:3:4150"
PTK+++$(ddmmyy -100)"
ODI+MOW+REN"
ORG+1A:MUC+92229196:121255+MOW++A++SYSTEM"
EQN+1:TF"
TXD+700+1500:::YQ+200:::YR"
IFT+4:39+RUSSIA+WHITE TREVEL"
IFT+4:5+79033908109"
IFT+4:10+CHNG BEF DEP/REFUND RESTR"
IFT+4:15:0+MOW 5N REN2450.00RUB2450.00END"
TKT+2982348111616:T:1:3"
CPN+1:I::E"
TVL+$(ddmmyy):0150+DME+LED+UT+103:A++1"
TVL+$(ddmmyy):0150+DME+LED+:UR+103:A++1"
RPI++OK"
PTS++APROW"
EBD++1::N"
DAT+B:$(ddmmyy)+A:$(ddmmyy)"
UNT+26+1"
UNZ+1+$(last_edifact_ref)0001"

$(dump_table WC_PNR)
$(dump_table WC_TICKET)
$(dump_table WC_COUPON)
$(dump_table AIRPORT_CONTROLS)

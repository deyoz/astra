
#########################################################################################
### отдача контроля ЭБ ими

$(defmacro UAC_REQUEST
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  surname
  name
  pnr_addr
  subcl
  airline
  flt_no
  airp_dep
  date_dep #формат: ddmmyy
  time_dep #формат: hhnn
  airp_arv
{
<<
UNB+SIRE:1+$(ets_addr)+$(dcs_addr)+$(yymmdd):$(hhmi)+$(ediref)0001++RESR1+O"
UNH+4028+TKCUAC:01:1:IA+010000"
MSG+:733"
ORG+SU:ATH+:555000000+++A++SYSTEM"
TAI+0011+AWS:B"
RCI+$(airline):$(pnr_addr):1"
EQN+1:TD"
TIF+$(surname)+$(name)"
MON+B:123.00:EUR+T:123.00:EUR"
FOP+CC:3::CA:5566254140291582:0119:709755:M"
PTK+X::I++$(date_dep)+++:GR"
ODI+$(airp_dep)+$(airp_arv)"
ORG+1S:ATH+27213082:13OG+ATH++T+GR+AWS"
EQN+1:TF"
IFT+4:10+NONREF/HEBO3BPATEH"
IFT+4:15:02+VVO SU TYO138.04NUC138.04END ROE0.891032"
IFT+4:39+ATHENS        GR+ETRAVEL"
TKT+$(ticket_no):T:1:3"
CPN+$(coupon_no):I::E:::I"
TVL+$(date_dep):$(time_dep)+$(airp_dep)+$(airp_arv)+$(airline)+$(flt_no):$(subcl)++1"
PTS++NVO"
DAT+A:$(date_dep)+B:$(date_dep)"
RPI++OK"
EBD++1::N"
UNT+43+4028"
UNZ+1+$(ediref)0001"

})

$(defmacro UAC_RESPONSE_OK
  dcs_addr
  ets_addr
  ediref
{
>>
UNB+SIRE:1+$(dcs_addr)+$(ets_addr)+xxxxxx:xxxx+$(ediref)0001++RESR1+T"
UNH+1+TKCRES:01:1:IA+010000"
MSG+:733+3"
UNT+3+1"
UNZ+1+$(ediref)0001"

})

$(defmacro UAC_OK
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  surname
  name
  pnr_addr
  subcl
  airline
  flt_no
  airp_dep
  date_dep #формат: ddmmyy
  time_dep #формат: hhnn
  airp_arv
{
$(UAC_REQUEST $(dcs_addr) $(ets_addr) $(ediref) $(ticket_no) $(coupon_no) $(surname) $(name) $(pnr_addr) $(subcl)
              $(airline) $(flt_no) $(airp_dep) $(date_dep) $(time_dep) $(airp_arv))
$(UAC_RESPONSE_OK $(dcs_addr) $(ets_addr) $(ediref))
})

#########################################################################################
### запрос контроля ЭБ нами

$(defmacro RAC_REQUEST
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  airline
{
>>
UNB+SIRE:1+$(dcs_addr)+$(ets_addr)+xxxxxx:xxxx+$(ediref)0001+++O"
UNH+1+TKCREQ:00:1:IA+$(ediref)"
MSG+:734"
ORG+1H:MOW+++$(airline)+Y+::EN+MOVROM"
EQN+1:TD"
TKT+$(ticket_no):T:1:3"
CPN+$(coupon_no):AL::E"
UNT+7+1"
UNZ+1+$(ediref)0001"

})

$(defmacro RAC_RESPONSE_OK
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  surname
  name
  pnr_addr
  subcl
  airline
  flt_no
  airp_dep
  date_dep #формат: ddmmyy
  time_dep #формат: hhnn
  airp_arv
{
<<
UNB+IATA:1+$(ets_addr)+$(dcs_addr)+$(yymmdd):$(hhmi)+FEF48066940001+$(ediref)0001+RESR1+T"
UNH+1+TKCRES:00:1:IA+$(ediref)"
MSG+:734+3"
EQN+1:TD"
TIF+$(surname)+$(name)"
TAI+5552+WIC:B"
RCI+$(airline):$(pnr_addr):1"
MON+B:19.00:USD+E:1135:RUB+T:1351:RUB"
FOP+CC:3:1351:CA:5469380075657782:0318:756926:M"
PTK+++$(date_dep)+++:RU"
ODI+$(airp_dep)+$(airp_arv)"
ORG+SU:DSU+92499853:DSU+DSU++A+RU+WS4"
EQN+1:TF"
TXD++164:::YR+22:::RI+30:::RI"
IFT+4:10+P345476893491 APPLICABLE TO ZED/MIBA ZED ONLY/SU EMP CHD/170+M/DOH               07NOV11"
IFT+4:15:66+MCX SU MOW19.00USD19.00END XT30RI"
IFT+4:39+AEROFLOT"
TKT+$(ticket_no):T:1:3"
CPN+$(coupon_no):AL::E:::I"
TVL+$(date_dep):$(time_dep)+$(airp_dep)+$(airp_arv)+$(airline)+$(flt_no):$(subcl)++1"
RPI++SA"
PTS++MIDZL3R2/ZEC"
EBD++1::N"
DAT+A:$(date_dep)"
UNT+24+1"
UNZ+1+FEF48066940001"

})

$(defmacro RAC_RESPONSE_ERROR
  dcs_addr
  ets_addr
  ediref
  error
{
<<
UNB+SIRE:1+$(ets_addr)+$(dcs_addr)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(ediref)"
MSG+:734+7"
ERC+$(error)"
UNT+4+1"
UNZ+1+$(ediref)0001"

})

$(defmacro RAC_OK
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  surname
  name
  pnr_addr
  subcl
  airline
  flt_no
  airp_dep
  date_dep #формат: ddmmyy
  time_dep #формат: hhnn
  airp_arv
{
$(RAC_REQUEST $(dcs_addr) $(ets_addr) $(ediref) $(ticket_no) $(coupon_no) $(airline))
$(RAC_RESPONSE_OK $(dcs_addr) $(ets_addr) $(ediref) $(ticket_no) $(coupon_no) $(surname) $(name) $(pnr_addr) $(subcl)
                  $(airline) $(flt_no) $(airp_dep) $(date_dep) $(time_dep) $(airp_arv))
})

$(defmacro RAC_ERROR
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  airline
  error
{
$(RAC_REQUEST $(dcs_addr) $(ets_addr) $(ediref) $(ticket_no) $(coupon_no) $(airline))
$(RAC_RESPONSE_ERROR $(dcs_addr) $(ets_addr) $(ediref) $(error))
})

#########################################################################################
### запрос контроля ЭБ ими

$(defmacro EXTRACT_AC_REQUEST
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
{
<<
UNB+SIRE:1+$(ets_addr)+$(dcs_addr)+$(yymmdd):$(hhmi)+$(ediref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(ediref)"
MSG+:142"
ORG+SU:ATH+:555000000+++A++SYSTEM"
EQN+1:TD"
TKT+$(ticket_no):T:1"
CPN+$(coupon_no):701"
UNT+7+1"
UNZ+1+$(ediref)0001"

})

$(defmacro EXTRACT_AC_RESPONSE_OK
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  status
{
>>
UNB+SIRE:1+$(dcs_addr)+$(ets_addr)+xxxxxx:xxxx+$(ediref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(ediref)"
MSG+:142+3"
TKT+$(ticket_no):T:1:3"
CPN+$(coupon_no):$(status)::E:::AL"
UNT+5+1"
UNZ+1+$(ediref)0001"

})

$(defmacro EXTRACT_AC_RESPONSE_ERROR
  dcs_addr
  ets_addr
  ediref
  error
{
>>
UNB+SIRE:1+$(dcs_addr)+$(ets_addr)+xxxxxx:xxxx+$(ediref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(ediref)"
MSG+:142+7"
ERC+$(error)"
UNT+4+1"
UNZ+1+$(ediref)0001"

})


$(defmacro EXTRACT_AC_OK
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  status
{
$(EXTRACT_AC_REQUEST $(dcs_addr) $(ets_addr) $(ediref) $(ticket_no) $(coupon_no))
$(EXTRACT_AC_RESPONSE_OK $(dcs_addr) $(ets_addr) $(ediref) $(ticket_no) $(coupon_no) $(status))
})

$(defmacro EXTRACT_AC_ERROR
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  error
{
$(EXTRACT_AC_REQUEST $(dcs_addr) $(ets_addr) $(ediref) $(ticket_no) $(coupon_no))
$(EXTRACT_AC_RESPONSE_ERROR $(dcs_addr) $(ets_addr) $(ediref) $(error))
})

#########################################################################################
### отдача контроля ЭБ нами

$(defmacro PUSH_AC_REQUEST
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  status
  subcl
  airline
  flt_no
  airp_dep
  date_dep #формат: ddmmyy
  time_dep #формат: hhnn
  airp_arv
{
>>
UNB+SIRE:1+$(dcs_addr)+$(ets_addr)+xxxxxx:xxxx+$(ediref)0001+++O"
UNH+1+TKCREQ:00:1:IA+$(ediref)"
MSG+:142"
ORG+1H:MOW+++$(airline)+Y+::RU+SYSTEM"
EQN+1:TD"
TKT+$(ticket_no):T"
CPN+$(coupon_no):$(status)"
TVL+$(date_dep)+$(airp_dep)+$(airp_arv)+$(airline)+$(flt_no)$(if $(eq $(subcl) "") "" ":"$(subcl))++$(coupon_no)"
UNT+8+1"
UNZ+1+$(ediref)0001"

})

$(defmacro PUSH_AC_RESPONSE_OK
  dcs_addr
  ets_addr
  ediref
  ticket_no
  coupon_no
  status
{
<<
UNB+SIRE:1+$(ets_addr)+$(dcs_addr)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(ediref)"
MSG+:142+3"
TKT+$(ticket_no):T:1:3"
CPN+$(coupon_no):$(status)::E:::AL"
UNT+5+1"
UNZ+1+$(ediref)0001"

})

$(defmacro PUSH_AC_RESPONSE_ERROR
  dcs_addr
  ets_addr
  ediref
  error
{
<<
UNB+SIRE:1+$(ets_addr)+$(dcs_addr)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(ediref)"
MSG+:142+3"
ERC+$(error)"
UNT+4+1"
UNZ+1+$(ediref)0001"

})




$(defmacro CHANGE_PAX_STATUS_REQUEST
  pax_id
  capture=off
{
!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETStatus' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <ChangePaxStatus>
      <segments>
        <segment>
          <pax_id>$(pax_id)</pax_id>
        </segment>
      </segments>
    </ChangePaxStatus>
  </query>
</term>

})


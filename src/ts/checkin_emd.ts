include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/sirena_exchange_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/spp/write_trips_macro.ts)
include(ts/pnl/pnl_ut_580_461.ts)
include(ts/pax/checkin_macro.ts)
include(ts/pax/et_emd_macro.ts)
include(ts/pax/boarding_macro.ts)

# meta: suite checkin

$(defmacro TKCRES_ET_DISP_2982410821479
    from
    to
    ediref
{UNB+SIRE:1+$(from)+$(to)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:131+3"
EQN+1:TD"
TIF+������+�����"
TAI+2984+1494:B"
RCI+1H:04VSFC:1+UT:054C82:1"
MON+B:17000:RUB+T:19185:RUB"
FOP+CA:3:19185"
PTK+++$(ddmmyy -3)"
ODI+AER+TJM"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1494+������"
EQN+2:TF"
TXD++2000:::YQ+185:::YR"
IFT+4:15:0+AER UT X/MOW UT TJM17000RUB17000END"
IFT+4:10+����/�1744.09"
IFT+4:39+������+��� ������ ������"
IFT+4:5+74951234567"
TKT+2982410821479:T:1:3"
CPN+1:I::E"
TVL+$(ddmmyy +1):0600+AER+VKO+UT+580:Y++1"
RPI++OK"
PTS++YTR"
EBD++1::N"
CPN+2:I::E"
TVL+$(ddmmyy +1):1130+VKO+TJM+UT+461:Y++2"
RPI++OK"
PTS++YTR"
EBD++1::N"
UNT+29+1"
UNZ+1+$(ediref)0001"}
)

$(defmacro TKCRES_ET_DISP_WITH_EMD_2982410821479
    from
    to
    ediref
{UNB+SIRE:1+$(from)+$(to)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:131+3"
EQN+1:TD"
TIF+������+�����"
TAI+2984+1494:B"
RCI+1H:04VSFC:1+UT:054C82:1"
MON+B:17000:RUB+T:19185:RUB"
FOP+CA:3:19185"
PTK+++$(ddmmyy -3)"
ODI+AER+TJM"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1494+������"
EQN+2:TF"
TXD++2000:::YQ+185:::YR"
IFT+4:15:0+AER UT X/MOW UT TJM17000RUB17000END"
IFT+4:10+����/�1744.09"
IFT+4:39+������+��� ������ ������"
IFT+4:5+74951234567"
TKT+2982410821479:T:1:3"
CPN+1:CK::E"
TVL+$(ddmmyy +1):0600+AER+VKO+UT+580:Y++1"
RPI++OK"
PTS++YTR"
EBD++1::N"
CPN+2:CK::E"
TVL+$(ddmmyy +1):1130+VKO+TJM+UT+461:Y++2"
RPI++OK"
PTS++YTR"
EBD++1::N"
TKT+2982410821479:T:1:4::2988200015229"
CPN+1:::::::1::702"
PTS++YTR++++0AI"
TKT+2982410821479:T:1:4::2988200015229"
CPN+2:::::::2::702"
PTS++YTR++++0AI"
TKT+2982410821479:T:1:4::2988200015231"
CPN+1:::::::1::702"
PTS++YTR++++BF1"
TKT+2982410821479:T:1:4::2988200015231"
CPN+2:::::::2::702"
PTS++YTR++++BF1"
TKT+2982410821479:T:1:4::2988200015233"
CPN+1:::::::1::702"
PTS++YTR++++0BS"
TKT+2982410821479:T:1:4::2988200015233"
CPN+2:::::::2::702"
PTS++YTR++++0BS"
UNT+47+1"
UNZ+1+$(ediref)0001"}
)

$(defmacro TKCRES_ET_DISP_2982410821480
    from
    to
    ediref
{UNB+SIRE:1+$(from)+$(to)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:131+3"
EQN+1:TD"
TIF+������+�����"
TAI+2984+1494:B"
RCI+1H:04VSFC:1+UT:054C82:1"
MON+B:17000:RUB+T:19185:RUB"
FOP+CA:3:19185"
PTK+++$(ddmmyy -3)"
ODI+AER+TJM"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1494+������"
EQN+2:TF"
TXD++2000:::YQ+185:::YR"
IFT+4:15:0+AER UT X/MOW UT TJM17000RUB17000END"
IFT+4:10+����/�1744.09"
IFT+4:39+������+��� ������ ������"
IFT+4:5+74951234567"
TKT+2982410821480:T:1:3"
CPN+1:I::E"
TVL+$(ddmmyy +1):0600+AER+VKO+UT+580:Y++1"
RPI++OK"
PTS++YTR"
EBD++1::N"
CPN+2:I::E"
TVL+$(ddmmyy +1):1130+VKO+TJM+UT+461:Y++2"
RPI++OK"
PTS++YTR"
EBD++1::N"
UNT+29+1"
UNZ+1+$(ediref)0001"}
)

$(defmacro TKCRES_ET_DISP_WITH_EMD_2982410821480
    from
    to
    ediref
{UNB+SIRE:1+$(from)+$(to)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:131+3"
EQN+1:TD"
TIF+������+�����"
TAI+2984+1494:B"
RCI+1H:04VSFC:1+UT:054C82:1"
MON+B:17000:RUB+T:19185:RUB"
FOP+CA:3:19185"
PTK+++$(ddmmyy -3)"
ODI+AER+TJM"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1494+������"
EQN+2:TF"
TXD++2000:::YQ+185:::YR"
IFT+4:15:0+AER UT X/MOW UT TJM17000RUB17000END"
IFT+4:10+����/�1744.09"
IFT+4:39+������+��� ������ ������"
IFT+4:5+74951234567"
TKT+2982410821480:T:1:3"
CPN+1:CK::E"
TVL+$(ddmmyy +1):0600+AER+VKO+UT+580:Y++1"
RPI++OK"
PTS++YTR"
EBD++1::N"
CPN+2:CK::E"
TVL+$(ddmmyy +1):1130+VKO+TJM+UT+461:Y++2"
RPI++OK"
PTS++YTR"
EBD++1::N"
TKT+2982410821480:T:1:4::2988200015230"
CPN+1:::::::1::702"
PTS++YTR++++0AI"
TKT+2982410821480:T:1:4::2988200015230"
CPN+2:::::::2::702"
PTS++YTR++++0AI"
TKT+2982410821480:T:1:4::2988200015232"
CPN+2:::::::1::702"
PTS++YTR++++SPF"
TKT+2982410821480:T:1:4::2988200015234"
CPN+2:::::::2::702"
PTS++YTR++++04V"
TKT+2982410821480:T:1:4::2988200015234"
CPN+1:::::::1::702"
PTS++YTR++++04V"
UNT+44+1"
UNZ+1+$(ediref)0001"}
)

$(defmacro TKCRES_EMD_DISP_2982410821479
    from
    to
    ediref
    emd_no
    emd_status
    rfisc
    rfic
    service_name
{UNB+SIRE:1+$(from)+$(to)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:791+3"
TIF+������+�����"
TAI+2984+99���:B"
RCI+1H:04VSFC:1+UT:054C82:1"
MON+B:2000:RUB+T:2000:RUB"
FOP+CA:3:2000"
PTK+:::::::NR++$(ddmmyy -1)"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1494+������"
EQN+1:TD"
IFT+4:15:0"
IFT+4:39+������+��� ������ ������"
IFT+4:41+01���76"
IFT+4:43+���181.82+����� �� ������ �� �������� ��������"
IFT+4:733:0"
PTS+++++$(rfic)"
TKT+$(emd_no):J:1"
CPN+1:$(emd_status):1000:E"
TVL++AER+VKO+UT"
PTS++++++$(rfisc)"
IFT+4:47+$(service_name)"
CPN+2:$(emd_status):1000:E"
TVL++VKO+TJM+UT"
PTS++++++$(rfisc)"
IFT+4:47+$(service_name)"
TKT+$(emd_no):J::4::2982410821479"
CPN+1:::::::1::702"
PTS++YTR"
CPN+2:::::::2::702"
PTS++YTR"
UNT+31+1"
UNZ+1+$(ediref)0001"}
)

$(defmacro TKCRES_EMD_DISP_2982410821480
    from
    to
    ediref
    emd_no
    emd_status
    rfisc
    rfic
    service_name
{UNB+SIRE:1+$(from)+$(to)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:791+3"
TIF+������+�����"
TAI+2984+99���:B"
RCI+1H:04VSFC:1+UT:054C82:1"
MON+B:2000:RUB+T:2000:RUB"
FOP+CA:3:2000"
PTK+:::::::NR++$(ddmmyy -1)"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1494+������"
EQN+1:TD"
IFT+4:15:0"
IFT+4:39+������+��� ������ ������"
IFT+4:41+01���76"
IFT+4:43+���181.82+����� �� ������ �� �������� ��������"
IFT+4:733:0"
PTS+++++$(rfic)"
TKT+$(emd_no):J:1"
CPN+1:$(emd_status):1000:E"
TVL++AER+VKO+UT"
PTS++++++$(rfisc)"
IFT+4:47+$(service_name)"
CPN+2:$(emd_status):1000:E"
TVL++VKO+TJM+UT"
PTS++++++$(rfisc)"
IFT+4:47+$(service_name)"
TKT+$(emd_no):J::4::2982410821480"
CPN+1:::::::1::702"
PTS++YTR"
CPN+2:::::::2::702"
PTS++YTR"
UNT+31+1"
UNZ+1+$(ediref)0001"}
)

$(defmacro TKCRES_EMD_DISP_2982410821480_1SEG
    from
    to
    ediref
    emd_no
    emd_status
    rfisc
    rfic
    service_name
{UNB+SIRE:1+$(from)+$(to)+$(yymmdd):$(hhmi)+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:791+3"
TIF+������+�����"
TAI+2984+99���:B"
RCI+1H:04VSFC:1+UT:054C82:1"
MON+B:2000:RUB+T:2000:RUB"
FOP+CA:3:2000"
PTK+:::::::NR++$(ddmmyy -1)"
ORG+1H:MOW+29842300:99���+MOW++T+RU+1494+������"
EQN+1:TD"
IFT+4:15:0"
IFT+4:39+������+��� ������ ������"
IFT+4:41+01���76"
IFT+4:43+���181.82+����� �� ������ �� �������� ��������"
IFT+4:733:0"
PTS+++++$(rfic)"
TKT+$(emd_no):J:1"
CPN+1:$(emd_status):1000:E"
TVL++VKO+TJM+UT"
PTS++++++$(rfisc)"
IFT+4:47+$(service_name)"
TKT+$(emd_no):J::4::2982410821480"
CPN+1:::::::2::702"
PTS++YTR"
UNT+25+1"
UNZ+1+$(ediref)0001"}
)

###########################################################################################################################

$(defmacro EMD_REFRESH_2982410821479
  et_disp_edi_ref
{

<<
$(TKCRES_ET_DISP_WITH_EMD_2982410821479 UTET UTDC $(et_disp_edi_ref))

$(set edi_ref_5229 $(last_edifact_ref 2))
$(set edi_ref_5231 $(last_edifact_ref 1))
$(set edi_ref_5233 $(last_edifact_ref 0))

>>
$(TKCREQ_EMD_DISP UTDC UTET $(get edi_ref_5229) �� 2988200015229)
>>
$(TKCREQ_EMD_DISP UTDC UTET $(get edi_ref_5231) �� 2988200015231)
>>
$(TKCREQ_EMD_DISP UTDC UTET $(get edi_ref_5233) �� 2988200015233)

<<
$(TKCRES_EMD_DISP_2982410821479 UTET UTDC $(get edi_ref_5233) 2988200015233 I 0BS C "�������� � �������� �����")
<<
$(TKCRES_EMD_DISP_2982410821479 UTET UTDC $(get edi_ref_5231) 2988200015231 I BF1 G ��������)
<<
$(TKCRES_EMD_DISP_2982410821479 UTET UTDC $(get edi_ref_5229) 2988200015229 I 0AI G �������)

})

###########################################################################################################################

$(defmacro EMD_REFRESH_2982410821480
  et_disp_edi_ref
{

<<
$(TKCRES_ET_DISP_WITH_EMD_2982410821480 UTET UTDC $(et_disp_edi_ref))

$(set edi_ref_5230 $(last_edifact_ref 2))
$(set edi_ref_5232 $(last_edifact_ref 1))
$(set edi_ref_5234 $(last_edifact_ref 0))

>>
$(TKCREQ_EMD_DISP UTDC UTET $(get edi_ref_5230) �� 2988200015230)
>>
$(TKCREQ_EMD_DISP UTDC UTET $(get edi_ref_5232) �� 2988200015232)
>>
$(TKCREQ_EMD_DISP UTDC UTET $(get edi_ref_5234) �� 2988200015234)

<<
$(TKCRES_EMD_DISP_2982410821480 UTET UTDC $(get edi_ref_5234) 2988200015234 I 04V C ������)
<<
$(TKCRES_EMD_DISP_2982410821480_1SEG UTET UTDC $(get edi_ref_5232) 2988200015232 I SPF A "����� �����")
<<
$(TKCRES_EMD_DISP_2982410821480 UTET UTDC $(get edi_ref_5230) 2988200015230 I 0AI G �������)

})

###########################################################################################################################

$(defmacro EMD_CHANGE_STATUS_1GROUP
{
$(set edi_ref8 $(last_edifact_ref 8))
$(set edi_ref7 $(last_edifact_ref 7))
$(set edi_ref6 $(last_edifact_ref 6))
$(set edi_ref5 $(last_edifact_ref 5))
$(set edi_ref4 $(last_edifact_ref 4))
$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref8) �� 2988200015229 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref7) �� 2988200015230 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref6) �� 2988200015232 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref5) �� 2988200015233 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref4) �� 2988200015234 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref3) �� 2988200015229 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref2) �� 2988200015230 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref1) �� 2988200015233 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref0) �� 2988200015234 1 CK)

<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref8) 2988200015229 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref7) 2988200015230 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref6) 2988200015232 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref5) 2988200015233 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref4) 2988200015234 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref3) 2988200015229 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref2) 2988200015230 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref1) 2988200015233 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref0) 2988200015234 1 CK)

})

###########################################################################################################################

$(defmacro EMD_CHANGE_STATUS_2GROUPS
{
$(set edi_ref8 $(last_edifact_ref 8))
$(set edi_ref7 $(last_edifact_ref 7))
$(set edi_ref6 $(last_edifact_ref 6))
$(set edi_ref5 $(last_edifact_ref 5))
$(set edi_ref4 $(last_edifact_ref 4))
$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))


>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref8) �� 2988200015229 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref7) �� 2988200015233 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref6) �� 2988200015229 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref5) �� 2988200015233 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref4) �� 2988200015230 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref3) �� 2988200015232 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref2) �� 2988200015234 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref1) �� 2988200015230 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref0) �� 2988200015234 1 CK)


<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref8) 2988200015229 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref7) 2988200015233 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref6) 2988200015229 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref5) 2988200015233 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref4) 2988200015230 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref3) 2988200015232 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref2) 2988200015234 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref1) 2988200015232 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref0) 2988200015234 1 CK)


})

###########################################################################################################################

$(defmacro ADL_UT_461_WITH_ASVC
{$(ADL_UT_461
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015229C2
.R/ASVC HI1 G/BF1//��������/A/2988200015231C2
.R/ASVC HI1 C/0BS//ZHIVOTNOE V BAGAZH OTDEL DO 23/A/2988200015233C2}
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015230C2
.R/ASVC HI1 A/SPF//����� �����/A/2988200015232C1
.R/ASVC HI1 C/04V//������������� ������ �� 32��/A/2988200015234C2})})

$(defmacro ADL_UT_580_WITH_ASVC
{$(ADL_UT_580
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015229C1
.R/ASVC HI1 G/BF1//��������/A/2988200015231C1
.R/ASVC HI1 C/0BS//ZHIVOTNOE V BAGAZH OTDEL DO 23/A/2988200015233C1}
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015230C1
.R/ASVC HI1 C/04V//������������� ������ �� 32��/A/2988200015234C1})})

###########################################################################################################################

$(defmacro CHANGE_CHECKIN_WEIGHT_CONCEPT_ADD_BAG
{      <value_bags/>
      <bags>
        <bag>
          <bag_type/>
          <airline>��</airline>
          <num>1</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>13</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>1</bag_pool_num>
        </bag>
        <bag>
          <bag_type/>
          <airline>��</airline>
          <num>2</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>12</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>2</bag_pool_num>
        </bag>
      </bags>
      <tags pr_print=\"1\"/>
      <unaccomps/>
      <services/>})

###########################################################################################################################

$(defmacro CHANGE_CHECKIN_ADD_SVC
{      <value_bags/>
      <bags>
        <bag>
          <rfisc>0BS</rfisc>
          <service_type>C</service_type>
          <airline>��</airline>
          <num>1</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>5</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>1</bag_pool_num>
        </bag>
        <bag>
          <rfisc>04V</rfisc>
          <service_type>C</service_type>
          <airline>��</airline>
          <num>2</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>7</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>2</bag_pool_num>
        </bag>
      </bags>
      <tags pr_print=\"1\"/>
      <unaccomps/>
      <services>
        <item>
          <rfisc>SPF</rfisc>
          <service_type>F</service_type>
          <airline>��</airline>
          <service_quantity>1</service_quantity>
          <pax_id>$(get pax_id_1480_1)</pax_id>
          <transfer_num>1</transfer_num>
        </item>
      </services>})

$(defmacro CHANGE_CHECKIN_ADD_SVC_1479
{      <value_bags/>
      <bags>
        <bag>
          <rfisc>0BS</rfisc>
          <service_type>C</service_type>
          <airline>��</airline>
          <num>1</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>5</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>1</bag_pool_num>
        </bag>
      </bags>
      <tags pr_print=\"1\"/>
      <unaccomps/>
      <services/>})

$(defmacro CHANGE_CHECKIN_ADD_SVC_1480
  svc_transfer_num=1
{      <value_bags/>
      <bags>
        <bag>
          <rfisc>04V</rfisc>
          <service_type>C</service_type>
          <airline>��</airline>
          <num>1</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>7</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>1</bag_pool_num>
        </bag>
      </bags>
      <tags pr_print=\"1\"/>
      <unaccomps/>
      <services>
        <item>
          <rfisc>SPF</rfisc>
          <service_type>F</service_type>
          <airline>��</airline>
          <service_quantity>1</service_quantity>
          <pax_id>$(get pax_id_1480_1)</pax_id>
          <transfer_num>$(svc_transfer_num)</transfer_num>
        </item>
      </services>})

###########################################################################################################################

$(defmacro SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE
{    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"F\" rfisc=\"SPF\" rfic=\"A\" emd_type=\"EMD-A\">
      <name language=\"en\">SEAT ASSIGNMENT</name>
      <name language=\"ru\">����� �����</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0BS\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">PET IN HOLD</name>
      <name language=\"ru\">�������� � ����� ����� �� 23��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"1\" company=\"UT\" service_type=\"C\" rfisc=\"0BS\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">PET IN HOLD</name>
      <name language=\"ru\">�������� � ����� ����� �� 23��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>})

$(defmacro SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1479
{    <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0BS\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">PET IN HOLD</name>
      <name language=\"ru\">�������� � ����� ����� �� 23��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"1\" company=\"UT\" service_type=\"C\" rfisc=\"0BS\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">PET IN HOLD</name>
      <name language=\"ru\">�������� � ����� ����� �� 23��</name>
    </svc>})

$(defmacro SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1479_1SEG
{    <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0BS\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">PET IN HOLD</name>
      <name language=\"ru\">�������� � ����� ����� �� 23��</name>
    </svc>})

$(defmacro SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1480
{    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"F\" rfisc=\"SPF\" rfic=\"A\" emd_type=\"EMD-A\">
      <name language=\"en\">SEAT ASSIGNMENT</name>
      <name language=\"ru\">����� �����</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>})

$(defmacro SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1480_1SEG
{    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" company=\"UT\" service_type=\"F\" rfisc=\"SPF\" rfic=\"A\" emd_type=\"EMD-A\">
      <name language=\"en\">SEAT ASSIGNMENT</name>
      <name language=\"ru\">����� �����</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>})

$(defmacro SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_AFTER
{    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"F\" rfisc=\"SPF\" rfic=\"A\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">SEAT ASSIGNMENT</name>
      <name language=\"ru\">����� �����</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"0BS\" rfic=\"C\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">PET IN HOLD</name>
      <name language=\"ru\">�������� � ����� ����� �� 23��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"1\" company=\"UT\" service_type=\"C\" rfisc=\"0BS\" rfic=\"C\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">PET IN HOLD</name>
      <name language=\"ru\">�������� � ����� ����� �� 23��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>})

$(defmacro SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_AFTER_1480
{    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"F\" rfisc=\"SPF\" rfic=\"A\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">SEAT ASSIGNMENT</name>
      <name language=\"ru\">����� �����</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>
    <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" company=\"UT\" service_type=\"C\" rfisc=\"04V\" rfic=\"C\" emd_type=\"EMD-A\" paid=\"true\">
      <name language=\"en\">FIREARMS UP TO 32KG</name>
      <name language=\"ru\">������������� ������ �� 32��</name>
    </svc>})

###########################################################################################################################

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE
{ <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"SPF\" service_type=\"F\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" rfisc=\"0BS\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"1\" rfisc=\"0BS\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" rfisc=\"04V\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"04V\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>})

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1479
{ <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" rfisc=\"0BS\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"1\" rfisc=\"0BS\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>})

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1479_1SEG
{ <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" rfisc=\"0BS\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>})

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1480
{ <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"SPF\" service_type=\"F\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" rfisc=\"04V\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"04V\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>})

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1480_1SEG
{ <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" rfisc=\"SPF\" service_type=\"F\" payment_status=\"need\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" rfisc=\"04V\" service_type=\"C\" payment_status=\"need\" company=\"UT\"/>})

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_AFTER
{ <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"SPF\" service_type=\"F\" payment_status=\"paid\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"0\" rfisc=\"0BS\" service_type=\"C\" payment_status=\"paid\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1479_1)\" segment-id=\"1\" rfisc=\"0BS\" service_type=\"C\" payment_status=\"paid\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" rfisc=\"04V\" service_type=\"C\" payment_status=\"paid\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"04V\" service_type=\"C\" payment_status=\"paid\" company=\"UT\"/>})

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_AFTER_1480
{ <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"SPF\" service_type=\"F\" payment_status=\"paid\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"0\" rfisc=\"04V\" service_type=\"C\" payment_status=\"paid\" company=\"UT\"/>
 <svc passenger-id=\"$(get pax_id_1480_1)\" segment-id=\"1\" rfisc=\"04V\" service_type=\"C\" payment_status=\"paid\" company=\"UT\"/>})

###########################################################################################################################

$(defmacro PAID_RFISCS_BEFORE_EN
{    <paid_rfiscs>
      <item>
        <rfisc>04V</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>firearms up to 32kg</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>04V</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>firearms up to 32kg</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>0BS</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>pet in hold</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>0BS</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>pet in hold</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>SPF</rfisc>
        <service_type>F</service_type>
        <airline>��</airline>
        <name_view>seat assignment</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <total_view>-/1</total_view>
        <paid_view>-/1</paid_view>
      </item>
    </paid_rfiscs>})

$(defmacro PAID_RFISCS_AFTER_EN
{    <paid_rfiscs>
      <item>
        <rfisc>04V</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>firearms up to 32kg</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>04V</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>firearms up to 32kg</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>0AI</rfisc>
        <service_type/>
        <airline/>
        <name_view>����ࠪ</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>2</service_quantity>
        <paid>2</paid>
        <total_view>2/2</total_view>
        <paid_view>2/2</paid_view>
      </item>
      <item>
        <rfisc>0AI</rfisc>
        <service_type/>
        <airline/>
        <name_view>����ࠪ</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>2</service_quantity>
        <paid>2</paid>
        <total_view>2/2</total_view>
        <paid_view>2/2</paid_view>
      </item>
      <item>
        <rfisc>0BS</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>pet in hold</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>0BS</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>pet in hold</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>SPF</rfisc>
        <service_type>F</service_type>
        <airline>��</airline>
        <name_view>seat assignment</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <total_view>-/1</total_view>
        <paid_view>-/1</paid_view>
      </item>
    </paid_rfiscs>})

###########################################################################################################################

$(defmacro PAID_BAG_VIEW_BEFORE_EN
{    <paid_bag_view font_size='8'>
      <cols>
        <col width='140' align='taLeftJustify'/>
        <col width='30' align='taCenter'/>
        <col width='80' align='taLeftJustify'/>
        <col width='40' align='taRightJustify'/>
        <col width='40' align='taRightJustify'/>
      </cols>
      <header font_size='10' font_style='' align='taLeftJustify'>
        <col>RFISC</col>
        <col>Num.</col>
        <col>Segment</col>
        <col>Charge</col>
        <col>Paid</col>
      </header>
      <rows>
        <row>
          <col>04V: firearms up to 32kg</col>
          <col>1</col>
          <col>1: UT580 AER</col>
          <col font_style='fsBold' font_color='clInactiveAlarm' font_color_selected='clInactiveAlarm'>1</col>
          <col font_style='fsBold'>0</col>
        </row>
        <row>
          <col>04V: firearms up to 32kg</col>
          <col>1</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveAlarm' font_color_selected='clInactiveAlarm'>1</col>
          <col font_style='fsBold'>0</col>
        </row>
        <row>
          <col>0BS: pet in hold</col>
          <col>1</col>
          <col>1: UT580 AER</col>
          <col font_style='fsBold' font_color='clInactiveAlarm' font_color_selected='clInactiveAlarm'>1</col>
          <col font_style='fsBold'>0</col>
        </row>
        <row>
          <col>0BS: pet in hold</col>
          <col>1</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveAlarm' font_color_selected='clInactiveAlarm'>1</col>
          <col font_style='fsBold'>0</col>
        </row>
        <row>
          <col>SPF: seat assignment</col>
          <col>1</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveAlarm' font_color_selected='clInactiveAlarm'>1</col>
          <col font_style='fsBold'>0</col>
        </row>
      </rows>
    </paid_bag_view>})

$(defmacro PAID_BAG_VIEW_AFTER_EN
{    <paid_bag_view font_size='8'>
      <cols>
        <col width='140' align='taLeftJustify'/>
        <col width='30' align='taCenter'/>
        <col width='80' align='taLeftJustify'/>
        <col width='40' align='taRightJustify'/>
        <col width='40' align='taRightJustify'/>
      </cols>
      <header font_size='10' font_style='' align='taLeftJustify'>
        <col>RFISC</col>
        <col>Num.</col>
        <col>Segment</col>
        <col>Charge</col>
        <col>Paid</col>
      </header>
      <rows>
        <row>
          <col>04V: firearms up to 32kg</col>
          <col>1</col>
          <col>1: UT580 AER</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>1</col>
          <col font_style='fsBold'>1</col>
        </row>
        <row>
          <col>04V: firearms up to 32kg</col>
          <col>1</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>1</col>
          <col font_style='fsBold'>1</col>
        </row>
        <row>
          <col>0AI: ����ࠪ</col>
          <col>2</col>
          <col>1: UT580 AER</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>2</col>
          <col font_style='fsBold'>2</col>
        </row>
        <row>
          <col>0AI: ����ࠪ</col>
          <col>2</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>2</col>
          <col font_style='fsBold'>2</col>
        </row>
        <row>
          <col>0BS: pet in hold</col>
          <col>1</col>
          <col>1: UT580 AER</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>1</col>
          <col font_style='fsBold'>1</col>
        </row>
        <row>
          <col>0BS: pet in hold</col>
          <col>1</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>1</col>
          <col font_style='fsBold'>1</col>
        </row>
        <row>
          <col>SPF: seat assignment</col>
          <col>1</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>1</col>
          <col font_style='fsBold'>1</col>
        </row>
      </rows>
    </paid_bag_view>})

###########################################################################################################################

$(defmacro PAID_BAG_VIEW_WEIGHT_CONCEPT_BEFORE_EN
{    <paid_bags>
      <paid_bag>
        <bag_type/>
        <airline>��</airline>
        <name_view>Regular or hand bagg.</name_view>
        <weight>25</weight>
        <rate_id/>
        <rate/>
        <rate_cur/>
        <rate_trfer/>
        <priority>2</priority>
        <weight_calc>25</weight_calc>
        <total_view>2/25</total_view>
        <paid_view>25</paid_view>
      </paid_bag>
    </paid_bags>
    <paid_bag_view font_size='8'>
      <cols>
        <col width='120' align='taLeftJustify'/>
        <col width='30' align='taCenter'/>
        <col width='75' align='taLeftJustify'/>
        <col width='25' align='taCenter'/>
        <col width='40' align='taRightJustify'/>
        <col width='40' align='taRightJustify'/>
      </cols>
      <header font_size='10' font_style='' align='taLeftJustify'>
        <col>Baggage type</col>
        <col>Num.</col>
        <col>Norm</col>
        <col>Trfr</col>
        <col>Charge</col>
        <col>Paid</col>
      </header>
      <rows>
        <row>
          <col>regular or hand bagg.</col>
          <col>2/25</col>
          <col>-</col>
          <col/>
          <col font_style='fsBold' font_color='clInactiveAlarm' font_color_selected='clInactiveAlarm'>25</col>
          <col font_style='fsBold'>0</col>
        </row>
      </rows>
    </paid_bag_view>})

$(defmacro PAID_BAG_VIEW_WEIGHT_CONCEPT_AFTER_EN
{    <paid_bags>
      <paid_bag>
        <bag_type/>
        <airline>��</airline>
        <name_view>Regular or hand bagg.</name_view>
        <weight>25</weight>
        <rate_id/>
        <rate/>
        <rate_cur/>
        <rate_trfer/>
        <priority>2</priority>
        <weight_calc>25</weight_calc>
        <total_view>2/25</total_view>
        <paid_view>25</paid_view>
      </paid_bag>
    </paid_bags>
    <paid_bag_view font_size='8'>
      <cols>
        <col width='140' align='taLeftJustify'/>
        <col width='30' align='taCenter'/>
        <col width='80' align='taLeftJustify'/>
        <col width='40' align='taRightJustify'/>
        <col width='40' align='taRightJustify'/>
      </cols>
      <header font_size='10' font_style='' align='taLeftJustify'>
        <col>RFISC/Baggage type</col>
        <col>Num.</col>
        <col>Norm/Segment</col>
        <col>Charge</col>
        <col>Paid</col>
      </header>
      <rows>
        <row>
          <col>0AI: ����ࠪ</col>
          <col>2</col>
          <col>1: UT580 AER</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>2</col>
          <col font_style='fsBold'>2</col>
        </row>
        <row>
          <col>0AI: ����ࠪ</col>
          <col>2</col>
          <col>2: UT461 VKO</col>
          <col font_style='fsBold' font_color='clInactiveBright' font_color_selected='clInactiveBright'>2</col>
          <col font_style='fsBold'>2</col>
        </row>
        <row>
          <col>regular or hand bagg.</col>
          <col>2/25</col>
          <col>-</col>
          <col font_style='fsBold' font_color='clInactiveAlarm' font_color_selected='clInactiveAlarm'>25</col>
          <col font_style='fsBold'>0</col>
        </row>
      </rows>
    </paid_bag_view>})

###########################################################################################################################

$(defmacro RUN_REPORT_REQUEST
  point_id
  rpt_type
  text
  lang=RU
  capture=off
{
!! capture=on
<term>
  <query handle='0' id='docs' ver='1' opr='PIKE' screen='DOCS.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <run_report2>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <point_id>$(point_id)</point_id>
      <rpt_type>$(rpt_type)</rpt_type>
      <text>$(text)</text>
      <LoadForm/>
    </run_report2>
  </query>
</term>

})

###########################################################################################################################

$(defmacro CHECK_SERVICES_REPORT_BEFORE
{

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1A</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>C</RFIC>
            <RFISC>0BS</RFISC>
            <desc>PET IN HOLD</desc>
            <num/>
            <str>  1A    KOTOVA IRINA       1  C    0BS  PET IN HOLD                             </str>
          </row>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>FIREARMS UP TO 32KG</desc>
            <num/>
            <str>  1B    MOTOVA IRINA       2  C    04V  FIREARMS UP TO 32KG                     </str>
          </row>
        </table>
      </datasets>



$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1�</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>C</RFIC>
            <RFISC>0BS</RFISC>
            <desc>�������� � ����� ����� �� 23��</desc>
            <num/>
            <str>  1�    KOTOVA IRINA       1  C    0BS  �������� � �����                        $()
                                        ����� �� 23��                           </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>������������� ������ �� 32��</desc>
            <num/>
            <str>  1�    MOTOVA IRINA       2  C    04V  �������������                           $()
                                        ������ �� 32��                          </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>A</RFIC>
            <RFISC>SPF</RFISC>
            <desc>����� �����</desc>
            <num/>
            <str>  1�    MOTOVA IRINA       2  A    SPF  ����� �����                             </str>
          </row>
        </table>
      </datasets>

})

###########################################################################################################################

$(defmacro CHECK_SERVICES_REPORT_AFTER
{

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1A</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>BREAKFAST</desc>
            <num>2988200015229/1</num>
            <str>  1A    KOTOVA IRINA       1  G    0AI  BREAKFAST           2988200015229/1     </str>
          </row>
          <row>
            <seat_no>  1A</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>C</RFIC>
            <RFISC>0BS</RFISC>
            <desc>PET IN HOLD</desc>
            <num>2988200015233/1</num>
            <str>  1A    KOTOVA IRINA       1  C    0BS  PET IN HOLD         2988200015233/1     </str>
          </row>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>FIREARMS UP TO 32KG</desc>
            <num>2988200015234/1</num>
            <str>  1B    MOTOVA IRINA       2  C    04V  FIREARMS UP TO 32KG 2988200015234/1     </str>
          </row>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>BREAKFAST</desc>
            <num>2988200015230/1</num>
            <str>  1B    MOTOVA IRINA       2  G    0AI  BREAKFAST           2988200015230/1     </str>
          </row>
        </table>
      </datasets>

$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1�</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015229/2</num>
            <str>  1�    KOTOVA IRINA       1  G    0AI  �������             2988200015229/2     </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>C</RFIC>
            <RFISC>0BS</RFISC>
            <desc>�������� � ����� ����� �� 23��</desc>
            <num>2988200015233/2</num>
            <str>  1�    KOTOVA IRINA       1  C    0BS  �������� � �����    2988200015233/2     $()
                                        ����� �� 23��                           </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>������������� ������ �� 32��</desc>
            <num>2988200015234/2</num>
            <str>  1�    MOTOVA IRINA       2  C    04V  �������������       2988200015234/2     $()
                                        ������ �� 32��                          </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015230/2</num>
            <str>  1�    MOTOVA IRINA       2  G    0AI  �������             2988200015230/2     </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>A</RFIC>
            <RFISC>SPF</RFISC>
            <desc>����� �����</desc>
            <num>2988200015232/1</num>
            <str>  1�    MOTOVA IRINA       2  A    SPF  ����� �����         2988200015232/1     </str>
          </row>
        </table>
      </datasets>

})

###########################################################################################################################

$(defmacro PREPARE_2PAXES_2SEGS
  gds_exchange=1
{

$(set tomor $(date_format %d.%m.%Y +1))

$(init_eds �� UTET UTDC)
$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)
$(allow_gds_exchange �� allow=$(gds_exchange))

!! capture=on
$(cache PIKE RU RFISC_SETS $(cache_iface_ver RFISC_SETS) ""
  insert airline:$(get_elem_id etAirline ��)
         rfic:G
         rfisc:0AI
         auto_checkin:1)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

!! capture=on
$(cache PIKE RU RFISC_SETS $(cache_iface_ver RFISC_SETS) ""
  insert airline:$(get_elem_id etAirline ��)
         rfic:A
         rfisc:SPF
         auto_checkin:0)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

!! capture=on
$(cache PIKE EN RFISC_SETS $(cache_iface_ver RFISC_SETS) ""
  insert airline:$(get_elem_id etAirline ��)
         rfic:C
         rfisc:0BS
         auto_checkin:1)

$(USER_ERROR_RESPONSE MSG.FORBIDDEN_INSERT_RFIC)


$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point �� 580 TU3 65021 ""                   ��� "$(get tomor) 12:00")
  $(new_spp_point_last             "$(get tomor) 15:00" ��� ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point �� 461 TU3 65021 ""                   ��� "$(get tomor) 16:00")
  $(new_spp_point_last             "$(get tomor) 21:20" ��� ) })

$(PNL_UT_461)

$(set edi_ref_1479_1 $(last_edifact_ref 1))
$(set edi_ref_1480_1 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479_1) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480_1) �� 2982410821480)
<<
$(TKCRES_ET_DISP_2982410821479 UTET UTDC $(get edi_ref_1479_1))


$(PNL_UT_580)

$(set edi_ref_1480_2 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480_2) �� 2982410821480)
<<
$(TKCRES_ET_DISP_2982410821480 UTET UTDC $(get edi_ref_1480_1))
<<
$(TKCRES_ET_DISP_2982410821480 UTET UTDC $(get edi_ref_1480_2))


$(set point_dep1 $(get_point_dep_for_flight �� 580 "" $(yymmdd +1) ���))
$(set point_arv1 $(get_next_trip_point_id $(get point_dep1)))
$(set point_dep2 $(get_point_dep_for_flight �� 461 "" $(yymmdd +1) ���))
$(set point_arv2 $(get_next_trip_point_id $(get point_dep2)))

$(set pax_id_1479_1 $(get_pax_id $(get point_dep1) KOTOVA IRINA))
$(set pax_id_1480_1 $(get_pax_id $(get point_dep1) MOTOVA IRINA))
$(set pax_id_1479_2 $(get_pax_id $(get point_dep2) KOTOVA IRINA))
$(set pax_id_1480_2 $(get_pax_id $(get point_dep2) MOTOVA IRINA))

$(set svc_seg1 {company=\"UT\" flight=\"580\" operating_company=\"UT\" operating_flight=\"580\" departure=\"AER\" arrival=\"VKO\" departure_time=\"$(date_format %Y-%m-%d +1)T12:00:00\" arrival_time=\"$(date_format %Y-%m-%d +1)T15:00:00\" equipment=\"TU3\"})
$(set svc_seg2 {company=\"UT\" flight=\"461\" operating_company=\"UT\" operating_flight=\"461\" departure=\"VKO\" arrival=\"TJM\" departure_time=\"$(date_format %Y-%m-%d +1)T16:00:00\" arrival_time=\"$(date_format %Y-%m-%d +1)T21:20:00\" equipment=\"TU3\"})

$(prepare_bt_for_flight $(get point_dep1) ����)

})

###########################################################################################################################

$(defmacro CHECKIN_2PAXES_2SEGS_1GROUP
{

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_2) 2)
  </pax>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_2) 2)
  </pax>
</passengers>})})

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref3) �� 2982410821479 2 CK xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref2) �� 2982410821480 2 CK xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) �� 2982410821479 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821480 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref3) 2982410821479 2 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref2) 2982410821480 2 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982410821479 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821480 1 CK)

$(http_forecast content=$(SVC_AVAILABILITY_RESPONSE_UT_2PAXES_2SEGS $(get pax_id_1479_1) $(get pax_id_1480_1)))

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>


>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg1) $(get svc_seg2))
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 2 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
    <display id=\"2\">...</display>
  </svc_availability>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*

$(set grp_id_1479_1 $(get_single_grp_id $(get pax_id_1479_1)))
$(set grp_id_1480_1 $(get_single_grp_id $(get pax_id_1480_1)))
$(set grp_id_1479_2 $(get_single_grp_id $(get pax_id_1479_2)))
$(set grp_id_1480_2 $(get_single_grp_id $(get pax_id_1480_2)))

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_1 $(get_single_pax_tid $(get pax_id_1480_1)))
$(set pax_tid_1479_2 $(get_single_pax_tid $(get pax_id_1479_2)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))

})

###########################################################################################################################

$(defmacro CHECKIN_2PAXES_2SEGS_2GROUPS
{

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_2) 2)
  </pax>
</passengers>})})

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) �� 2982410821479 2 CK xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821479 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982410821479 2 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821479 1 CK)

$(http_forecast content=$(SVC_AVAILABILITY_RESPONSE_UT_1PAX_2SEGS $(get pax_id_1479_1)))

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>


>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
  </svc_availability>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>1</reg_no>.*

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_2) 2)
  </pax>
</passengers>})})

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) �� 2982410821480 2 CK xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821480 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982410821480 2 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821480 1 CK)

$(http_forecast content=$(SVC_AVAILABILITY_RESPONSE_UT_1PAX_2SEGS $(get pax_id_1480_1)))

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>


>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 1 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
  </svc_availability>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
            <reg_no>2</reg_no>.*
            <reg_no>2</reg_no>.*

$(set grp_id_1479_1 $(get_single_grp_id $(get pax_id_1479_1)))
$(set grp_id_1480_1 $(get_single_grp_id $(get pax_id_1480_1)))
$(set grp_id_1479_2 $(get_single_grp_id $(get pax_id_1479_2)))
$(set grp_id_1480_2 $(get_single_grp_id $(get pax_id_1480_2)))

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_1 $(get_single_pax_tid $(get pax_id_1480_1)))
$(set pax_tid_1479_2 $(get_single_pax_tid $(get pax_id_1479_2)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))

})

###########################################################################################################################

$(defmacro CHECKIN_2PAXES_2SEGS_1GROUP_ADD_SVC
{

$(CHECKIN_2PAXES_2SEGS_1GROUP)

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      </datasets>

$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      </datasets>

$(http_forecast
  content=$(SVC_PAYMENT_STATUS_RESPONSE_UT_2PAXES_2SEGS $(get pax_id_1479_1) $(get pax_id_1480_1) $(SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num=2)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1479_2) $(get_single_grp_tid $(get pax_id_1479_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_2) $(get pax_tid_1479_2) 2 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 2 bag_pool_num=2)
  </pax>
</passengers>})}
$(CHANGE_CHECKIN_ADD_SVC)
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg1) $(get svc_seg2))
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 2 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
    <display id=\"2\">...</display>
$(SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE)
  </svc_payment_status>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*
$(PAID_RFISCS_BEFORE_EN)
.*
$(PAID_BAG_VIEW_BEFORE_EN)
.*


$(set edi_ref_1479 $(last_edifact_ref 1))
$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)
<<
$(TKCRES_ET_DISP_2982410821479 UTET UTDC $(get edi_ref_1479))
<<
$(TKCRES_ET_DISP_2982410821480 UTET UTDC $(get edi_ref_1480))

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_RFISCS_BEFORE_EN)
.*
$(PAID_BAG_VIEW_BEFORE_EN)
.*

})

###########################################################################################################################

$(defmacro CHANGE_TCHECKIN_ADD_SVC_1479
{

$(http_forecast
  content=$(SVC_PAYMENT_STATUS_RESPONSE_UT_1PAX_2SEGS $(get pax_id_1479_1) $(SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1479)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num=1)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1479_2) $(get_single_grp_tid $(get pax_id_1479_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_2) $(get pax_tid_1479_2) 2 bag_pool_num=1)
  </pax>
</passengers>})}
$(CHANGE_CHECKIN_ADD_SVC_1479)
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
$(SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1479)
  </svc_payment_status>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*

$(set edi_ref_1479 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
<<
$(TKCRES_ET_DISP_2982410821479 UTET UTDC $(get edi_ref_1479))

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
    <paid_rfiscs>
      <item>
        <rfisc>0BS</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>pet in hold</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>0BS</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>pet in hold</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
    </paid_rfiscs>
.*

})

###########################################################################################################################

$(defmacro CHANGE_TCHECKIN_ADD_SVC_1480
{

$(http_forecast
  content=$(SVC_PAYMENT_STATUS_RESPONSE_UT_1PAX_2SEGS $(get pax_id_1480_1) $(SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1480)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1480_1) $(get_single_grp_tid $(get pax_id_1480_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num=1)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1480_2) $(get_single_grp_tid $(get pax_id_1480_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 2 bag_pool_num=1)
  </pax>
</passengers>})}
$(CHANGE_CHECKIN_ADD_SVC_1480)
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 1 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
$(SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1480)
  </svc_payment_status>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*

$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)
<<
$(TKCRES_ET_DISP_2982410821480 UTET UTDC $(get edi_ref_1480))

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
    <paid_rfiscs>
      <item>
        <rfisc>04V</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>firearms up to 32kg</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>04V</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>firearms up to 32kg</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1/1</total_view>
        <paid_view>1/1</paid_view>
      </item>
      <item>
        <rfisc>SPF</rfisc>
        <service_type>F</service_type>
        <airline>��</airline>
        <name_view>seat assignment</name_view>
        <transfer_num>1</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <total_view>-/1</total_view>
        <paid_view>-/1</paid_view>
      </item>
    </paid_rfiscs>
.*

})

###########################################################################################################################

$(defmacro CHECKIN_2PAXES_2SEGS_2GROUPS_ADD_SVC
{

$(CHECKIN_2PAXES_2SEGS_2GROUPS)

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      </datasets>

$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      </datasets>

### ������塞 ��㣨 ��ࢮ�� ���ᠦ���

$(CHANGE_TCHECKIN_ADD_SVC_1479)

### ������塞 ��㣨 ��஬� ���ᠦ��

$(CHANGE_TCHECKIN_ADD_SVC_1480)

})


### test 1 - ���ॣ������ EMD �� ��ࢮ��砫쭮� ॣ����樨
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)

$(ADL_UT_461_WITH_ASVC)
$(ADL_UT_580_WITH_ASVC)

$(CHECKIN_2PAXES_2SEGS_1GROUP)

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1A</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>BREAKFAST</desc>
            <num>2988200015229/1</num>
            <str>  1A    KOTOVA IRINA       1  G    0AI  BREAKFAST           2988200015229/1     </str>
          </row>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>BREAKFAST</desc>
            <num>2988200015230/1</num>
            <str>  1B    MOTOVA IRINA       2  G    0AI  BREAKFAST           2988200015230/1     </str>
          </row>
        </table>
      </datasets>

$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1�</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015229/2</num>
            <str>  1�    KOTOVA IRINA       1  G    0AI  �������             2988200015229/2     </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015230/2</num>
            <str>  1�    MOTOVA IRINA       2  G    0AI  �������             2988200015230/2     </str>
          </row>
        </table>
      </datasets>

%%

### test 2 - ���ॣ������ � ��⮯ਢ離� EMD �� ����㧪� ��㯯� �� ॣ. ������
### ���� ⠪��:
### 1. ��室�� PNL ��� ASVC
### 2. ��������㥬 2 ���� �� 2-� ᥣ����� ������ ��㯯��. ����������, �� EMD ���
### 3. ������ ����� ����� � ���� �� ॣ����樨
### 4. ��ଫ����� EMD, ��室�� ADL c ASVC
### 5. ����㦠�� ��㯯� �� ॣ����樮����� ������ - ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD
### 6. ��室�� �� ADL, �� ⠬ ��� ��������� ASVC. �஡㥬 �� ࠧ �맢��� ��㯯� - ������� �������, �� �ਢ易��
### 7. ����塞 ����稪� �� ADL ��ࢮ�� ᥣ����. �஡㥬 �� ࠧ �맢��� ��㯯� - ������ ������� ��ᯫ�� �� ���ᠦ��� � ��������묨 ASVC
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)

$(CHECKIN_2PAXES_2SEGS_1GROUP_ADD_SVC)

$(CHECK_SERVICES_REPORT_BEFORE)

#########################################################################################
### ��ଫ����� EMD, ��室�� ADL

$(ADL_UT_580_WITH_ASVC)
$(ADL_UT_461_WITH_ASVC)

### ����㦠�� ��㯯� �� ॣ����樮����� ������ - ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD

$(LOAD_PAX_BY_REG_NO capture=on lang=EN $(get point_dep1) reg_no=2)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*
$(PAID_RFISCS_BEFORE_EN)
.*
$(PAID_BAG_VIEW_BEFORE_EN)
.*

### ����訢��� ��ᯫ�� ET � EMD

$(set edi_ref_1479 $(last_edifact_ref 1))
$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)

$(EMD_REFRESH_2982410821479 $(get edi_ref_1479))
$(EMD_REFRESH_2982410821480 $(get edi_ref_1480))

$(KICK_IN)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.EDS_CONNECT_ERROR message=.*)
.*
$(PAID_RFISCS_BEFORE_EN)
.*
$(PAID_BAG_VIEW_BEFORE_EN)
.*

### ���塞 ����� �ਢ易���� EMD

$(EMD_CHANGE_STATUS_1GROUP)

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_RFISCS_AFTER_EN)
.*
$(PAID_BAG_VIEW_AFTER_EN)
.*

#########################################################################################
### ��㧨� ADL, �� ⠬ ��� ��������� ASVC

$(ADL_UT_580
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015229C1
.R/ASVC HI1 G/BF1//��������/A/2988200015231C1
.R/ASVC HI1 C/0BS//ZHIVOTNOE V BAGAZH OTDEL DO 23/A/2988200015233C1
.R/XAXA}
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015230C1
.R/ASVC HI1 C/04V//������������� ������ �� 32��/A/2988200015234C1
.R/XAXA})

### �஡㥬 �� ࠧ �맢��� ��㯯� - ������� �������, �� �ਢ易��

$(LOAD_PAX_BY_REG_NO capture=on lang=EN $(get point_dep1) reg_no=2)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_RFISCS_AFTER_EN)
.*
$(PAID_BAG_VIEW_AFTER_EN)
.*

#########################################################################################
### 㤠�塞 ����稪� �� ADL ��ࢮ�� ᥣ���� (�� ��ண�)
### ��������� �� ��஬ ᥣ���� �� �������� �� ��⮯ਢ離� � ���ॣ������ �� ��ࢮ�, ���� �᫨ ᪢����� ॣ������. �� ������⮪ :(

$(ADL_UT_580
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015229C1
.R/ASVC HI1 C/0BS//ZHIVOTNOE V BAGAZH OTDEL DO 23/A/2988200015233C1
.R/XAXA}
{.R/ASVC HI1 G/0AI//ZAVTRAK/A/2988200015230C1
.R/ASVC HI1 C/04V//������������� ������ �� 32��/A/2988200015234C1
.R/XAXA})


### �஡㥬 �� ࠧ �맢��� ��㯯� - ������ ������� ��ᯫ�� �� ���ᠦ��� � ��������묨 ASVC

$(LOAD_PAX_BY_REG_NO capture=on lang=EN $(get point_dep1) reg_no=2)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*
$(PAID_RFISCS_AFTER_EN)
.*
$(PAID_BAG_VIEW_AFTER_EN)
.*

$(set edi_ref_1479 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)

$(EMD_REFRESH_2982410821479 $(get edi_ref_1479))

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_RFISCS_AFTER_EN)
.*
$(PAID_BAG_VIEW_AFTER_EN)
.*

$(CHECK_SERVICES_REPORT_AFTER)

%%

### test 3 - ��⮯ਢ離� EMD �� ����㧪� ��㯯� �� ॣ. ������ ��� ��ᮢ�� ��⥬� ���� ������
### ���� ⠪��:
### 1. ��室�� PNL ��� ASVC
### 2. ��������㥬 2 ���� �� 2-� ᥣ����� ������ ��㯯��. ����������, �� EMD ���
### 3. ������ ����� �����
### 4. ��ଫ����� EMD
### 5. ����㦠�� ��㯯� �� ॣ����樮����� ������ - ������ ��⮬���᪬ ��ॣ����஢����� EMD
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS gds_exchange=0)

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_1) 1 Y)
  </pax>
</passengers>})
$(NEW_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_2) 2)
  </pax>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_2) 2)
  </pax>
</passengers>})})

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref3) �� 2982410821479 2 CK xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref2) �� 2982410821480 2 CK xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) �� 2982410821479 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821480 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref3) 2982410821479 2 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref2) 2982410821480 2 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982410821479 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821480 1 CK)

$(KICK_IN)

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*

$(set grp_id_1479_1 $(get_single_grp_id $(get pax_id_1479_1)))
$(set grp_id_1480_1 $(get_single_grp_id $(get pax_id_1480_1)))
$(set grp_id_1479_2 $(get_single_grp_id $(get pax_id_1479_2)))
$(set grp_id_1480_2 $(get_single_grp_id $(get pax_id_1480_2)))

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_1 $(get_single_pax_tid $(get pax_id_1480_1)))
$(set pax_tid_1479_2 $(get_single_pax_tid $(get pax_id_1479_2)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num=2)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1479_2) $(get_single_grp_tid $(get pax_id_1479_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_2) $(get pax_tid_1479_2) 2 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 2 bag_pool_num=2)
  </pax>
</passengers>})}
$(CHANGE_CHECKIN_WEIGHT_CONCEPT_ADD_BAG)
)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_BAG_VIEW_WEIGHT_CONCEPT_BEFORE_EN)
.*

### ����㦠�� ��㯯� �� ॣ����樮����� ������ - ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD

$(LOAD_PAX_BY_REG_NO capture=on lang=EN $(get point_dep1) reg_no=2)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*
$(PAID_BAG_VIEW_WEIGHT_CONCEPT_BEFORE_EN)
.*

### ����訢��� ��ᯫ�� ET � EMD

$(set edi_ref_1479 $(last_edifact_ref 1))
$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)

$(EMD_REFRESH_2982410821479 $(get edi_ref_1479))
$(EMD_REFRESH_2982410821480 $(get edi_ref_1480))

$(KICK_IN)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.EDS_CONNECT_ERROR message=.*)
.*
$(PAID_BAG_VIEW_WEIGHT_CONCEPT_BEFORE_EN)
.*

### ���塞 ����� �ਢ易���� EMD

$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref3) �� 2988200015229 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref2) �� 2988200015230 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref1) �� 2988200015229 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref0) �� 2988200015230 1 CK)

<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref3) 2988200015229 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref2) 2988200015230 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref1) 2988200015229 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref0) 2988200015230 1 CK)


$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_BAG_VIEW_WEIGHT_CONCEPT_AFTER_EN)
.*

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1A</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015229/1</num>
            <str>  1A    KOTOVA IRINA       1  G    0AI  �������             2988200015229/1     </str>
          </row>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015230/1</num>
            <str>  1B    MOTOVA IRINA       2  G    0AI  �������             2988200015230/1     </str>
          </row>
        </table>
      </datasets>

$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1�</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015229/2</num>
            <str>  1�    KOTOVA IRINA       1  G    0AI  �������             2988200015229/2     </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015230/2</num>
            <str>  1�    MOTOVA IRINA       2  G    0AI  �������             2988200015230/2     </str>
          </row>
        </table>
      </datasets>


%%

### test 4 - ���ॣ������ � ��⮯ਢ離� EMD �� ����� ���������
### ���� ⠪��:
### 1. ��室�� PNL ��� ASVC
### 2. ��������㥬 2 ���� �� 2-� ᥣ����� ������ ��㯯��
### 3. ��ଫ����� EMD, ��室�� ADL c ASVC
### 4. ������ ����� ����� � ���� �� ॣ����樨 - ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD
### 5. ������ �����-����� ��㣨� ��������� �� ॣ����樨 - ������� �������, �� �ਢ易��
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)

$(CHECKIN_2PAXES_2SEGS_1GROUP)

#########################################################################################
### ��ଫ����� EMD, ��室�� ADL

$(ADL_UT_580_WITH_ASVC)
$(ADL_UT_461_WITH_ASVC)

$(http_forecast
  content=$(SVC_PAYMENT_STATUS_RESPONSE_UT_2PAXES_2SEGS $(get pax_id_1479_1) $(get pax_id_1480_1) $(SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num=2)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1479_2) $(get_single_grp_tid $(get pax_id_1479_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_2) $(get pax_tid_1479_2) 2 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 2 bag_pool_num=2)
  </pax>
</passengers>})}
$(CHANGE_CHECKIN_ADD_SVC)
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg1) $(get svc_seg2))
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 2 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
    <display id=\"2\">...</display>
$(SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE)
  </svc_payment_status>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*
$(PAID_RFISCS_BEFORE_EN)
.*
$(PAID_BAG_VIEW_BEFORE_EN)
.*

### ����訢��� ��ᯫ�� ET � EMD

$(set edi_ref_1479 $(last_edifact_ref 1))
$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)

$(EMD_REFRESH_2982410821479 $(get edi_ref_1479))
$(EMD_REFRESH_2982410821480 $(get edi_ref_1480))

$(KICK_IN)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.EDS_CONNECT_ERROR message=.*)
.*
$(PAID_RFISCS_BEFORE_EN)
.*
$(PAID_BAG_VIEW_BEFORE_EN)
.*

### ���塞 ����� �ਢ易���� EMD

$(EMD_CHANGE_STATUS_1GROUP)

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_RFISCS_AFTER_EN)
.*
$(PAID_BAG_VIEW_AFTER_EN)
.*


#########################################################################################
### ������ �����-����� ��㣨� ��������� �� ॣ����樨 - ������� �������, �� �ਢ易��
### �� �� ࠢ�� ����� �� �業���, ��⮬� �� ���஢���� �㤥� ࠡ���� ⮫쪮 � ᫥���騩 ࠧ

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_1 $(get_single_pax_tid $(get pax_id_1480_1)))
$(set pax_tid_1479_2 $(get_single_pax_tid $(get pax_id_1479_2)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))

$(http_forecast
  content=$(SVC_PAYMENT_STATUS_RESPONSE_UT_2PAXES_2SEGS $(get pax_id_1479_1) $(get pax_id_1480_1) $(SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_AFTER)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num=2)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1479_2) $(get_single_grp_tid $(get pax_id_1479_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_2) $(get pax_tid_1479_2) 2 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 2 bag_pool_num=2)
  </pax>
</passengers>})}
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg1) $(get svc_seg2))
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 2 $(get svc_seg1) $(get svc_seg2))
    <display id=\"1\">...</display>
    <display id=\"2\">...</display>
$(SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_AFTER)
  </svc_payment_status>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_RFISCS_AFTER_EN)
.*
$(PAID_BAG_VIEW_AFTER_EN)
.*

#########################################################################################
### ������ �����-����� ��㣨� ��������� �� ॣ����樨 - ������� �������, �� �ਢ易��
### 㦥 �� ����� �� �業��� - ࠡ�⠥� ���஢����

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_1 $(get_single_pax_tid $(get pax_id_1480_1)))
$(set pax_tid_1479_2 $(get_single_pax_tid $(get pax_id_1479_2)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num=2)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1479_2) $(get_single_grp_tid $(get pax_id_1479_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_2) $(get pax_tid_1479_2) 2 bag_pool_num=1)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 2 bag_pool_num=2)
  </pax>
</passengers>})}
)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(PAID_RFISCS_AFTER_EN)
.*
$(PAID_BAG_VIEW_AFTER_EN)
.*

$(CHECK_SERVICES_REPORT_AFTER)

$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))
$(set pax_tid_1480_1 $(get_single_pax_tid $(get pax_id_1480_1)))
$(set pax_tid_1479_2 $(get_single_pax_tid $(get pax_id_1479_2)))
$(set pax_tid_1480_2 $(get_single_pax_tid $(get pax_id_1480_2)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1479_1) $(get_single_grp_tid $(get pax_id_1479_1))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num="" refuse=�)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num="" refuse=�)
  </pax>
</passengers>})
$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                         $(get grp_id_1479_2) $(get_single_grp_tid $(get pax_id_1479_2))
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_2) $(get pax_tid_1479_2) 2 bag_pool_num="" refuse=�)
  </pax>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_2) $(get pax_tid_1480_2) 2 bag_pool_num="" refuse=�)
  </pax>
</passengers>})}
)

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref3) �� 2982410821479 2 I xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref2) �� 2982410821480 2 I xxxxxx ��� ��� 461 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) �� 2982410821479 1 I xxxxxx ��� ��� 580 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821480 1 I xxxxxx ��� ��� 580 depd=$(ddmmyy +1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref3) 2982410821479 2 I)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref2) 2982410821480 2 I)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982410821479 1 I)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821480 1 I)

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <segments/>
  </answer>
</term>


%%

### test 5 - ��⮯ਢ離� � ���ॣ������ EMD �� ��ᠤ��
### ���� ⠪��:
### 1. ��室�� PNL ��� ASVC
### 2. ��������㥬 2 ���� �� 2-� ᥣ����� ������ ��㯯��
### 3. ������ ����� ����� � ���� �� ॣ����樨
### 4. ��ଫ����� EMD, ��室�� ADL c ASVC
### 5. ������ ������ ���ᠦ�� �� ��ࢮ� ᥣ���� - ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD ��� �ᥩ ��㯯�
### 6. ������ ��ண� ���ᠦ�� �� ��ࢮ� ᥣ���� - ������� �������, �� �ਢ易��
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)
$(PREPARE_HALLS_FOR_BOARDING ���)

$(CHECKIN_2PAXES_2SEGS_1GROUP_ADD_SVC)

$(CHECK_SERVICES_REPORT_BEFORE)

#########################################################################################
### ��ଫ����� EMD, ��室�� ADL

$(ADL_UT_580_WITH_ASVC)
$(ADL_UT_461_WITH_ASVC)

### ᠦ��� ������ ���ᠦ�� �� ��ࢮ� ᥣ���� - ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep1) $(get pax_id_1480_1) 777 "" 1)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <form_data>
      <variables/>
    </form_data>
    <data/>
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR)
  </answer>
</term>


$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)

$(EMD_REFRESH_2982410821480 $(get edi_ref_1480))

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
$(USER_ERROR_MESSAGE_TAG MSG.EDS_CONNECT_ERROR)
  </answer>
</term>

### ���塞 ����� �ਢ易���� EMD

$(EMD_CHANGE_STATUS_1GROUP)

$(KICK_IN)

>> mode=regex
.*
      <updated>
        <pax_id>$(get pax_id_1480_1)</pax_id>
      </updated>
.*
$(MESSAGE_TAG MSG.PASSENGER.BOARDING2 message=.*)
.*

### ᠦ��� ��ண� ���ᠦ�� �� ��ࢮ� ᥣ���� - ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD �ᥩ ��㯯�

$(BOARDING_REQUEST_BY_PAX_ID capture=on $(get point_dep1) $(get pax_id_1479_1) 777 "" 1)

>> mode=regex
.*
      <updated>
        <pax_id>$(get pax_id_1479_1)</pax_id>
      </updated>
.*
$(MESSAGE_TAG MSG.PASSENGER.BOARDING2 message=.*)
.*

$(CHECK_SERVICES_REPORT_AFTER)

%%

### test 6 - ��⮯ਢ離� � ���ॣ������ EMD �� �����⨨ ॣ����樨 ��� ����� ��㯯�
### ���� ⠪��:
### 1. ��室�� PNL ��� ASVC
### 2. ��������㥬 2 ���� �� 2-� ᥣ����� ������ ��㯯��
### 3. ������ ����� ����� � ���� �� ॣ����樨
### 4. ��ଫ����� EMD, ��室�� ADL c ASVC
### 5. ����뢠���� ॣ������ �� ��஬ ᥣ���� - � 䮭���� ०��� ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD �� ����� ᥣ�����
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)

$(CHECKIN_2PAXES_2SEGS_1GROUP_ADD_SVC)

#########################################################################################
### ��ଫ����� EMD, ��室�� ADL

$(ADL_UT_580_WITH_ASVC)
$(ADL_UT_461_WITH_ASVC)

$(CLOSE_CHECKIN $(get point_dep2))

$(kick_flt_tasks_daemon)


$(set edi_ref_1479 $(last_edifact_ref 1))
$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)

$(EMD_REFRESH_2982410821479 $(get edi_ref_1479))
$(EMD_REFRESH_2982410821480 $(get edi_ref_1480))

$(kick_flt_tasks_daemon)

### ���塞 ����� �ਢ易���� EMD

$(EMD_CHANGE_STATUS_1GROUP)

$(CHECK_SERVICES_REPORT_BEFORE)

$(kick_flt_tasks_daemon)

$(CHECK_SERVICES_REPORT_AFTER)

%%

### test 7 - ��⮯ਢ離� � ���ॣ������ EMD �� �����⨨ ॣ����樨 ��� ࠧ��� ��㯯
### ���� ⠪��:
### 1. ��室�� PNL ��� ASVC
### 2. ��������㥬 2 ���� �� 2-� ᥣ����� 2-�� ࠧ�묨 ��㯯���
### 3. ������ ����� ����� � ���� �� ॣ����樨
### 4. ��ଫ����� EMD, �� �� ��室�� ADL c ASVC
### 5. ����뢠���� ॣ������ �� ��ࢮ� ᥣ���� - � 䮭���� ०��� ������ ��⮬���᪬ ��ॣ����஢����� � �ਢ易���� EMD �� ����� ᥣ�����
### 6. ������� ��室�� ADL c ASVC, �� ⮫쪮 ��� ��ண� ᥣ���� � ��� ������ �� ���ᠦ�஢
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)

$(CHECKIN_2PAXES_2SEGS_2GROUPS_ADD_SVC)

$(CLOSE_CHECKIN $(get point_dep1))

$(kick_flt_tasks_daemon)


$(set edi_ref_1479 $(last_edifact_ref 1))
$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)

$(EMD_REFRESH_2982410821479 $(get edi_ref_1479))
$(EMD_REFRESH_2982410821480 $(get edi_ref_1480))

$(kick_flt_tasks_daemon)

$(EMD_CHANGE_STATUS_2GROUPS)

$(CHECK_SERVICES_REPORT_BEFORE)

$(kick_flt_tasks_daemon)

$(CHECK_SERVICES_REPORT_AFTER)

%%

### test 8 - ��⮯ਢ離� � ���ॣ������ EMD �� �����⨨ ॣ����樨 ��� ࠧ��� ��㯯 (2-� ��ਠ��)
### ���� ⠪��:
### 1. ��室�� PNL ��� ASVC
### 2. ��������㥬 2 ���� �� 2-� ᥣ����� 2-�� ࠧ�묨 ��㯯���
### 3. ������ ����� ����� � ���� �� ॣ����樨 ⮫쪮 ������ ���ᠦ���
### 4. ��ଫ����� EMD, ��室�� ADL c ASVC
### 5. ����뢠���� ॣ������ �� ��஬ ᥣ���� -
###    � 䮭���� ०��� ������ ��⮬���᪬ ��ॣ����஢����� EMD ��� ����� ���ᠦ�஢ � �ਢ易���� EMD ��� ������ �� ���ᠦ�஢
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)

$(CHECKIN_2PAXES_2SEGS_2GROUPS)

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      </datasets>

$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      </datasets>

### ������ ����� ����� � ���� �� ॣ����樨 ⮫쪮 ������ ���ᠦ���

$(CHANGE_TCHECKIN_ADD_SVC_1480)

### ��ଫ����� EMD, ��室�� ADL

$(ADL_UT_580_WITH_ASVC)
$(ADL_UT_461_WITH_ASVC)

$(CLOSE_CHECKIN $(get point_dep2))

$(kick_flt_tasks_daemon)

$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)

$(EMD_REFRESH_2982410821480 $(get edi_ref_1480))

$(kick_flt_tasks_daemon)

$(set edi_ref6 $(last_edifact_ref 6))
$(set edi_ref5 $(last_edifact_ref 5))
$(set edi_ref4 $(last_edifact_ref 4))
$(set edi_ref3 $(last_edifact_ref 3))
$(set edi_ref2 $(last_edifact_ref 2))
$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref6) �� 2988200015229 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref5) �� 2988200015229 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref4) �� 2988200015230 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref3) �� 2988200015232 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref2) �� 2988200015234 2 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref1) �� 2988200015230 1 CK)
>>
$(TKCREQ_EMD_COS UTDC UTET $(get edi_ref0) �� 2988200015234 1 CK)

### �⢥�� �ࠧ�����

<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref0) 2988200015234 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref2) 2988200015234 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref3) 2988200015232 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref1) 2988200015230 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref4) 2988200015230 2 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref5) 2988200015229 1 CK)
<<
$(TKCRES_EMD_COS UTET UTDC $(get edi_ref6) 2988200015229 2 CK)


$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>FIREARMS UP TO 32KG</desc>
            <num/>
            <str>  1B    MOTOVA IRINA       2  C    04V  FIREARMS UP TO 32KG                     </str>
          </row>
        </table>
      </datasets>


$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>������������� ������ �� 32��</desc>
            <num/>
            <str>  1�    MOTOVA IRINA       2  C    04V  �������������                           $()
                                        ������ �� 32��                          </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>A</RFIC>
            <RFISC>SPF</RFISC>
            <desc>����� �����</desc>
            <num/>
            <str>  1�    MOTOVA IRINA       2  A    SPF  ����� �����                             </str>
          </row>
        </table>
      </datasets>

$(kick_flt_tasks_daemon)

$(RUN_REPORT_REQUEST capture=on $(get point_dep1) SERVICES 1 EN)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1A</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>BREAKFAST</desc>
            <num>2988200015229/1</num>
            <str>  1A    KOTOVA IRINA       1  G    0AI  BREAKFAST           2988200015229/1     </str>
          </row>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>FIREARMS UP TO 32KG</desc>
            <num>2988200015234/1</num>
            <str>  1B    MOTOVA IRINA       2  C    04V  FIREARMS UP TO 32KG 2988200015234/1     </str>
          </row>
          <row>
            <seat_no>  1B</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>BREAKFAST</desc>
            <num>2988200015230/1</num>
            <str>  1B    MOTOVA IRINA       2  G    0AI  BREAKFAST           2988200015230/1     </str>
          </row>
        </table>
      </datasets>


$(RUN_REPORT_REQUEST capture=on $(get point_dep2) SERVICES 1 RU)

>> lines=auto
      <datasets>
        <table>
          <row>
            <seat_no>  1�</seat_no>
            <family>KOTOVA IRINA</family>
            <reg_no>1</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015229/2</num>
            <str>  1�    KOTOVA IRINA       1  G    0AI  �������             2988200015229/2     </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>C</RFIC>
            <RFISC>04V</RFISC>
            <desc>������������� ������ �� 32��</desc>
            <num>2988200015234/2</num>
            <str>  1�    MOTOVA IRINA       2  C    04V  �������������       2988200015234/2     $()
                                        ������ �� 32��                          </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>G</RFIC>
            <RFISC>0AI</RFISC>
            <desc>�������</desc>
            <num>2988200015230/2</num>
            <str>  1�    MOTOVA IRINA       2  G    0AI  �������             2988200015230/2     </str>
          </row>
          <row>
            <seat_no>  1�</seat_no>
            <family>MOTOVA IRINA</family>
            <reg_no>2</reg_no>
            <RFIC>A</RFIC>
            <RFISC>SPF</RFISC>
            <desc>����� �����</desc>
            <num>2988200015232/1</num>
            <str>  1�    MOTOVA IRINA       2  A    SPF  ����� �����         2988200015232/1     </str>
          </row>
        </table>
      </datasets>

%%

### test 9 - ����� �� ��७� ���ᠦ�஢ � ������祭�묨 ��㣠�� �� ����ঠ���� ३�
### ���� ⠪��:
### 1. ������� ��� ���������� ३� � ࠧ��楩 � ��⪨. ����� ࠭���� ���⠢�塞 ����প� �� ᫥���騥 ��⪨
### 2. ��������㥬 �� ������ ���ᠦ��� �� ������ ३� � ��ଫ塞 ��� ������� ����� �����
### 3. ����訢��� �� ��७� ३� �� ����� ࠭��� ����. � �⢥� ���ᠦ�� ⮫쪮 �⮣� ३�
### 4. ����訢��� �� ��७� ३� �� ����� ����. � �⢥� ���ᠦ��� ����� ३ᮢ, � ⮬ �᫥ � ��७�ᥭ����
### 5. ���⠢�塞 䠪� �뫥� ३ᠬ �� ��।�. � �⢥�� ��७� �� �������� ���ᠦ��� �뫥⥢�� ३ᮢ
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

### ����� ��� �室��� http-����ᮢ �� ��७�

$(CREATE_USER SIRENATEST SIRENATEST)
$(CREATE_DESK SIREN2 1)
$(ADD_HTTP_CLIENT PIECE_CONCEPT SIRENATEST SIRENATEST SIREN2)

### ����� � ���

$(init_eds �� UTET UTDC)

### ����ன�� ��� ��室��� http-����ᮢ � ��७�

$(settcl SIRENA_HOST localhost)
$(settcl SIRENA_PORT 8008)
$(allow_gds_exchange �� allow=1)

$(set today $(date_format %d.%m.%Y +0))
$(set tomor $(date_format %d.%m.%Y +1))

### ��� ���������� ३� � ࠧ��楩 � ��⪨

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point �� 580 TU3 65021 ""                   ��� "$(get today) 12:00")
  $(new_spp_point_last             "$(get today) 15:00" ��� ) })

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point �� 580 TU3 65021 ""                   ��� "$(get tomor) 12:00")
  $(new_spp_point_last             "$(get tomor) 15:00" ��� ) })

$(set point_dep1 $(get_point_dep_for_flight �� 580 "" $(yymmdd +0) ���))
$(set point_arv1 $(get_next_trip_point_id $(get point_dep1)))
$(set point_dep2 $(get_point_dep_for_flight �� 580 "" $(yymmdd +1) ���))
$(set point_arv2 $(get_next_trip_point_id $(get point_dep2)))

$(prepare_bt_for_flight $(get point_dep1) ����)
$(prepare_bt_for_flight $(get point_dep2) ����)

### ����প� ������ ३�, �⮡� ��� ���������� ३� �믮��﫨�� � ���� ��⪨

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep1)
{ $(change_spp_point $(get point_dep1) �� 580 TU3 65021 ""                   ""                   "" ��� "$(get today) 12:00" "$(get tomor) 01:00")
  $(change_spp_point_last $(get point_arv1)             "$(get today) 15:00" "$(get tomor) 04:00" "" ��� )
})

$(PNL_UT_580 time_create=$(dd -1)$(hhmi) date_dep=$(ddmon +0 en))

$(set edi_ref_1479_1 $(last_edifact_ref 1))
$(set edi_ref_1480_1 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479_1) �� 2982410821479)
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480_1) �� 2982410821480)
<<
$(TKCRES_ET_DISP_2982410821479 UTET UTDC $(get edi_ref_1479_1))
<<
$(TKCRES_ET_DISP_2982410821480 UTET UTDC $(get edi_ref_1480_1))

$(PNL_UT_580 time_create=$(dd -0)$(hhmi) date_dep=$(ddmon +1 en))

################################################################################
### ॣ�����㥬 ������ ���ᠦ�� �� ��ࢮ� ३�

$(set pax_id_1480_1 $(get_pax_id $(get point_dep1) MOTOVA IRINA))

$(NEW_CHECKIN_REQUEST $(get point_dep1) $(get point_arv1) ��� ��� hall=1 capture=on
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_1) 1)
  </pax>
</passengers>})

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821480 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1)) ### � COS 㦥 ��७�ᥭ��� ���
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821480 1 CK)

$(http_forecast content=$(SVC_AVAILABILITY_RESPONSE_UT_1PAX_1SEG $(get pax_id_1480_1)))

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

$(set svc_seg1
{company=\"UT\" flight=\"580\" operating_company=\"UT\" operating_flight=\"580\" departure=\"AER\" arrival=\"VKO\" \
scd_departure_time=\"$(date_format %Y-%m-%d +0)T12:00:00\" departure_time=\"$(date_format %Y-%m-%d +1)T01:00:00\" \
arrival_time=\"$(date_format %Y-%m-%d +1)T04:00:00\" equipment=\"TU3\"})

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 1 $(get svc_seg1))
    <display id=\"1\">...</display>
  </svc_availability>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
            <reg_no>1</reg_no>.*

$(set grp_id_1480_1 $(get_single_grp_id $(get pax_id_1480_1)))
$(set grp_tid_1480_1 $(get_single_grp_tid $(get pax_id_1480_1)))
$(set pax_tid_1480_1 $(get_single_pax_tid $(get pax_id_1480_1)))

$(set paid_rfiscs_before_en_1480
{    <paid_rfiscs>
      <item>
        <rfisc>04V</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>firearms up to 32kg</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1</total_view>
        <paid_view>1</paid_view>
      </item>
      <item>
        <rfisc>SPF</rfisc>
        <service_type>F</service_type>
        <airline>��</airline>
        <name_view>seat assignment</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <total_view>1</total_view>
        <paid_view>1</paid_view>
      </item>
    </paid_rfiscs>})


$(http_forecast
  content=$(SVC_PAYMENT_STATUS_RESPONSE_UT_1PAX_1SEG $(get pax_id_1480_1) $(SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1480_1SEG)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
                          $(get grp_id_1480_1) $(get grp_tid_1480_1)
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821480 $(get pax_id_1480_1) $(get pax_tid_1480_1) 1 bag_pool_num=1)
  </pax>
</passengers>})}
$(CHANGE_CHECKIN_ADD_SVC_1480 svc_transfer_num=0)
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 1 $(get svc_seg1))
    <display id=\"1\">...</display>
$(SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1480_1SEG)
  </svc_payment_status>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*
$(get paid_rfiscs_before_en_1480)
.*

$(set edi_ref_1480 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1480) �� 2982410821480)
<<
$(TKCRES_ET_DISP_2982410821480 UTET UTDC $(get edi_ref_1480))

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(get paid_rfiscs_before_en_1480)
.*

################################################################################
### ॣ�����㥬 ������ ���ᠦ�� �� ��஬ ३�

$(set pax_id_1479_1 $(get_pax_id $(get point_dep2) KOTOVA IRINA))

$(NEW_CHECKIN_REQUEST $(get point_dep2) $(get point_arv2) ��� ��� hall=1 capture=on
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1)
  </pax>
</passengers>})

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821479 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821479 1 CK)

$(http_forecast content=$(SVC_AVAILABILITY_RESPONSE_UT_1PAX_1SEG $(get pax_id_1479_1)))

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

$(set svc_seg2 {company=\"UT\" flight=\"580\" operating_company=\"UT\" operating_flight=\"580\" departure=\"AER\" arrival=\"VKO\" departure_time=\"$(date_format %Y-%m-%d +1)T12:00:00\" arrival_time=\"$(date_format %Y-%m-%d +1)T15:00:00\" equipment=\"TU3\"})

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg2))
    <display id=\"1\">...</display>
  </svc_availability>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
            <reg_no>1</reg_no>.*

$(set grp_id_1479_1 $(get_single_grp_id $(get pax_id_1479_1)))
$(set grp_tid_1479_1 $(get_single_grp_tid $(get pax_id_1479_1)))
$(set pax_tid_1479_1 $(get_single_pax_tid $(get pax_id_1479_1)))

$(set paid_rfiscs_before_en_1479
{    <paid_rfiscs>
      <item>
        <rfisc>0BS</rfisc>
        <service_type>C</service_type>
        <airline>��</airline>
        <name_view>pet in hold</name_view>
        <transfer_num>0</transfer_num>
        <service_quantity>1</service_quantity>
        <paid>1</paid>
        <priority>0</priority>
        <total_view>1</total_view>
        <paid_view>1</paid_view>
      </item>
    </paid_rfiscs>})

$(http_forecast
  content=$(SVC_PAYMENT_STATUS_RESPONSE_UT_1PAX_1SEG $(get pax_id_1479_1) $(SVC_PAYMENT_STATUS_RESPONSE_SVC_LIST_BEFORE_1479_1SEG)))

$(CHANGE_TCHECKIN_REQUEST capture=on lang=EN hall=1
{$(CHANGE_CHECKIN_SEGMENT $(get point_dep2) $(get point_arv2) ��� ���
                          $(get grp_id_1479_1) $(get grp_tid_1479_1)
{<passengers>
  <pax>
$(CHANGE_CHECKIN_2982410821479 $(get pax_id_1479_1) $(get pax_tid_1479_1) 1 bag_pool_num=1)
  </pax>
</passengers>})}
$(CHANGE_CHECKIN_ADD_SVC_1479)
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_payment_status show_free_carry_on_norm=\"true\" set_pupil=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg2))
    <display id=\"1\">...</display>
$(SVC_PAYMENT_STATUS_REQUEST_SVC_LIST_BEFORE_1479_1SEG)
  </svc_payment_status>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
$(USER_ERROR_MESSAGE_TAG MSG.ETS_EDS_CONNECT_ERROR message=.*)
.*
$(get paid_rfiscs_before_en_1479)
.*

$(set edi_ref_1479 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref_1479) �� 2982410821479)
<<
$(TKCRES_ET_DISP_2982410821479 UTET UTDC $(get edi_ref_1479))

$(KICK_IN)

>> mode=regex
.*
  <answer.*
    <segments>
.*
$(get paid_rfiscs_before_en_1479)
.*

### ��७� ���뫠�� ����� �� ᯨ᮪ ���ᠦ�஢ � ������祭�묨 ��㣠��

!! capture=on req_type=http
POST / HTTP/1.1
Host: /
Accept-Encoding: gzip,deflate
CLIENT-ID: SIRENATEST
OPERATION: piece_concept
Content-Type: text/xml;charset=UTF-8
Content-Length: 243
$()
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <passenger_with_svc>
    <company>UT</company>
    <flight>580</flight>
    <departure_date>$(date_format %Y-%m-%d +0)</departure_date>
    <departure>AER</departure>
  </passenger_with_svc>
</query>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <passenger_with_svc>
    <passenger>
      <surname>������</surname>
      <name>�����</name>
      <category>ADT</category>
      <group_id>$(get grp_id_1480_1)</group_id>
      <reg_no>1</reg_no>
      <recloc crs=\"DT\">04VSFC</recloc>
      <recloc crs=\"UT\">054C82</recloc>
    </passenger>
  </passenger_with_svc>
</answer>

!! capture=on req_type=http
POST / HTTP/1.1
Host: /
Accept-Encoding: gzip,deflate
CLIENT-ID: SIRENATEST
OPERATION: piece_concept
Content-Type: text/xml;charset=UTF-8
Content-Length: 243
$()
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <passenger_with_svc>
    <company>UT</company>
    <flight>580</flight>
    <departure_date>$(date_format %Y-%m-%d +1)</departure_date>
    <departure>AER</departure>
  </passenger_with_svc>
</query>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <passenger_with_svc>
    <passenger>
      <surname>������</surname>
      <name>�����</name>
      <category>ADT</category>
      <group_id>$(get grp_id_1480_1)</group_id>
      <reg_no>1</reg_no>
      <recloc crs=\"DT\">04VSFC</recloc>
      <recloc crs=\"UT\">054C82</recloc>
    </passenger>
    <passenger>
      <surname>������</surname>
      <name>�����</name>
      <category>ADT</category>
      <group_id>$(get grp_id_1479_1)</group_id>
      <reg_no>1</reg_no>
      <recloc crs=\"DT\">04VSFC</recloc>
      <recloc crs=\"UT\">054C82</recloc>
    </passenger>
  </passenger_with_svc>
</answer>

### ���⠢�塞 �뫥� ��ࢮ�� ३��

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep1)
{ $(change_spp_point $(get point_dep1) �� 580 TU3 65021 ""                   ""                   "" ��� "$(get today) 12:00" "$(get tomor) 01:00" "$(get tomor) 01:05")
  $(change_spp_point_last $(get point_arv1)             "$(get today) 15:00" "$(get tomor) 04:00" "" ��� )
})

!! capture=on req_type=http
POST / HTTP/1.1
Host: /
Accept-Encoding: gzip,deflate
CLIENT-ID: SIRENATEST
OPERATION: piece_concept
Content-Type: text/xml;charset=UTF-8
Content-Length: 243
$()
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <passenger_with_svc>
    <company>UT</company>
    <flight>580</flight>
    <departure_date>$(date_format %Y-%m-%d +1)</departure_date>
    <departure>AER</departure>
  </passenger_with_svc>
</query>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <passenger_with_svc>
    <passenger>
      <surname>������</surname>
      <name>�����</name>
      <category>ADT</category>
      <group_id>$(get grp_id_1479_1)</group_id>
      <reg_no>1</reg_no>
      <recloc crs=\"DT\">04VSFC</recloc>
      <recloc crs=\"UT\">054C82</recloc>
    </passenger>
  </passenger_with_svc>
</answer>

### ���⠢�塞 �뫥� ��஬� ३��

$(CHANGE_SPP_FLIGHT_REQUEST $(get point_dep2)
{ $(change_spp_point $(get point_dep2) �� 580 TU3 65021 ""                   "" "" ��� "$(get tomor) 12:00" "" "$(get tomor) 12:00")
  $(change_spp_point_last $(get point_arv2)             "$(get tomor) 15:00" "" "" ��� )
})

!! capture=on req_type=http
POST / HTTP/1.1
Host: /
Accept-Encoding: gzip,deflate
CLIENT-ID: SIRENATEST
OPERATION: piece_concept
Content-Type: text/xml;charset=UTF-8
Content-Length: 243
$()
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <passenger_with_svc>
    <company>UT</company>
    <flight>580</flight>
    <departure_date>$(date_format %Y-%m-%d +1)</departure_date>
    <departure>AER</departure>
  </passenger_with_svc>
</query>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <passenger_with_svc/>
</answer>

%%

### test 10 - ����� � ��७� �� ��ଫ���� �࠭��୮�� ������, ����� �����業��� ᪢����� ॣ����樨
###########################################################################################################################

$(init_term)
$(set_user_time_type LocalAirp PIKE)

$(PREPARE_2PAXES_2SEGS)

$(NEW_TCHECKIN_REQUEST capture=on lang=EN hall=1
$(TRANSFER_SEGMENT �� 461 "" $(dd +1) ��� ���)
{$(NEW_CHECKIN_SEGMENT $(get point_dep1) $(get point_arv1) ��� ���
{<passengers>
  <pax>
$(NEW_CHECKIN_2982410821479 $(get pax_id_1479_1) 1 Y)
  </pax>
  <pax>
$(NEW_CHECKIN_2982410821480 $(get pax_id_1480_1) 1 Y)
  </pax>
</passengers>})})

$(ERROR_RESPONSE MSG.ETS_CONNECT_ERROR)

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref1) �� 2982410821479 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))
>>
$(TKCREQ_ET_COS UTDC UTET $(get edi_ref0) �� 2982410821480 1 CK xxxxxx ��� ��� 580 depd=$(ddmmyy +1))

<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref1) 2982410821479 1 CK)
<<
$(TKCRES_ET_COS UTET UTDC $(get edi_ref0) 2982410821480 1 CK)

$(http_forecast content=$(SVC_AVAILABILITY_RESPONSE_UT_2PAXES_2SEGS $(get pax_id_1479_1) $(get pax_id_1480_1)))

$(KICK_IN)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
</term>

>> lines=auto
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<query>
  <svc_availability show_brand_info=\"true\" show_all_svc=\"true\" show_free_carry_on_norm=\"true\">
$(SVC_REQUEST_2982410821479 $(get pax_id_1479_1) 1 $(get svc_seg1) ""
  {operating_company=\"UT\" operating_flight=\"461\" departure=\"VKO\" arrival=\"TJM\" departure_date=\"$(date_format %Y-%m-%d +1)\"})
$(SVC_REQUEST_2982410821480 $(get pax_id_1480_1) 2 $(get svc_seg1) ""
  {operating_company=\"UT\" operating_flight=\"461\" departure=\"VKO\" arrival=\"TJM\" departure_date=\"$(date_format %Y-%m-%d +1)\"})
    <display id=\"1\">...</display>
    <display id=\"2\">...</display>
  </svc_availability>
</query>

$(KICK_IN_AFTER_HTTP)

>> mode=regex
.*
            <reg_no>1</reg_no>.*
            <reg_no>2</reg_no>.*








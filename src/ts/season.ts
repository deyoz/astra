# meta: suite season

include(ts/macro.ts)
include(ts/adm_macro.ts)

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps ž’ –‡ APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD UT UFA AER 422 -1 TU5 $(date_format %d.%m.%Y -1000) $(date_format %d.%m.%Y +100))

<<
MOWKB1H
.MOWRMUT 300730
SSM
LT
21JAN00052E001/000000-UT422/21JAN
NEW
UT422
25APR18 03MAY21 123456
J 735 GV.Y126
UFA1515 AER1500
UFAAER 99/1
UFAAER 505/ET

<<
MOWKK1H
.TJMRMUT 210507
SSM
LT
21JAN00052E001/000000-UT422/21JAN
CNL
UT422
19FEB16 25FEB22 1234567

<<
$(dump_table ROUTES fields="AIRLINE, FLT_NO, AIRP, SCD_IN, SCD_OUT" display=on)

$(defmacro PNL_UT_580
  time_create=$(dd)$(hhmi)
  date_dep=$(ddmon +1 en)
  outb_day_dep=$(dd +1)
{
<<
MOWKB1H
.TJMRMUT $(time_create)
PNL
UT580/$(date_dep) AER PART1
CFG/010C100Y
RBD C/CJIDA Y/YSTEQGNBXWUORVHLKPZF
AVAIL
 AER  VKO
C010
Y083
-VKO016Y
1KOTOVA/IRINA-E8
.L/054C82/UT
.L/04VSFC/DT
.O/UT461Y$(outb_day_dep)VKOTJM1130HK
.R/TKNE HK1 2982410821479/1
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//KOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/KOTOVA/IRINA/F
.R/FOID PP7774441110
1MOTOVA/IRINA-E8
.R/TKNE HK1 2982410821480/1
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//MOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/MOTOVA/IRINA/F
.R/FOID PP7774441110
1��������/������-E8
.R/INFT HK1 ��������/������
1��������/������-E8
.R/INFT HK1 ��������/������
1��������/������-E8
.R/INFT HK1 ��������/������
1�������/������-E8
1�������/������-E8
1�������/������-E8
ENDPNL

})

$(defmacro ADL_UT_580
  pax1_rems
  pax2_rems
  time_create=$(dd)$(hhmi)
  date_dep=$(ddmon +1 en)
  outb_day_dep=$(dd +1)
{
<<
MOWKB1H
.TJMRMUT $(time_create)
ADL
UT580/$(date_dep) AER PART1
CFG/010C100Y
RBD C/CJIDA Y/YSTEQGNBXWUORVHLKPZF
AVAIL
 AER  VKO
C010
Y083
-VKO016Y
CHG
1KOTOVA/IRINA-E2
.L/054C82/UT
.L/04VSFC/DT
.O/UT461Y$(outb_day_dep)VKOTJM1130HK
.R/TKNE HK1 2982410821479/1
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//KOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/KOTOVA/IRINA/F
.R/FOID PP7774441110\
$(if $(eq $(pax1_rems) "") "" {
$(pax1_rems)})
1MOTOVA/IRINA-E2
.R/TKNE HK1 2982410821480/1
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//MOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/MOTOVA/IRINA/F
.R/FOID PP7774441110\
$(if $(eq $(pax2_rems) "") "" {
$(pax2_rems)})
ENDADL

})

$(defmacro PNL_UT_461
  time_create=$(dd)$(hhmi)
  date_dep=$(ddmon +1 en)
  inb_day_dep=$(dd +1)
{
<<
MOWKB1H
.TJMRMUT $(time_create)
PNL
UT461/$(date_dep) VKO PART1
CFG/008C116Y
RBD C/CJIDA Y/YSTEQGNBXWUORVHLKPZF
AVAIL
 VKO  TJM
C008
Y109
-VKO016Y
1KOTOVA/IRINA-A8
.L/054C82/UT
.L/04VSFC/DT
.I/UT580Y$(inb_day_dep)AERVKOHK
.R/TKNE HK1 2982410821479/2
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//KOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/KOTOVA/IRINA/F
.R/FOID PP7774441110
.R/PCTC /74951234567-1KOTOVA/IRINA
.R/CTCE TEST//TEST.RU-1KOTOVA/IRINA
1MOTOVA/IRINA-A8
.R/TKNE HK1 2982410821480/2
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//MOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/MOTOVA/IRINA/F
.R/FOID PP7774441110
.R/PCTC /74951234567-1MOTOVA/IRINA
.R/CTCE TEST//TEST.RU-1MOTOVA/IRINA
1��������/������-A8
.R/INFT HK1 ��������/������
1��������/������-A8
.R/INFT HK1 ��������/������
1��������/������-A8
.R/INFT HK1 ��������/������
1�������/������-A8
1�������/������-A8
1�������/������-A8
ENDPNL

})

$(defmacro ADL_UT_461
  pax1_rems
  pax2_rems
  time_create=$(dd)$(hhmi)
  date_dep=$(ddmon +1 en)
  inb_day_dep=$(dd +1)
{
<<
MOWKB1H
.TJMRMUT $(time_create)
ADL
UT461/$(date_dep) VKO PART1
CFG/008C116Y
RBD C/CJIDA Y/YSTEQGNBXWUORVHLKPZF
AVAIL
 VKO  TJM
C008
Y109
-VKO016Y
CHG
1KOTOVA/IRINA-A2
.L/054C82/UT
.L/04VSFC/DT
.I/UT580Y$(inb_day_dep)AERVKOHK
.R/TKNE HK1 2982410821479/2
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//KOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/KOTOVA/IRINA/F
.R/FOID PP7774441110
.R/PCTC /74951234567-1KOTOVA/IRINA
.R/CTCE TEST//TEST.RU-1KOTOVA/IRINA\
$(if $(eq $(pax1_rems) "") "" {
$(pax1_rems)})
1MOTOVA/IRINA-A2
.R/TKNE HK1 2982410821480/2
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/F//MOTOVA/IRINA
.R/PSPT HK1 7774441110/RU/01MAY76/MOTOVA/IRINA/F
.R/FOID PP7774441110
.R/PCTC /74951234567-1MOTOVA/IRINA
.R/CTCE TEST//TEST.RU-1MOTOVA/IRINA\
$(if $(eq $(pax2_rems) "") "" {
$(pax2_rems)})
ENDADL

})


$(defmacro PNL_UT_804_1
  time_create=$(dd)$(hhmi)
  date_dep=$(ddmon +1 en)
{
<<
MOWKB1H
.TJMRMUT $(time_create)
PNL
UT804/$(date_dep) VKO PART1
CFG/008C060Y
RBD C/CJIDA Y/YSTEQGNBXWUORVHLKPZF
AVAIL
 VKO  LED
C008
Y045
-LED000C
-LED000J
-LED000I
-LED000D
-LED000A
-LED006Y
1BUK/IVAN-A5
.L/05952D/UT
.L/04G0X2/DT
.R/TKNE HK1 2982410821862/1
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/M//BUK/IVAN
.R/PSPT HK1 7774441110/RU/01MAY76/BUK/IVAN/M
.R/FOID PP7774441110
.R/PCTC /74951234567-1BUK/IVAN
.R/CTCE TEST//TEST.RU-1BUK/IVAN
1KHUK/IVAN-A5
.R/TKNE HK1 2982410821863/1
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/M//KHUK/IVAN
.R/PSPT HK1 7774441110/RU/01MAY76/KHUK/IVAN/M
.R/FOID PP7774441110
.R/PCTC /74951234567-1KHUK/IVAN
.R/CTCE TEST//TEST.RU-1KHUK/IVAN
1KUK/IVAN-A5
.R/TKNE HK1 2982410821860/1
.R/CKIN HK1 KARAUL
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/M//KUK/IVAN
.R/PSPT HK1 7774441110/RU/01MAY76/KUK/IVAN/M
.R/FOID PP7774441110
.R/PCTC /74951234567-1KUK/IVAN
.R/CTCE TEST//TEST.RU-1KUK/IVAN
1MUK/IVAN-A5
.R/TKNE HK1 2982410821861/1
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/M//MUK/IVAN
.R/PSPT HK1 7774441110/RU/01MAY76/MUK/IVAN/M
.R/FOID PP7774441110
.R/PCTC /74951234567-1MUK/IVAN
.R/CTCE TEST//TEST.RU-1MUK/IVAN
1TUK/IVAN-A5
.R/TKNE HK1 2982410821864/1
.R/OTHS HK1 DOCS/7774441110/PS
.R/DOCS HK1/P/RU/7774441110/RU/01MAY76/M//TUK/IVAN
.R/PSPT HK1 7774441110/RU/01MAY76/TUK/IVAN/M
.R/FOID PP7774441110
.R/PCTC /74951234567-1TUK/IVAN
.R/CTCE TEST//TEST.RU-1TUK/IVAN
-LED000S
-LED000T
-LED000E
-LED000Q
-LED000G
-LED000N
-LED000B
-LED000X
-LED000W
-LED000U
-LED000O
-LED000R
-LED001V
-LED000H
-LED000L
-LED000K
-LED000P
-LED008Z-PAD1
-LED000F
ENDPNL

})

$(defmacro PNL_UT_804_2
  time_create=$(dd)$(hhmi)
  date_dep=$(ddmon +1 en)
{
<<
MOWKB1H
.TJMRMUT $(time_create)
PNL
UT804/$(date_dep) VKO PART1
CFG/008C060Y
RBD C/CJIDA Y/YSTEQGNBXWUORVHLKPZF
AVAIL
 VKO  LED
C008
Y042
-LED000C
-LED000J
-LED000I
-LED000D
-LED000A
-LED004Y
1BOT/VASIA-E4
.L/05952K/UT
.L/04G0X6/DT
.R/TKNE HK1 2982410821867/1
.R/OTHS HK1 DOCS/8885552220/PS
.R/DOCS HK1/P/RU/8885552220/RU/01MAY76/M//BOT/VASIA
.R/PSPT HK1 8885552220/RU/01MAY76/BOT/VASIA/M
.R/FOID PP8885552220
.R/PCTC /74951234567-1BOT/VASIA
.R/CTCE TEST//TEST.RU-1BOT/VASIA
1KOT/VASIA-E4
.R/TKNE HK1 2982410821865/1
.R/OTHS HK1 DOCS/8885552220/PS
.R/DOCS HK1/P/RU/8885552220/RU/01MAY76/M//KOT/VASIA
.R/PSPT HK1 8885552220/RU/01MAY76/KOT/VASIA/M
.R/FOID PP8885552220
.R/PCTC /74951234567-1KOT/VASIA
.R/CTCE TEST//TEST.RU-1KOT/VASIA
1MOT/VASIA-E4
.R/TKNE HK1 2982410821866/1
.R/OTHS HK1 DOCS/8885552220/PS
.R/DOCS HK1/P/RU/8885552220/RU/01MAY76/M//MOT/VASIA
.R/PSPT HK1 8885552220/RU/01MAY76/MOT/VASIA/M
.R/FOID PP8885552220
.R/PCTC /74951234567-1MOT/VASIA
.R/CTCE TEST//TEST.RU-1MOT/VASIA
1SOT/VASIA-E4
.R/TKNE HK1 2982410821868/1
.R/OTHS HK1 DOCS/8885552220/PS
.R/DOCS HK1/P/RU/8885552220/RU/01MAY76/M//SOT/VASIA
.R/PSPT HK1 8885552220/RU/01MAY76/SOT/VASIA/M
.R/FOID PP8885552220
.R/PCTC /74951234567-1SOT/VASIA
.R/CTCE TEST//TEST.RU-1SOT/VASIA
-LED000S
-LED000T
-LED000E
-LED000Q
-LED000G
-LED000N
-LED000B
-LED000X
-LED000W
-LED000U
-LED000O
-LED000R
-LED000V
-LED000H
-LED000L
-LED000K
-LED000P
-LED014Z
1VOLKOV/VIACHESLAV IGOREVICH
.L/059529/UT
.L/1C4RV2/1U
.R/OTHS HK1 DOCS/3773765567/P
.R/DOCS HK1/P/RU/3773765567/RU/02JAN00/M/19APR23/VOLKOV
.RN//VIACHESLAV IGOREVICH
.R/PSPT HK1 P3773765567/RU/02JAN00/VOLKOV/VIACHESLAV IGOREVICH
.RN//M
.R/FOID PPP3773765567
.R/PCTC /79085298989-1VOLKOV/VIACHESLAV IGOREVICH
.R/PCTC /79085298989-1VOLKOV/VIACHESLAV IGOREVICH
.R/CTCE TEST..WL..USER1//MAIL.RU-1VOLKOV/VIACHESLAV IGOREVICH
-LED000F
ENDPNL

})

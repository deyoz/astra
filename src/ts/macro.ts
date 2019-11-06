include(ts/pnl/pnl_ut_c7y56.ts)

$(defmacro login
  user=PIKE
  passwd=PIKE
{
{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='MainDCS' ver='1' opr='' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <UserLogon>
       <term_version>201509-0173355</term_version>
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
}) #end-of-defmacro login

#########################################################################################

$(defmacro login2
  user=PIKE
  passwd=PIKE
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='MainDCS' ver='1' opr='' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <UserLogon>
      <term_version>201509-0173355</term_version>
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
}) #end-of-defmacro login2

#########################################################################################

$(defmacro KICK_IN
{
>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

}) #end-of-macro KICK_IN

#########################################################################################

$(defmacro KICK_IN_SILENT
{
>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

}) #end-of-macro KICK_IN_SILENT

#########################################################################################

$(defmacro PREPARE_SEASON_SCD
  airl
  depp
  arrp
  fltno
  craft=TU5
{
{<?xml version='1.0' encoding='CP866'?>
 <term>
  <query handle='0' id='season' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <write>
      <filter>
        <season>2</season>
      </filter>
      <SubrangeList>
        <subrange>
          <modify>insert</modify>
          <move_id>-1</move_id>
          <first>$(date_format %d.%m.%Y -1mon) 12:00:00</first>
          <last>$(date_format %d.%m.%Y +1mon) 12:00:00</last>
          <days>1234567</days>
          <dests  >
            <dest>
              <cod>$(depp)</cod>
              <company>$(airl)</company>
              <trip>$(fltno)</trip>
              <bc>$(craft)</bc>
              <takeoff>30.12.1899 10:15:00</takeoff>
              <y>-1</y>
            </dest>
            <dest>
              <cod>$(arrp)</cod>
              <land>30.12.1899 12:00:00</land>
            </dest>
          </dests  >
        </subrange>
      </SubrangeList>
    </write>
  </query>
</term>}
}) # end-of-macro PREPARE_SEASON_SCD

#########################################################################################

$(defmacro TKCREQ_ET_DISP
    from
    to
    ediref
    airl
    tickno
{UNB+SIRE:1+$(from)+$(to)+xxxxxx:xxxx+$(ediref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(ediref)"
MSG+:131"
ORG+1H:xxx+++$(airl)+Y+::xx+xxxxxx"
TKT+$(tickno)"
UNT+5+1"
UNZ+1+$(ediref)0001"}
) # end-of-macro TKCREQ_ET_DISP

#########################################################################################

$(defmacro TKCREQ_ET_COS
    from
    to
    ediref
    airl
    tickno
    cpnno
    status
    pult=xxxxxx
    depp=���
    arrp=���
    fltno=103
    subcls=Y
    depd=$(ddmmyy)
{UNB+SIRE:1+$(from)+$(to)+xxxxxx:xxxx+$(ediref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(ediref)"
MSG+:142"
ORG+1H:���+++$(airl)+Y+::xx+$(pult)"
EQN+1:TD"
TKT+$(tickno):T"
CPN+$(cpnno):$(status)"
TVL+$(depd)+$(depp)+$(arrp)+$(airl)+$(fltno)++1"
UNT+8+1"
UNZ+1+$(ediref)0001"}
) # end-of-macro TKCREQ_ET_COS

#########################################################################################

$(defmacro TKCRES_ET_DISP_1CPN
    from
    to
    ediref
    airl
    tickno
    status
    surname
    name
    depd
    depp
    arrp
    fltno=103
    subcls=Y
    cpnno=1
    depc=MOW
    arrc=LED
{UNB+SIRE:1+$(from)+$(to)+091030:0529+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:131+3"
TIF+$(surname)+$(name)"
TAI+0162"
RCI+$(airl):G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+$(depc)+$(arrc)"
ORG+$(airl):MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+$(tickno):T:1:3"
CPN+$(cpnno):$(status)"
TVL+$(depd):2205+$(depp)+$(arrp)+$(airl)+$(fltno):$(subcls)+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+$(ediref)0001"}
) # end-of-macro TKCRES_ET_DISP

#########################################################################################

$(defmacro TKCRES_ET_COS
    from
    to
    ediref
    tickno
    cpnno
    status
{UNB+SIRE:1+$(from)+$(to)+160408:0828+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:142+3"
EQN+1:TD"
TKT+$(tickno):T::3"
CPN+$(cpnno):$(status)::E"
UNT+6+1"
UNZ+1+$(ediref)0001"}
) # end-of-macro TKCRES_ET_COS

#########################################################################################

$(defmacro TKCRES_ET_COS_ERR
    from
    to
    ediref
    errcode
    errtext
{UNB+SIRE:1+$(from)+$(to)+151027:1527+$(ediref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(ediref)"
MSG+:142+7"
ERC+$(errcode)"
IFT+3+$(errtext)"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"}
) # end-of-macro TKCRES_ET_COS_ERR

#########################################################################################

$(defmacro TKCRES_ET_COS_FAKE_ERR
    from
    to
    ediref
    tickno
    cpnno
    status
    errcode
{UNB+SIRE:1+$(from)+$(to)+160408:0828+$(ediref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(ediref)"
MSG+:142+3"
EQN+1:TD"
TKT+$(tickno):T::3"
CPN+$(cpnno):$(status)::E"
ERC+$(errcode)"
UNT+7+1"
UNZ+1+$(ediref)0001"}
) # end-of-macro TKCRES_ET_COS

#########################################################################################

$(defmacro PNL_1PAX_1SEG
    airl
    depp
    arrp
    flt
    surname
    name
    airl_recloc=0840Z6
    gds_recloc=09T1B3
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl)$(flt)/$(ddmon +0 en) $(depp) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp)  $(arrp)
F060
C060
Y059
-$(arrp)000F
-$(arrp)000C
-$(arrp)001Y
1$(surname)/$(name)
.L/$(airl_recloc)/$(airl)
.L/$(gds_recloc)/1H
-$(arrp)000K
-$(arrp)000M
-$(arrp)000U
ENDPNL}
) #end-of-macro PNL_1PAX_1SEG

#########################################################################################

$(defmacro PNL_1PAX_2SEGS
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    surname
    name
    tickno
    cpnno
    airl_recloc=0840Z6
    gds_recloc=09T1B3
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1$(surname)/$(name)
.L/$(airl_recloc)/$(airl1)
.L/$(gds_recloc)/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro PNL_1PAX_2SEGS

#########################################################################################

$(defmacro PNL_1PAX_2SEGS_WITH_REMARKS
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    surname
    name
    tickno
    cpnno
    airl_recloc=0840Z6
    gds_recloc=09T1B3
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
$(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1$(surname)/$(name)
.L/$(airl_recloc)/$(airl1)
.L/$(gds_recloc)/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
.R/TKNE HK1 $(tickno)/$(cpnno)-1$(surname)/$(name)
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/$(surname)
.RN//$(name)-1$(surname)/$(name)
.R/PSPT HK1 ZB400522509/TJK/24JUL85/$(surname)/$(name)/M
.RN/-1$(surname)/$(name)
.R/FOID PPZB400522509-1$(surname)/$(name)
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro PNL_1PAX_2SEGS_WITH_REMARKS

#########################################################################################

$(defmacro PNL_2PAXES_1SEG
    airl1
    depp1
    arrp1
    flt1
    surname1
    name1
    surname2
    name2
    airl_recloc1=0840Z6
    gds_recloc1=09T1B3
    airl_recloc2=0840Z7
    gds_recloc2=09T1B4
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/Y
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y058
-LED000F
-LED000C
-LED002Y
1$(surname1)/$(name1)
.L/$(airl_recloc1)/$(airl1)
.L/$(gds_recloc1)/1H
1$(surname2)/$(name2)
.L/$(airl_recloc2)/$(airl1)
.L/$(gds_recloc2)/1H
ENDPNL}
) #end-of-macro PNL_2PAXES_1SEG

#########################################################################################

$(defmacro PNL_2PAXES_2SEGS_WITH_REMARKS
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    surname1
    name1
    tickno1
    cpnno1
    surname2
    name2
    tickno2
    cpnno2
    airl_recloc1=0840Z6
    gds_recloc1=09T1B3
    airl_recloc2=0840Z7
    gds_recloc2=09T1B4
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED002Y
1$(surname1)/$(name1)
.L/$(airl_recloc1)/$(airl1)
.L/$(gds_recloc1)/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
.R/TKNE HK1 $(tickno1)/$(cpnno1)-1$(surname1)/$(name1)
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/$(surname1)
.RN//$(name1)-1$(surname1)/$(name1)
.R/PSPT HK1 ZB400522509/TJK/24JUL85/$(surname1)/$(name1)/M
.RN/-1$(surname1)/$(name1)
.R/FOID PPZB400522509-1$(surname1)/$(name1)
1$(surname2)/$(name2)
.L/$(airl_recloc2)/$(airl1)
.L/$(gds_recloc2)/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
.R/TKNE HK1 $(tickno2)/$(cpnno2)-1$(surname2)/$(name2)
.R/DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/$(surname2)
.RN//$(name2)-1$(surname2)/$(name2)
.R/PSPT HK1 ZB400522510/TJK/24JUL85/$(surname2)/$(name2)/M
.RN/-1$(surname2)/$(name2)
.R/FOID PPZB400522510-1$(surname2)/$(name2)
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro PNL_2PAXES_2SEGS_WITH_REMARKS

#########################################################################################

$(defmacro PNL_1PAX_1INFT_1SEG
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    surname
    name
    tickno
    cpnno
    inftSurname
    inftName
    inftTickno
    inftCouponno
    airl_recloc=0840Z6
    gds_recloc=09T1B3
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED002Y
1$(surname)/$(name)
.L/$(airl_recloc)/$(airl1)
.L/$(gds_recloc)/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2315AR
.R/TKNE HK1 $(tickno)/$(cpnno)-1$(surname)/$(name)
.R/TKNE HK1 INF$(inftTickno)/$(inftCouponno)-1$(surname)/$(name)
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/$(surname)
.RN//$(name)-1$(surname)/$(name)
.R/PSPT HK1 ZB400522509/TJK/24JUL85/$(surname)/$(name)/M
.RN/-1$(surname)/$(name)
.R/FOID PPZB400522509-1$(surname)/$(name)
.R/INFT HK1 01JAN17 $(inftSurname)/$(inftName)-1$(surname)/$(name)
.R/DOCS HK1/P/RUS/1234566/RUS/01JAN17/MI/01JAN20/$(inftSurname)/$(inftName)_
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro PNL_1PAX_1INFT_1SEG

#########################################################################################

$(defmacro PNL_1PAX_3SEGS_WITH_REMARKS
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    airl3
    depp3
    arrp3
    flt3
    surname
    name
    tickno
    cpnno
    airl_recloc=0840Z6
    gds_recloc=09T1B3
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1$(surname)/$(name)
.L/$(airl_recloc)/$(airl1)
.L/$(gds_recloc)/1H
.O/$(airl2)$(flt2)Y$(dd +0 en)$(depp2)$(arrp2)2115HK
.O2/$(airl3)$(flt3)Y$(dd +0 en)$(depp3)$(arrp3)2330HK
.R/TKNE HK1 $(tickno)/$(cpnno)-1$(surname)/$(name)
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/$(surname)
.RN//$(name)-1$(surname)/$(name)
.R/PSPT HK1 ZB400522509/TJK/24JUL85/$(surname)/$(name)/M
.RN/-1$(surname)/$(name)
.R/FOID PPZB400522509-1$(surname)/$(name)
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro PNL_1PAX_3SEGS

#########################################################################################

$(defmacro PNL_2PAXES_3SEGS_WITH_REMARKS
    airl1
    depp1
    arrp1
    flt1
    airl2
    depp2
    arrp2
    flt2
    airl3
    depp3
    arrp3
    flt3
    surname1
    name1
    tickno1
    cpnno1
    pax1subclsdotO
    pax1subclsdotO2
    airl_recloc1
    gds_recloc1
    surname2
    name2
    tickno2
    cpnno2
    pax2subclsdotO
    pax2subclsdotO2
    airl_recloc2
    gds_recloc2
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl1)$(flt1)/$(ddmon +0 en) $(depp1) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp1)  $(arrp1)
F060
C060
Y059
-LED000F
-LED000C
-LED002Y
1$(surname1)/$(name1)
.L/$(airl_recloc1)/$(airl1)
.L/$(gds_recloc1)/1H
.O/$(airl2)$(flt2)$(pax1subclsdotO)$(dd +0 en)$(depp2)$(arrp2)2115HK
.O2/$(airl3)$(flt3)$(pax1subclsdotO2)$(dd +0 en)$(depp3)$(arrp3)2330HK
.R/TKNE HK1 $(tickno1)/$(cpnno1)-1$(surname1)/$(name1)
.R/DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/$(surname1)
.RN//$(name1)-1$(surname1)/$(name1)
.R/PSPT HK1 ZB400522509/TJK/24JUL85/$(surname1)/$(name1)/M
.RN/-1$(surname1)/$(name1)
.R/FOID PPZB400522509-1$(surname1)/$(name1)
1$(surname2)/$(name2)
.L/$(airl_recloc2)/$(airl2)
.L/$(gds_recloc2)/1H
.O/$(airl2)$(flt2)$(pax2subclsdotO)$(dd +0 en)$(depp2)$(arrp2)2115HK
.O2/$(airl3)$(flt3)$(pax2subclsdotO2)$(dd +0 en)$(depp3)$(arrp3)2330HK
.R/TKNE HK1 $(tickno2)/$(cpnno2)-1$(surname2)/$(name2)
.R/DOCS HK1/P/TJK/400522512/TJK/24JUL86/M/05FEB22/$(surname2)
.RN//$(name2)-1$(surname2)/$(name2)
.R/PSPT HK1 ZB400522512/TJK/24JUL86/$(surname2)/$(name2)/M
.RN/-1$(surname2)/$(name2)
.R/FOID PPZB400522512-1$(surname2)/$(name2)
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro PNL_2PAXES_3SEGS_WITH_REMARKS

#########################################################################################

$(defmacro PREPARE_FLIGHT_1PAX_1SEG
    airl=��
    flt=103
    depp=���
    arrp=���
    surname=REPIN
    name=IVAN
    cls=�
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl))
                     $(get_lat_code aer $(depp))
                     $(get_lat_code aer $(arrp))
                     $(flt))
$(make_spp)

<<
$(PNL_1PAX_1SEG $(get_lat_code awk $(airl))
                $(get_lat_code aer $(depp))
                $(get_lat_code aer $(arrp))
                $(flt)
                $(surname) $(name))

$(auto_set_craft $(get_dep_point_id $(depp) $(airl) $(flt) $(yymmdd +0)))
}) #end-of-macro PREPARE_FLIGHT_1PAX_1SEG

#########################################################################################

$(defmacro PREPARE_FLIGHT_1PAX_2SEGS_WITH_REMARKS
    airl1=��
    flt1=103
    depp1=���
    arrp1=���
    airl2=��
    flt2=2278
    depp2=���
    arrp2=���
    surname=REPIN
    name=IVAN
    tickno=2982401841689
    cpnno=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(make_spp)

<<
$(PNL_1PAX_2SEGS_WITH_REMARKS $(airl1) $(depp1) $(arrp1) $(flt1)
                              $(airl2) $(depp2) $(arrp2) $(flt2) $(surname) $(name)
                              $(tickno) $(cpnno))

>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref) �� 2982401841689)
<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(last_edifact_ref) $(airl1) $(tickno) I $(surname) $(name) $(ddmmyy) $(depp1) $(arrp1) $(flt1))

$(auto_set_craft $(get_dep_point_id ��� �� 103 $(yymmdd +0)))
}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_1PAX_2SEGS
    airl1=��
    flt1=103
    depp1=���
    arrp1=���
    airl2=�7
    flt2=1027
    depp2=���
    arrp2=���
    surname=REPIN
    name=IVAN
    tickno=2982401841689
    cpnno=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(make_spp)

<<
$(PNL_1PAX_2SEGS $(airl1) $(depp1) $(arrp1) $(flt1)
                 $(airl2) $(depp2) $(arrp2) $(flt2) $(surname) $(name)
                 $(tickno) $(cpnno))

$(auto_set_craft $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)))
}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_2PAXES_1SEG
    airl=��
    flt=103
    depp=���
    arrp=���
    surname1=�����
    name1=����
    surname2=������
    name2=����
{

$(set airl_lat $(get_lat_code awk $(airl)))
$(set depp_lat $(get_lat_code aer $(depp)))
$(set arrp_lat $(get_lat_code aer $(arrp)))

$(PREPARE_SEASON_SCD $(get airl_lat)
                     $(get depp_lat)
                     $(get arrp_lat)
                     $(flt))
$(make_spp)

<<
$(PNL_2PAXES_1SEG $(get airl_lat)
                  $(get depp_lat)
                  $(get arrp_lat)
                  $(flt)
                  $(surname1) $(name1)
                  $(surname2) $(name2))

$(auto_set_craft $(get_dep_point_id $(depp) $(airl) $(flt) $(yymmdd +0)))
}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_2PAXES_2SEGS_WITH_REMARKS
    airl1=��
    flt1=103
    depp1=���
    arrp1=���
    airl2=�7
    flt2=1027
    depp2=���
    arrp2=���
    surname1=REPIN
    name1=IVAN
    tickno1=2982401841689
    cpnno1=1
    surname2=PETROV
    name2=PETR
    tickno2=2982401841612
    cpnno2=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(make_spp)

<<
$(PNL_2PAXES_2SEGS_WITH_REMARKS $(airl1) $(depp1) $(arrp1) $(flt1)
                                $(airl2) $(depp2) $(arrp2) $(flt2)
                                $(surname1) $(name1) $(tickno1) $(cpnno1)
                                $(surname2) $(name2) $(tickno2) $(cpnno2))

$(auto_set_craft $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)))

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref1) �� $(tickno1))
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref0) �� $(tickno2))

<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(get edi_ref1) $(airl1) $(tickno1) I $(surname1) $(name1) $(ddmmyy) $(depp1) $(arrp1) $(flt1) Y $(cpnno1))
<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(get edi_ref0) $(airl1) $(tickno2) I $(surname2) $(name2) $(ddmmyy) $(depp1) $(arrp1) $(flt1) Y $(cpnno2))

}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_1PAX_1INFT_1SEG
    airl1=��
    flt1=103
    depp1=���
    arrp1=���
    airl2=�7
    flt2=1027
    depp2=���
    arrp2=���
    surname=REPIN
    name=IVAN
    tickno=2982401841689
    cpnno=1
    inftSurname=PETROV
    inftName=PETR
    inftTickno=2982401841612
    inftCouponno=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(make_spp)

<<
$(PNL_1PAX_1INFT_1SEG $(airl1) $(depp1) $(arrp1) $(flt1)
                      $(airl2) $(depp2) $(arrp2) $(flt2)
                      $(surname) $(name) $(tickno) $(cpnno)
                      $(inftSurname) $(inftName) $(inftTickno) $(inftCouponno))

$(auto_set_craft $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)))

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref1) �� $(tickno))
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref0) �� $(inftTickno))

<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(get edi_ref1) $(airl1) $(tickno) I $(surname) $(name) $(ddmmyy) $(depp1) $(arrp1) $(flt1) Y $(cpnno))
<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(get edi_ref0) $(airl1) $(inftTickno) I $(inftSurname) $(inftName) $(ddmmyy) $(depp1) $(arrp1) $(flt1) Y $(inftCouponno))

}) #end-of-macro

#########################################################################################

$(defmacro OPEN_CHECKIN
    point_id
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(point_id)</point_id>
          <tripstages>
            <stage>
              <stage_id>10</stage_id>
              <act>$(date_format %d.%m.%Y -24h) 00:41:00</act>
            </stage>
            <stage>
              <stage_id>20</stage_id>
              <act>$(date_format %d.%m.%Y -24h) 00:41:00</act>
            </stage>
          </tripstages>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>}

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED' code='0'>����� �ᯥ譮 ��࠭���</message>
    </command>

}) #end-of-macro

#########################################################################################

$(defmacro SEARCH_EMD_BY_DOC_NO
    point_dep
    emd_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='EMDSearch' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <SearchEMDByDocNo>
       <point_id>$(point_dep)</point_id>
       <EmdNoEdit>$(emd_no)</EmdNoEdit>
     </SearchEMDByDocNo>
   </query>
 </term>}

}) #end-of-macro

#########################################################################################

$(defmacro SEARCH_ET_BY_TICK_NO
    point_dep
    tick_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchETByTickNo>
      <point_id>$(point_dep)</point_id>
      <TickNoEdit>$(tick_no)</TickNoEdit>
    </SearchETByTickNo>
  </query>
</term>}

}) #end-of-macro

#########################################################################################

$(defmacro REQUEST_AC_BY_TICK_NO_CPN_NO
    point_dep
    tick_no
    cpn_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='RequestAC' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <RequestControl>
      <point_id>$(point_dep)</point_id>
      <TickNoEdit>$(tick_no)</TickNoEdit>
      <TickCpnNo>$(cpn_no)</TickCpnNo>
    </RequestControl>
  </query>
</term>}

}) #end-of-macro

#########################################################################################

$(defmacro EMD_TEXT_VIEW
    point_dep
    tick_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='EMDSearch' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <EMDTextView>
       <point_id>$(point_dep)</point_id>
       <ticket_no>$(tick_no)</ticket_no>
       <pax_id/>
       <coupon_no/>
       <ticket_rem/>
     </EMDTextView>
   </query>
 </term>}

}) #end-of-macro

#########################################################################################

$(defmacro SAVE_ET_DISP
    point_id
    tickno=2986120030297
    surname=REPIN
    name=IVAN
    airl=��
    dcs_addr=UTDC
    ets_addr=UTET
    recloc=G4LK6W
    depp=DME
    arrp=LED
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchETByTickNo>
      <point_id>$(point_id)</point_id>
      <TickNoEdit>$(tickno)</TickNoEdit>
    </SearchETByTickNo>
  </query>
</term>}

>>
$(TKCREQ_ET_DISP $(dcs_addr) $(ets_addr) $(last_edifact_ref 0) $(airl) $(tickno))
<<
$(TKCRES_ET_DISP_1CPN $(ets_addr) $(dcs_addr) $(last_edifact_ref) $(airl) $(tickno) I $(surname) $(name) $(ddmmyy) $(depp) $(arrp))

$(KICK_IN_SILENT)

}) #end-of-macro

#########################################################################################

$(defmacro CHECKIN_PAX
    pax_id
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    tickno
    pers_type
    doc_issue_country
    doc_no
    doc_nationality
    doc_birth_date
    doc_expiry_date
    doc_gender
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl)</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>$(doc_issue_country)</issue_country>
                <no>$(doc_no)</no>
                <nationality>$(doc_nationality)</nationality>
                <birth_date>$(doc_birth_date) 00:00:00</birth_date>
                <expiry_date>$(doc_expiry_date) 00:00:00</expiry_date>
                <gender>$(doc_gender)</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>�</subclass>
              <bag_pool_num/>
              <transfer/>
              <rems/>
              <fqt_rems/>
              <norms/>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
      </segments>
      <excess>0</excess>
      <hall>1</hall>
      <paid_bags>
        <paid_bag>
          <bag_type/>
          <weight>0</weight>
          <rate_id/>
          <rate_trfer/>
        </paid_bag>
      </paid_bags>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro CHECKIN_PAX

#########################################################################################

$(defmacro CHECKIN_PAX_WITH_VISA
    pax_id
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    tickno
    pers_type
    doc_issue_country
    doc_no
    doc_nationality
    doc_birth_date
    doc_expiry_date
    doc_gender
    visa_no
    visa_issue_place
    visa_issue_date
    visa_expiry_date
    visa_applic_country
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl)</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>$(doc_issue_country)</issue_country>
                <no>$(doc_no)</no>
                <nationality>$(doc_nationality)</nationality>
                <birth_date>$(doc_birth_date) 00:00:00</birth_date>
                <expiry_date>$(doc_expiry_date) 00:00:00</expiry_date>
                <gender>$(doc_gender)</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco>
                <type>V</type>
                <birth_place/>
                <no>$(visa_no)</no>
                <issue_place>$(visa_issue_place)</issue_place>
                <issue_date>$(visa_issue_date) 00:00:00</issue_date>
                <expiry_date>$(visa_expiry_date) 00:00:00</expiry_date>
                <applic_country>$(visa_applic_country)</applic_country>
              </doco>
              <addresses/>
              <subclass>�</subclass>
              <bag_pool_num/>
              <transfer/>
              <rems/>
              <fqt_rems/>
              <norms/>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
      </segments>
      <excess>0</excess>
      <hall>1</hall>
      <paid_bags>
        <paid_bag>
          <bag_type/>
          <weight>0</weight>
          <rate_id/>
          <rate_trfer/>
        </paid_bag>
      </paid_bags>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro CHECKIN_PAX_WITH_VISA

#########################################################################################

$(defmacro CHECKIN_PAX_WITH_VISA_AND_DOCA
    pax_id
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    tickno
    pers_type
    doc_issue_country
    doc_no
    doc_nationality
    doc_birth_date
    doc_expiry_date
    doc_gender
    visa_no
    visa_issue_place
    visa_issue_date
    visa_expiry_date
    visa_applic_country
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl)</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>$(doc_issue_country)</issue_country>
                <no>$(doc_no)</no>
                <nationality>$(doc_nationality)</nationality>
                <birth_date>$(doc_birth_date) 00:00:00</birth_date>
                <expiry_date>$(doc_expiry_date) 00:00:00</expiry_date>
                <gender>$(doc_gender)</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco>
                <type>V</type>
                <birth_place/>
                <no>$(visa_no)</no>
                <issue_place>$(visa_issue_place)</issue_place>
                <issue_date>$(visa_issue_date) 00:00:00</issue_date>
                <expiry_date>$(visa_expiry_date) 00:00:00</expiry_date>
                <applic_country>$(visa_applic_country)</applic_country>
              </doco>
              <addresses>
                <doca>
                  <type>D</type>
                  <country>USA</country>
                  <region>REGION</region>
                  <address>ADDRESS</address>
                  <city>CITY</city>
                  <postal_code>112233</postal_code>
                </doca>
                <doca>
                  <type>R</type>
                  <country>BLR</country>
                  <region>RESIDENCE REGION</region>
                  <address>RESIDENCE ADDRESS</address>
                  <city>RESIDENCE CITY</city>
                  <postal_code>001122</postal_code>
                </doca>
              </addresses>
              <subclass>�</subclass>
              <bag_pool_num/>
              <transfer/>
              <rems/>
              <fqt_rems/>
              <norms/>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
      </segments>
      <excess>0</excess>
      <hall>1</hall>
      <paid_bags>
        <paid_bag>
          <bag_type/>
          <weight>0</weight>
          <rate_id/>
          <rate_trfer/>
        </paid_bag>
      </paid_bags>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro CHECKIN_PAX_WITH_VISA_AND_DOCA

#########################################################################################

$(defmacro CANCEL_PAX
    pax_id
    grp_id
    tid
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    tickno
    pers_type
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>�</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
              <refuse>�</refuse>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document/>
              <transfer/>
              <doco/>
              <addresses/>
              <subclass>�</subclass>
              <bag_pool_num/>
              <tid>$(tid)</tid>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro CANCEL_PAX

#########################################################################################

$(defmacro CHECKIN_CREW
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    pers_type
    doc_issue_country
    doc_no
    doc_nationality
    doc_birthdate
    doc_expdate
    doc_gender
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>1</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>�</class>
          <status>E</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl)</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id/>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>-</ticket_no>
              <coupon_no/>
              <ticket_rem>TKNA</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>$(doc_issue_country)</issue_country>
                <no>$(doc_no)</no>
                <nationality>$(doc_nationality)</nationality>
                <birth_date>$(doc_birthdate) 00:00:00</birth_date>
                <expiry_date>$(doc_expdate) 00:00:00</expiry_date>
                <gender>$(doc_gender)</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass> </subclass>
              <bag_pool_num/>
              <transfer/>
              <rems>
                <rem>
                  <rem_code>CREW</rem_code>
                  <rem_text>CREW</rem_text>
                </rem>
              </rems>
              <fqt_rems/>
              <norms/>
            </pax>
          </passengers>
          <service_payment/>
          <paid_bag_emd/>
        </segment>
      </segments>
      <hall>1</hall>
      <paid_bags/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro CHECKIN_CREW

#########################################################################################

$(defmacro CHECKIN_CREW_WITH_VISA
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    pers_type
    doc_issue_country
    doc_no
    doc_nationality
    doc_birthdate
    doc_expdate
    doc_gender
    visa_no
    visa_issue_place
    visa_issue_date
    visa_expiry_date
    visa_applic_country
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>1</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>�</class>
          <status>E</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl)</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id/>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>-</ticket_no>
              <coupon_no/>
              <ticket_rem>TKNA</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>$(doc_issue_country)</issue_country>
                <no>$(doc_no)</no>
                <nationality>$(doc_nationality)</nationality>
                <birth_date>$(doc_birthdate) 00:00:00</birth_date>
                <expiry_date>$(doc_expdate) 00:00:00</expiry_date>
                <gender>$(doc_gender)</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco>
                <type>V</type>
                <birth_place/>
                <no>$(visa_no)</no>
                <issue_place>$(visa_issue_place)</issue_place>
                <issue_date>$(visa_issue_date) 00:00:00</issue_date>
                <expiry_date>$(visa_expiry_date) 00:00:00</expiry_date>
                <applic_country>$(visa_applic_country)</applic_country>
              </doco>
              <addresses/>
              <subclass> </subclass>
              <bag_pool_num/>
              <transfer/>
              <rems>
                <rem>
                  <rem_code>CREW</rem_code>
                  <rem_text>CREW</rem_text>
                </rem>
              </rems>
              <fqt_rems/>
              <norms/>
            </pax>
          </passengers>
          <service_payment/>
          <paid_bag_emd/>
        </segment>
      </segments>
      <hall>1</hall>
      <paid_bags/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro CHECKIN_CREW_WITH_VISA

#########################################################################################

$(defmacro CANCEL_CHECKIN_CREW
    pax_id
    grp_id
    tid
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    pers_type
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(airp_dep)</airp_dep>
          <airp_arv>$(airp_arv)</airp_arv>
          <class>�</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
              <refuse>�</refuse>
              <ticket_no>-</ticket_no>
              <coupon_no/>
              <ticket_rem>TKNA</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <subclass> </subclass>
              <bag_pool_num/>
              <tid>$(tid)</tid>
            </pax>
          </passengers>
          <service_payment/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro CANCEL_CHECKIN_CREW

#########################################################################################

$(defmacro GET_EVENTS
    point_id
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='Events' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <GetEvents>
    <dev_model/>
    <fmt_type/>
    <prnParams/>
    <point_id>$(point_id)</point_id>
    <EventsTypes>
      <type>���</type>
      <type>���</type>
      <type>���</type>
      <type>���</type>
      <type>���</type>
    </EventsTypes>
    <LoadForm/>
  </GetEvents>
  </query>
</term>}

}) #end-of-macro

#########################################################################################

$(defmacro WRITE_DESTS
    point_dep
    point_arv
    move_id
    airl
    flt
    depp
    arrp
    depd
    dept
    arrd
    arrt
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteDests>
      <data>
        <move_id>$(get move_id)</move_id>
        <canexcept>1</canexcept>
        <reference/>
        <dests>
          <dest>
            <modify/>
            <point_id>$(get point_dep)</point_id>
            <point_num>0</point_num>
            <airp>$(depp)</airp>
            <airline>$(airl)</airline>
            <flt_no>$(flt)</flt_no>
            <craft>735</craft>
            <scd_out>$(depd) $(dept)</scd_out>
            <act_out>$(depd) $(dept)</act_out>
            <trip_type>�</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>1</pr_reg>
          </dest>
          <dest>
            <point_id>$(point_arv)</point_id>
            <point_num>1</point_num>
            <first_point>$(point_dep)</first_point>
            <airp>$(arrp)</airp>
            <scd_in>$(arrd) $(arrt)</scd_in>
            <trip_type>�</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
        </dests>
      </data>
    </WriteDests>
  </query>
</term>}

}) #end-of-macro WRITE_DESTS


#########################################################################################

$(defmacro ET_DISP_61_UT_REQS
{

# �室�� ��ᯫ�� �� ���ᠦ�஢
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 60) �� 2986145212943)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 59) �� 2982425696898)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 58) �� 2982425696897)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 57) �� 2985085963078)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 56) �� 2982425697797)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 55) �� 2986145134261)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 54) �� 2986145134264)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 53) �� 2986145134262)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 52) �� 2986145134263)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 51) �� 2982409342779)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 50) �� 2985523437721)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 49) �� 2986145159105)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 48) �� 2982425690987)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 47) �� 2986145108674)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 46) �� 2986145143701)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 45) �� 2986145115578)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 44) �� 2986145143703)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 43) �� 2986145143704)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 42) �� 2986145132546)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 41) �� 2982425673353)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 40) �� 2986145053217)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 39) �� 2986145053218)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 38) �� 2986145051632)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 37) �� 2986145051633)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 36) �� 2986145054209)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 35) �� 2986145108615)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 34) �� 2986145092420)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 33) �� 2986145092419)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 32) �� 2982409342340)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 31) �� 2982425650976)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 30) �� 2982425641071)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 29) �� 2982425647848)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 28) �� 2982425647542)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 27) �� 2982425659617)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 26) �� 2982425643104)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 25) �� 2985588425342)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 24) �� 2985588425343)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 23) �� 2986013087180)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 22) �� 2986144854221)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 21) �� 2986144854225)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 20) �� 2986144854224)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 19) �� 2986144854222)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 18) �� 2986144854220)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 17) �� 2986144854223)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 16) �� 2985587608896)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 15) �� 2982425560779)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 14) �� 2982425560781)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 13) �� 2982425560780)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 12) �� 2982425505291)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 11) �� 2982425619341)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 10) �� 2982425622093)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 9) �� 2986144751885)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 8) �� 2982425618100)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 7) �� 2982425618101)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 6) �� 2982425618102)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 5) �� 2982425622116)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 4) �� 2982425459968)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 3) �� 2982425629499)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 2) �� 2982425530429)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 1) �� 2982425530428)
>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref 0) �� 2982425528249)

}) #end-of-macro ET_DISP_61_UT_REQS

#########################################################################################

$(defmacro INB_PNL_UT
    depp
    arrp
    fltno
    depd
    addr_to=MOWKK1H
    addr_from=TJMRMUT
{
<<
$(PNL_UT_C7Y56_PART1 $(depp) $(arrp) $(fltno) $(depd))
<<
$(PNL_UT_C7Y56_PART2 $(depp) $(arrp) $(fltno) $(depd))
<<
$(PNL_UT_C7Y56_PART3 $(depp) $(arrp) $(fltno) $(depd))
<<
$(PNL_UT_C7Y56_PART4 $(depp) $(arrp) $(fltno) $(depd))
<<
$(PNL_UT_C7Y56_PART5 $(depp) $(arrp) $(fltno) $(depd))
<<
$(PNL_UT_C7Y56_PART6 $(depp) $(arrp) $(fltno) $(depd))

}) #end-of-macro INB_PNL_UT

#########################################################################################

$(defmacro INB_ADL_UT_DEL2PAXES
    depp
    arrp
    fltno
    depd
    addr_to=MOWKK1H
    addr_from=TJMRMUT
{
<<
$(ADL_UT_C7Y56_DEL2PAXES $(depp) $(arrp) $(fltno) $(depd))

}) #end-if-macro INB_ADL_UT_DEL2PAXES

#########################################################################################

$(defmacro INB_ADL_UT_CHG1PAX
    depp
    arrp
    fltno
    depd
    addr_to=MOWKK1H
    addr_from=TJMRMUT
{
<<
$(ADL_UT_C7Y56_CHG1PAX $(depp) $(arrp) $(fltno) $(depd))

}) #end-of-macro INB_ADL_UT_CHG1PAX

#########################################################################################

$(defmacro CIRQ_61_UT_REQS_APPS_VERSION_21
    airl
    fltno
    depp
    arrp
    depd=$(yyyymmdd)
    arrd=$(yyyymmdd)
    dept=101500
    arrt=110000
{

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0305984920//P/20491009////VERGUNOV/VASILII LEONIDOVICH/19601104/M///N/N////.*

$(set msg_id1 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/KAZ/KAZ/N11024936//P/20261004////ALIMOV/TALGAT/19960511/M///N/N////.*

$(set msg_id2 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/KAZ/KAZ/N07298275//P/20210329////KHASSENOVA/ZULFIYA/19741106/F///N/N////.*

$(set msg_id3 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

$(set msg_id4 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0317833785//P/20201009////RIAZANOVA/IRINA GENNADEVNA/19721003/F///N/N////.*

$(set msg_id5 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0313361730//P/20241007////AKOPOV/ANDREI/19930621/M///N/N////.*

$(set msg_id6 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/VAG594936//O/20241007////BABAKHANOVA/KIRA/20100405/F///N/N////.*

$(set msg_id7 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0305555064//P/20241007////STIPIDI/ANGELINA/19820723/F///N/N////.*

$(set msg_id8 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/VIAG519994//O/20241007////AKOPOVA/OLIVIIA/20190822/F///N/N////.*

$(set msg_id9 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0306355301//P/20291005////KOBYLINSKIY/ALEKSEY/19861231/M///N/N////.*

$(set msg_id10 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/////P/////OZ/OFER//U///N/N////.*

$(set msg_id11 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0803963313//P/20241008////LUCHAK/OKSANA/19771022/F///N/N////.*

$(set msg_id12 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0319189298//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*

$(set msg_id13 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/652123387//P/20220110////BUMBURIDI/ODISSEI AFANASEVICH/19510514/M///N/N////.*

$(set msg_id14 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0319350828//P/20201007////CHEKMAREV/KONSTANTIN ALEKSANDROVICH/19900317/M///N/N////.*

$(set msg_id15 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

$(set msg_id16 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0307611933//P/20201007////VASILIADI/KSENIYA VALEREVNA/19840913/F///N/N////.*

$(set msg_id17 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/V��815247//O/20201007////CHEKMAREV/RONALD KONSTANTINOVICH/20180129/M///N/N////.*

$(set msg_id18 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0305350198//P/20350626////ZAINULLINA/RAISA GRIGOREVNA/19590102/F///N/N////.*

$(set msg_id19 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4611296643//P/20201007////KOTLIAR/VLADIMIR NIKOLAEVICH/19660117/M///N/N////.*

$(set msg_id20 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/6008404843//P/20491004////AGAFONOV/DENIS DMITRIEVICH/19881230/M///N/N////.*

$(set msg_id21 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4515495907//P/20491004////POLETAEVA/MARIIA DMITRIEVNA/19930807/F///N/N////.*

$(set msg_id22 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4503912696//P/20491004////ASTAFEV/DMITRII VLADIMIROVICH/19790707/M///N/N////.*

$(set msg_id23 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4510616333//P/20491004////TIKHOMIROVA/KRISTINA VALEREVNA/19870913/F///N/N////.*

$(set msg_id24 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4605592746//P/20491004////BALASHOV/SERGEI MIKHAILOVICH/19510831/M///N/N////.*

$(set msg_id25 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/726713638//P/20231113////BUMBURIDI/EKATERINA SERGEEVNA/19520331/F///N/N////.*

$(set msg_id26 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/728840142//P/20240523////MOKSOKHOEV/OLEG VIKTOROVICH/20110203/M///N/N////.*

$(set msg_id27 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/715704423//P/20210810////MOKSOKHOEV/VICTOR SERGEEVICH/19810907/M///N/N////.*

$(set msg_id28 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0305367719//P/20290930////RUBLEVA/MARINA/19801012/F///N/N////.*

$(set msg_id29 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/714051483//P/20210412////KAPANOVA/SVETLANA VLADISLAVOVNA/19720824/F///N/N////.*

$(set msg_id30 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0319013818//P/20380802////KRAVTSOVA/ELENA VLADIMIROVNA/19730627/F///N/N////.*

$(set msg_id31 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4606018370//P/20201004////KUZNETSOV/ILIA VLADIMIROVICH/19780804/M///N/N////.*

$(set msg_id32 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0319404210//P/20201004////TAGIROV/SERGEI/19740906/M///N/N////.*

$(set msg_id33 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/2512791944//P/20201005////CHETVERTKOV/EVGENII VLADIMIROVICH/19680207/M///N/N////.*

$(set msg_id34 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0305453906//P/20201003////IARINENKO/MAKSIM ALEKSEEVICH/19790430/M///N/N////.*

$(set msg_id35 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4505137601//P/20380116////BUGAEV/PAVEL/19820202/M///N/N////.*

$(set msg_id36 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0309013487//P/20380820////CHETVERIKOVA/JULIA/19791221/F///N/N////.*

$(set msg_id37 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/ISR/ISR/33322514//P/20690919////FUKS/LIUDMILA/19900707/F///N/N////.*

$(set msg_id38 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0312185128//P/20200928////KARUNA/ALBINA VALENTINOVNA/19670126/F///N/N////.*

$(set msg_id39 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/IV��827949//O/20200928////KARUNA/EKATERINA SERGEEVNA/20140724/F///N/N////.*

$(set msg_id40 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/IV��827950//O/20200928////KARUNA/ELIZAVETA SERGEEVNA/20140724/F///N/N////.*

$(set msg_id41 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/IV��848869//O/20200928////KARUNA/SERGEY SERGEEVICH/20140724/M///N/N////.*

$(set msg_id42 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0306152502//P/20200928////KARUNA/SERGEY VIKTOROVICH/19610101/M///N/N////.*

$(set msg_id43 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/IV��848868//O/20200928////KARUNA/SOFIYA SERGEEVNA/20140724/F///N/N////.*

$(set msg_id44 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0312220158//P/20290709////MAKEYAN/GEVORG SIMAVONOVICH/19921107/M///N/N////.*

$(set msg_id45 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0314612597//P/20200925////AVIDZBA/DZHIKHAN/19580901/F///N/N////.*

$(set msg_id46 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/I��083812//O/20200925////AVIDZBA/MARK/20141220/M///N/N////.*

$(set msg_id47 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0315047607//P/20200925////MKELBA/SALIMA/19871228/F///N/N////.*

$(set msg_id48 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0399187351//P/20380823////ATOMAS/NATALIA VALERIEVNA/19780423/F///N/N////.*

$(set msg_id49 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0308968576//P/20201001////BARSUK/TATIANA/19631016/F///N/N////.*

$(set msg_id50 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/4501742939//P/20201001////BURIAKOV/EVGENII EVGENEVICH/19750302/M///N/N////.*

$(set msg_id51 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0304440901//P/20201011////DIKOVA/MARIIA SERGEEVNA/19821024/F///N/N////.*

$(set msg_id52 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0310526187//P/20201001////DMITRIEVA/IULIIA ALEKSANDROVNA/19850823/F///N/N////.*

$(set msg_id53 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/V��841650//O/20201001////CHARKOV/NIKOLAI GENNADEVICH/20180811/M///N/N////.*

$(set msg_id54 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/V��568572//O/20201001////CHARKOV/MIKHAIL GENNADEVICH/20151121/M///N/N////.*

$(set msg_id55 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0312089903//P/20201001////SAMOILENKO/IGOR ALEKSANDROVICH/19670619/M///N/N////.*

$(set msg_id56 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/516703310//P/20221024////SERGIENKO/ALEKSANDR VIKTOROVICH/19860527/M///N/N////.*

$(set msg_id57 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0317834955//P/20201002////STARODUBTSEVA/OLGA ANDREEVNA/19900111/F///N/N////.*

$(set msg_id58 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/V��506585//O/20200922////KHARCHENKO/MARIIA SEMENOVNA/20150714/F///N/N////.*

$(set msg_id59 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0315043239//P/20200922////KHARCHENKO/NATALIA ALEKSANDROVNA/19910412/F///N/N////.*

$(set msg_id60 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/22/1/P/RUS/RUS/0315042043//P/20200922////KHARCHENKO/SEMEN VIACHESLAVOVICH/19800525/M///N/N////.*

$(set msg_id61 $(capture 1))

}) #end-if-macro CIRQ_61_UT_REQS

#########################################################################################

$(defmacro CIRQ_61_UT_REQS_APPS_VERSION_26
    airl
    fltno
    depp
    arrp
    depd=$(yyyymmdd)
    arrd=$(yyyymmdd)
    dept=101500
    arrt=110000
{

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0305984920//P//20491009////VERGUNOV/VASILII LEONIDOVICH/19601104/M///N/N////.*

$(set msg_id1 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/KAZ/KAZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M///N/N////.*

$(set msg_id2 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/KAZ/KAZ/N07298275//P//20210329////KHASSENOVA/ZULFIYA/19741106/F///N/N////.*

$(set msg_id3 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

$(set msg_id4 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0317833785//P//20201009////RIAZANOVA/IRINA GENNADEVNA/19721003/F///N/N////.*

$(set msg_id5 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0313361730//P//20241007////AKOPOV/ANDREI/19930621/M///N/N////.*

$(set msg_id6 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/VAG594936//O//20241007////BABAKHANOVA/KIRA/20100405/F///N/N////.*

$(set msg_id7 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0305555064//P//20241007////STIPIDI/ANGELINA/19820723/F///N/N////.*

$(set msg_id8 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/VIAG519994//O//20241007////AKOPOVA/OLIVIIA/20190822/F///N/N////.*

$(set msg_id9 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0306355301//P//20291005////KOBYLINSKIY/ALEKSEY/19861231/M///N/N////.*

$(set msg_id10 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/////P//////OZ/OFER//U///N/N////.*

$(set msg_id11 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0803963313//P//20241008////LUCHAK/OKSANA/19771022/F///N/N////.*

$(set msg_id12 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0319189298//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*

$(set msg_id13 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/652123387//P//20220110////BUMBURIDI/ODISSEI AFANASEVICH/19510514/M///N/N////.*

$(set msg_id14 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0319350828//P//20201007////CHEKMAREV/KONSTANTIN ALEKSANDROVICH/19900317/M///N/N////.*

$(set msg_id15 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M///N/N////.*

$(set msg_id16 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0307611933//P//20201007////VASILIADI/KSENIYA VALEREVNA/19840913/F///N/N////.*

$(set msg_id17 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/V��815247//O//20201007////CHEKMAREV/RONALD KONSTANTINOVICH/20180129/M///N/N////.*

$(set msg_id18 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0305350198//P//20350626////ZAINULLINA/RAISA GRIGOREVNA/19590102/F///N/N////.*

$(set msg_id19 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4611296643//P//20201007////KOTLIAR/VLADIMIR NIKOLAEVICH/19660117/M///N/N////.*

$(set msg_id20 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/6008404843//P//20491004////AGAFONOV/DENIS DMITRIEVICH/19881230/M///N/N////.*

$(set msg_id21 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4515495907//P//20491004////POLETAEVA/MARIIA DMITRIEVNA/19930807/F///N/N////.*

$(set msg_id22 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4503912696//P//20491004////ASTAFEV/DMITRII VLADIMIROVICH/19790707/M///N/N////.*

$(set msg_id23 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4510616333//P//20491004////TIKHOMIROVA/KRISTINA VALEREVNA/19870913/F///N/N////.*

$(set msg_id24 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4605592746//P//20491004////BALASHOV/SERGEI MIKHAILOVICH/19510831/M///N/N////.*

$(set msg_id25 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/726713638//P//20231113////BUMBURIDI/EKATERINA SERGEEVNA/19520331/F///N/N////.*

$(set msg_id26 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/728840142//P//20240523////MOKSOKHOEV/OLEG VIKTOROVICH/20110203/M///N/N////.*

$(set msg_id27 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/715704423//P//20210810////MOKSOKHOEV/VICTOR SERGEEVICH/19810907/M///N/N////.*

$(set msg_id28 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0305367719//P//20290930////RUBLEVA/MARINA/19801012/F///N/N////.*

$(set msg_id29 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/714051483//P//20210412////KAPANOVA/SVETLANA VLADISLAVOVNA/19720824/F///N/N////.*

$(set msg_id30 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0319013818//P//20380802////KRAVTSOVA/ELENA VLADIMIROVNA/19730627/F///N/N////.*

$(set msg_id31 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4606018370//P//20201004////KUZNETSOV/ILIA VLADIMIROVICH/19780804/M///N/N////.*

$(set msg_id32 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0319404210//P//20201004////TAGIROV/SERGEI/19740906/M///N/N////.*

$(set msg_id33 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/2512791944//P//20201005////CHETVERTKOV/EVGENII VLADIMIROVICH/19680207/M///N/N////.*

$(set msg_id34 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0305453906//P//20201003////IARINENKO/MAKSIM ALEKSEEVICH/19790430/M///N/N////.*

$(set msg_id35 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4505137601//P//20380116////BUGAEV/PAVEL/19820202/M///N/N////.*

$(set msg_id36 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0309013487//P//20380820////CHETVERIKOVA/JULIA/19791221/F///N/N////.*

$(set msg_id37 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/ISR/ISR/33322514//P//20690919////FUKS/LIUDMILA/19900707/F///N/N////.*

$(set msg_id38 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0312185128//P//20200928////KARUNA/ALBINA VALENTINOVNA/19670126/F///N/N////.*

$(set msg_id39 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/IV��827949//O//20200928////KARUNA/EKATERINA SERGEEVNA/20140724/F///N/N////.*

$(set msg_id40 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/IV��827950//O//20200928////KARUNA/ELIZAVETA SERGEEVNA/20140724/F///N/N////.*

$(set msg_id41 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/IV��848869//O//20200928////KARUNA/SERGEY SERGEEVICH/20140724/M///N/N////.*

$(set msg_id42 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0306152502//P//20200928////KARUNA/SERGEY VIKTOROVICH/19610101/M///N/N////.*

$(set msg_id43 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/IV��848868//O//20200928////KARUNA/SOFIYA SERGEEVNA/20140724/F///N/N////.*

$(set msg_id44 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0312220158//P//20290709////MAKEYAN/GEVORG SIMAVONOVICH/19921107/M///N/N////.*

$(set msg_id45 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0314612597//P//20200925////AVIDZBA/DZHIKHAN/19580901/F///N/N////.*

$(set msg_id46 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/I��083812//O//20200925////AVIDZBA/MARK/20141220/M///N/N////.*

$(set msg_id47 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0315047607//P//20200925////MKELBA/SALIMA/19871228/F///N/N////.*

$(set msg_id48 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0399187351//P//20380823////ATOMAS/NATALIA VALERIEVNA/19780423/F///N/N////.*

$(set msg_id49 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0308968576//P//20201001////BARSUK/TATIANA/19631016/F///N/N////.*

$(set msg_id50 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/4501742939//P//20201001////BURIAKOV/EVGENII EVGENEVICH/19750302/M///N/N////.*

$(set msg_id51 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0304440901//P//20201011////DIKOVA/MARIIA SERGEEVNA/19821024/F///N/N////.*

$(set msg_id52 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0310526187//P//20201001////DMITRIEVA/IULIIA ALEKSANDROVNA/19850823/F///N/N////.*

$(set msg_id53 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/V��841650//O//20201001////CHARKOV/NIKOLAI GENNADEVICH/20180811/M///N/N////.*

$(set msg_id54 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/V��568572//O//20201001////CHARKOV/MIKHAIL GENNADEVICH/20151121/M///N/N////.*

$(set msg_id55 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0312089903//P//20201001////SAMOILENKO/IGOR ALEKSANDROVICH/19670619/M///N/N////.*

$(set msg_id56 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/516703310//P//20221024////SERGIENKO/ALEKSANDR VIKTOROVICH/19860527/M///N/N////.*

$(set msg_id57 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0317834955//P//20201002////STARODUBTSEVA/OLGA ANDREEVNA/19900111/F///N/N////.*

$(set msg_id58 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/V��506585//O//20200922////KHARCHENKO/MARIIA SEMENOVNA/20150714/F///N/N////.*

$(set msg_id59 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0315043239//P//20200922////KHARCHENKO/NATALIA ALEKSANDROVNA/19910412/F///N/N////.*

$(set msg_id60 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/$(airl)$(fltno)/$(depp)/$(arrp)/$(depd)/$(dept)/$(arrd)/$(arrt)/PRQ/34/1/P/RUS/RUS/0315042043//P//20200922////KHARCHENKO/SEMEN VIACHESLAVOVICH/19800525/M///N/N////.*

$(set msg_id61 $(capture 1))

}) #end-if-macro CIRQ_61_UT_REQS

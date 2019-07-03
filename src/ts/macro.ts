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

$(defmacro ASSIGN_GATE
  point_id=0
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle="0" id="sopp" ver="1" opr="PIKE" screen="SOPP.EXE" mode="STAND" lang="RU" term_id="2479792165">
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(point_id)</point_id>
          <stations>
            <work_mode mode="П">
              <name>1</name>
            </work_mode>
          </stations>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>}
}) # end-of-macro ASSIGN_GATE

#########################################################################################

$(defmacro PREPARE_SEASON_SCD
  airl=UT
  depp=DME
  arrp=AER
  fltno=747
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
    depp=ДМД
    arrp=ПЛК
    fltno=103
    subcls=Y
    depd=$(ddmmyy)
{UNB+SIRE:1+$(from)+$(to)+xxxxxx:xxxx+$(ediref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(ediref)"
MSG+:142"
ORG+1H:МОВ+++$(airl)+Y+::xx+$(pult)"
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
-LED000F
-LED000C
-LED001Y
1$(surname)/$(name)
.L/$(airl_recloc)/$(airl)
.L/$(gds_recloc)/1H
-LED000K
-LED000M
-LED000U
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
    airl=ЮТ
    flt=103
    depp=ДМД
    arrp=ПЛК
    surname=REPIN
    name=IVAN
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl))
                     $(get_lat_code aer $(depp))
                     $(get_lat_code aer $(arrp))
                     $(flt))
$(create_spp $(ddmmyyyy +0))

<<
$(PNL_1PAX_1SEG $(get_lat_code awk $(airl))
                $(get_lat_code aer $(depp))
                $(get_lat_code aer $(arrp))
                $(flt)
                $(surname) $(name))

$(create_random_trip_comp $(get_dep_point_id $(depp) $(airl) 103 $(yymmdd +0)) Э)
$(ASSIGN_GATE $(get_dep_point_id $(depp) $(airl) 103 $(yymmdd +0)))
}) #end-of-macro PREPARE_FLIGHT_1PAX_1SEG

#########################################################################################

$(defmacro PREPARE_FLIGHT_1PAX_2SEGS_WITH_REMARKS
    airl1=ЮТ
    flt1=103
    depp1=ДМД
    arrp1=ПЛК
    airl2=СУ
    flt2=2278
    depp2=ПЛК
    arrp2=СОЧ
    surname=REPIN
    name=IVAN
    tickno=2982401841689
    cpnno=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(create_spp $(ddmmyyyy +0))

<<
$(PNL_1PAX_2SEGS_WITH_REMARKS $(airl1) $(depp1) $(arrp1) $(flt1)
                              $(airl2) $(depp2) $(arrp2) $(flt2) $(surname) $(name)
                              $(tickno) $(cpnno))

>>
$(TKCREQ_ET_DISP UTDC UTET $(last_edifact_ref) ЮТ 2982401841689)
<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(last_edifact_ref) $(airl1) $(tickno) I $(surname) $(name) $(ddmmyy) $(depp1) $(arrp1) $(flt1))

$(create_random_trip_comp $(get_dep_point_id ДМД ЮТ 103 $(yymmdd +0)) Э)
}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_1PAX_2SEGS
    airl1=ЮТ
    flt1=103
    depp1=ДМД
    arrp1=ПЛК
    airl2=С7
    flt2=1027
    depp2=ПЛК
    arrp2=СОЧ
    surname=REPIN
    name=IVAN
    tickno=2982401841689
    cpnno=1
{
$(PREPARE_SEASON_SCD $(get_lat_code awk $(airl1))
                     $(get_lat_code aer $(depp1))
                     $(get_lat_code aer $(arrp1))
                     $(flt1))
$(create_spp $(ddmmyyyy +0))

<<
$(PNL_1PAX_2SEGS $(airl1) $(depp1) $(arrp1) $(flt1)
                 $(airl2) $(depp2) $(arrp2) $(flt2) $(surname) $(name)
                 $(tickno) $(cpnno))

$(create_random_trip_comp $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)) Э)
}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_2PAXES_1SEG
    airl=ЮТ
    flt=103
    depp=ДМД
    arrp=ПЛК
    surname1=РЕПИН
    name1=ИВАН
    surname2=РЕПИНА
    name2=АННА
{

$(set airl_lat $(get_lat_code awk $(airl)))
$(set depp_lat $(get_lat_code aer $(depp)))
$(set arrp_lat $(get_lat_code aer $(arrp)))

$(PREPARE_SEASON_SCD $(get airl_lat)
                     $(get depp_lat)
                     $(get arrp_lat)
                     $(flt))
$(create_spp $(ddmmyyyy +0))

<<
$(PNL_2PAXES_1SEG $(get airl_lat)
                  $(get depp_lat)
                  $(get arrp_lat)
                  $(flt)
                  $(surname1) $(name1)
                  $(surname2) $(name2))

$(create_random_trip_comp $(get_dep_point_id $(depp) $(airl) $(flt) $(yymmdd +0)) Э)
}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_2PAXES_2SEGS_WITH_REMARKS
    airl1=ЮТ
    flt1=103
    depp1=ДМД
    arrp1=ПЛК
    airl2=С7
    flt2=1027
    depp2=ПЛК
    arrp2=СОЧ
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
$(create_spp $(ddmmyyyy +0))

<<
$(PNL_2PAXES_2SEGS_WITH_REMARKS $(airl1) $(depp1) $(arrp1) $(flt1)
                                $(airl2) $(depp2) $(arrp2) $(flt2)
                                $(surname1) $(name1) $(tickno1) $(cpnno1)
                                $(surname2) $(name2) $(tickno2) $(cpnno2))

$(create_random_trip_comp $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)) Э)

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref1) ЮТ $(tickno1))
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref0) ЮТ $(tickno2))

<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(get edi_ref1) $(airl1) $(tickno1) I $(surname1) $(name1) $(ddmmyy) $(depp1) $(arrp1) $(flt1) Y $(cpnno1))
<<
$(TKCRES_ET_DISP_1CPN UTET UTDC $(get edi_ref0) $(airl1) $(tickno2) I $(surname2) $(name2) $(ddmmyy) $(depp1) $(arrp1) $(flt1) Y $(cpnno2))

}) #end-of-macro

#########################################################################################

$(defmacro PREPARE_FLIGHT_1PAX_1INFT_1SEG
    airl1=ЮТ
    flt1=103
    depp1=ДМД
    arrp1=ПЛК
    airl2=С7
    flt2=1027
    depp2=ПЛК
    arrp2=СОЧ
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
$(create_spp $(ddmmyyyy +0))

<<
$(PNL_1PAX_1INFT_1SEG $(airl1) $(depp1) $(arrp1) $(flt1)
                      $(airl2) $(depp2) $(arrp2) $(flt2)
                      $(surname) $(name) $(tickno) $(cpnno)
                      $(inftSurname) $(inftName) $(inftTickno) $(inftCouponno))

$(create_random_trip_comp $(get_dep_point_id $(depp1) $(airl1) $(flt1) $(yymmdd +0)) Э)

$(set edi_ref1 $(last_edifact_ref 1))
$(set edi_ref0 $(last_edifact_ref 0))

>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref1) ЮТ $(tickno))
>>
$(TKCREQ_ET_DISP UTDC UTET $(get edi_ref0) ЮТ $(inftTickno))

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
      <message lexema_id='MSG.DATA_SAVED' code='0'>Данные успешно сохранены</message>
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
    airl=ЮТ
    dcs_addr=UTDC
    ets_addr=UTET
    recloc=G4LK6W
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
$(TKCRES_ET_DISP_1CPN $(ets_addr) $(dcs_addr) $(last_edifact_ref) $(airl) $(tickno) I $(surname) $(name) $(ddmmyy) DME LED)

$(KICK_IN_SILENT)

}) #end-of-macro

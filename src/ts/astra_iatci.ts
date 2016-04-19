include(ts/macro.ts)

# meta: suite iatci

$(defmacro INBOUND_PNL_LOCAL
{MOWKB1H
.MOWRMUT 020815
PNL
UT103/28APR DME PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 DME  LED
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1REPIN/IVAN
.L/0840Z6/UT
.L/09T1B3/1H
.O/S71027Y28LEDAER2315AR
-LED000K
-LED000M
-LED000U
ENDPNL}
) #end-of-macro


$(defmacro SAVE_ET_DISP point_id tick_no
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchETByTickNo>
      <point_id>$(point_id)</point_id>
      <TickNoEdit>$(tick_no)</TickNoEdit>
    </SearchETByTickNo>
  </query>
</term>}

>>
UNB+SIRE:1+ASTRA+ETICK+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
TKT+$(tick_no)"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+ETICK+ASTRA+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+REPIN+IVAN"
TAI+0162"
RCI+UA:G4LK6W:1"
MON+B:20.00:USD+T:20.00:USD"
FOP+CA:3"
PTK+++$(ddmmyy)+++:US"
ODI+DME+LED"
ORG+UT:MOW++IAH++A+US+D80D1BWO"
EQN+1:TD"
TXD+700+0.00:::US"
IFT+4:15:1+ /FC 20DEC MOW UT SGC10.00YINF UT MOW10.00YINF NUC20.00END"
IFT+4:5+00001230161213"
IFT+4:10+REFUNDABLE"
IFT+4:39+HOUSTON+UNITED AIRLINES INC"
TKT+$(tick_no):T:1:3"
CPN+1:I"
TVL+$(ddmmyy):2205+DME+LED+UT+103:Y+J"
RPI++NS"
PTS++YINF"
UNT+19+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...


!!
$(lastRedisplay)

}
) #end-of-macro


#########################################################################################


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)

# создание сезонного расписания на рейс
$(PREPARE_SEASON_SCD UT DME LED 103)

# генерация суточного плана полёта на дату
$(create_spp 28042016 ddmmyyyy)

# подаём на вход PNL по рейсу
<<
$(INBOUND_PNL_LOCAL)


$(set dep_point_id $(get_dep_point_id ДМД ЮТ 103 160428))
$(create_random_trip_comp $(get dep_point_id) Э)

# открываем регистрацию
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(last_point_id_spp)</point_id>
          <tripstages>
            <stage>
              <stage_id>10</stage_id>
              <act>27.04.2016 20:41:00</act>
            </stage>
            <stage>
              <stage_id>20</stage_id>
              <act>27.04.2016 20:41:00</act>
            </stage>
          </tripstages>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>}


>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED'...
    </command>


$(set first_point_id $(last_point_id_spp))
$(set next_point_id $(get_next_trip_point_id $(get first_point_id)))


!! capture=on
{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='trips' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <GetAdvTripList>
       <date>28.04.2016 00:00:00</date>
       <filter>
         <pr_takeoff>1</pr_takeoff>
       </filter>
       <view>
         <codes_fmt>5</codes_fmt>
       </view>
     </GetAdvTripList>
   </query>
 </term>}

>> lines=auto
    <date>28.04.2016 00:00:00</date>
    <trips>
      <trip>
        <point_id>$(get first_point_id)</point_id>
        <name>ЮТ103</name>
        <date>28</date>
        <airp>ДМД</airp>
        <name_sort_order>0</name_sort_order>
      </trip>
    </trips>


# выберем рейс
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='trips' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetTripInfo>
      <point_id>$(get first_point_id)</point_id>
      <refresh_type>0</refresh_type>
      <tripheader/>
      <tripdata/>
      <tripcounters/>
    </GetTripInfo>
  </query>
</term>}

>> lines=auto
    <data>
      <refresh_type>0</refresh_type>
      <point_id>$(get first_point_id)</point_id>
      <tripheader>
        <point_id>$(get first_point_id)</point_id>
        <flight>ЮТ103...
        <airline>ЮТ</airline>
        <aircode>298</aircode>
        <flt_no>103</flt_no>
        <suffix/>
        <airp>ДМД</airp>
        <scd_out_local>28.04.2016 10:00:00</scd_out_local>
        <scd_out>28.04.2016 10:00:00</scd_out>
        <real_out>10:00</real_out>
        <act_out/>
        <craft>ТУ5</craft>
        <bort/>
        <park/>
        <classes>...
        <route>ДМД-ПЛК</route>
        <places>ДМД-ПЛК</places>
        <trip_type>п</trip_type>
        <litera/>
        <remark/>
        <pr_tranzit>0</pr_tranzit>
        <trip>ЮТ103...
        <status>Регистрация</status>
        <stage_time>09:20</stage_time>
        <ckin_stage>20</ckin_stage>
        <tranzitable>0</tranzitable>
        <pr_tranz_reg>0</pr_tranz_reg>
        <pr_etstatus>0</pr_etstatus>
        <pr_etl_only>0</pr_etl_only>
        <pr_no_ticket_check>0</pr_no_ticket_check>
        <pr_mixed_norms>0</pr_mixed_norms>


# ищём пассажира
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchPax>
      <point_dep>$(last_point_id_spp)</point_dep>
      <pax_status>K</pax_status>
      <query>REPIN</query>
    </SearchPax>
  </query>
</term>}

>> lines=auto
    <trips>
      <trip>
        <point_id>...
        <airline>ЮТ</airline>
        <flt_no>103</flt_no>
        <scd>28.04.2016 00:00:00</scd>
        <airp_dep>ДМД</airp_dep>
        <groups>
          <pnr>
            <pnr_id>...
            <airp_arv>ПЛК</airp_arv>
            <subclass>Э</subclass>
            <class>Э</class>
            <passengers>
              <pax>
                <pax_id>...
                <surname>REPIN</surname>
                <name>IVAN</name>
                <rems/>
              </pax>
            </passengers>
            <transfer>
              <segment>
                <num>1</num>
                <airline>S7</airline>
                <flt_no>1027</flt_no>
                <local_date>28</local_date>
                <airp_dep>LED</airp_dep>
                <airp_arv>AER</airp_arv>
                <subclass>Y</subclass>
                <trfer_permit>0</trfer_permit>
              </segment>
            </transfer>
            <pnr_addrs>
              <pnr_addr>
                <airline>ЮТ</airline>
                <addr>0840Z6</addr>
              </pnr_addr>
              <pnr_addr>
                <airline>1H</airline>
                <addr>09T1B3</addr>
              </pnr_addr>
            </pnr_addrs>
          </pnr>
        </groups>
      </trip>
    </trips>
    <ckin_state>BeforeReg</ckin_state>
  </answer>


!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='cache' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <cache>
      <params>
        <code>DCS_ADDR_SET</code>
        <data_ver/>
        <interface_ver/>
      </params>
    </cache>
  </query>
</term>}


>> lines=auto
        <title>Адреса систем регистрации</title>


# смотрим сквозной маршрут
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(get first_point_id)</point_dep>
      <point_arv>$(get next_point_id)</point_arv>
      <airp_dep>ДМД</airp_dep>
      <airp_arv>ПЛК</airp_arv>
      <class>Э</class>
      <transfer>
        <segment>
            <airline>S7</airline>
            <flt_no>1027</flt_no>
            <suffix/>
            <local_date>28</local_date>
            <airp_dep>LED</airp_dep>
            <airp_arv>AER</airp_arv>
        </segment>
      </transfer>
      <passengers>
        <pax>
          <surname>REPIN</surname>
          <name>IVAN</name>
          <pers_type>ВЗ</pers_type>
          <seats>1</seats>
          <transfer>
            <segment>
              <subclass>Y</subclass>
            </segment>
          </transfer>
        </pax>
      </passengers>
    </CheckTCkinRoute>
  </query>
</term>}


>> lines=auto
        <classes>EDIFACT</classes>

!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(get first_point_id)</point_dep>
      <point_arv>$(get next_point_id)</point_arv>
      <airp_dep>ДМД</airp_dep>
      <airp_arv>ПЛК</airp_arv>
      <class>Э</class>
      <transfer>
        <segment>
            <airline>S7</airline>
            <flt_no>1027</flt_no>
            <suffix/>
            <local_date>28</local_date>
            <airp_dep>LED</airp_dep>
            <airp_arv>AER</airp_arv>
            <calc_status>CHECKIN</calc_status>
            <conf_status>1</conf_status>
        </segment>
      </transfer>
      <passengers>
        <pax>
          <surname>REPIN</surname>
          <name>IVAN</name>
          <pers_type>ВЗ</pers_type>
          <seats>1</seats>
          <transfer>
            <segment>
              <subclass>Y</subclass>
            </segment>
          </transfer>
        </pax>
      </passengers>
    </CheckTCkinRoute>
  </query>
</term>}


>> lines=auto
    <tckin_segments>
      <tckin_segment>
        <trfer_permit value='0'/>
        <tripheader>
          <flight>С71027/28 ПЛК (EDI)</flight>
          <airline>С7</airline>
          <aircode>421</aircode>
          <flt_no>1027</flt_no>
          <suffix/>
          <airp>ПЛК</airp>
          <scd_out_local>28.04.2016 03:00:00</scd_out_local>
          <pr_etl_only>1</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>1</pr_no_ticket_check>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <check_info>
                <pass/>
                <crew/>
              </check_info>
              <target_view>СОЧИ (СОЧ)</target_view>
              <airp_code>СОЧ</airp_code>
              <city_code>СОЧ</city_code>
            </airp>
          </airps>
          <classes>
            <class>
              <code>Э</code>
              <class_view>ЭКОНОМ</class_view>
              <cfg>-1</cfg>
            </class>
          </classes>
          <gates/>
          <halls/>
        </tripdata>
        <point_dep>-1</point_dep>
        <airp_dep>ПЛК</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>СОЧ</airp_arv>
        <city_arv_code>СОЧ</city_arv_code>
        <tckin_passengers>
          <tckin_pax>
            <trips>
              <trip>
                <airline>С7</airline>
                <point_id>-1</point_id>
                <airp_dep>ПЛК</airp_dep>
                <flt_no>1027</flt_no>
                <scd>28.04.2016 00:00:00</scd>
                <groups>
                  <pnr>
                    <pnr_id>-1</pnr_id>
                    <airp_arv>СОЧ</airp_arv>
                    <subclass>Э</subclass>
                    <class>Э</class>
                    <passengers>
                      <pax>
                        <pax_id>-1</pax_id>
                        <surname>REPIN</surname>
                        <name>IVAN</name>
                        <seat_no/>
                        <document/>
                        <ticket_no/>
                        <ticket_rem/>
                        <rems/>
                      </pax>
                    </passengers>
                    <transfer/>
                    <pnr_addrs/>
                  </pnr>
                </groups>
              </trip>
            </trips>
          </tckin_pax>
        </tckin_passengers>
        <class_code>Э</class_code>
      </tckin_segment>
    </tckin_segments>
  </answer>
</term>


$(SAVE_ET_DISP $(last_point_id_spp) 2986120030297)


$(set pax_id $(get_single_pax_id $(get first_point_id) REPIN IVAN K))

!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(get first_point_id)</point_dep>
          <point_arv>$(get next_point_id)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>ЮТ</airline>
            <flt_no>454</flt_no>
            <suffix/>
            <scd>28.04.2016 00:00:00</scd>
            <airp_dep>ДМД</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(get pax_id)</pax_id>
              <surname>REPIN</surname>
              <name>IVAN</name>
              <pers_type>ВЗ</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>2986120030297</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>REPIN</surname>
                <first_name>IVAN</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <transfer/>
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774441110</rem_text>
                </rem>
              </rems>
              <norms/>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>ПЛК</airp_dep>
          <airp_arv>СОЧ</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>С7</airline>
            <flt_no>1027</flt_no>
            <suffix/>
            <scd>28.04.2016 00:00:00</scd>
            <airp_dep>ПЛК</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id/>
              <surname>REPIN</surname>
              <name>IVAN</name>
              <pers_type>ВЗ</pers_type>
              <seat_no>7A</seat_no>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no/>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>0</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <rems/>
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


>>
UNB+SIRE:1+ASTRA+ETICK+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+280416+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# ответ от СЭБ
<<
UNB+SIRE:1+ETICK+ASTRA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+2986120030297:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=4
    <kick...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+ЮТ:ДМД"
FDQ+С7+1027+160428+ПЛК+СОЧ++ЮТ+454+160428++ДМД+ПЛК"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+1604281000+ПЛК+СОЧ++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:Э"
PSI++TKNE::29861200302972"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=4
    <kick...

$(lastRedisplay)

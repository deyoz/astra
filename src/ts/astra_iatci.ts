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


$(defmacro SAVE_ET_DISP
point_id
tick_no
surname=REPIN
name=IVAN
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
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
TKT+$(tick_no)"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:131+3"
TIF+$(surname)+$(name)"
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

$(KICK_IN)

}
) #end-of-macro


$(defmacro CHECK_SEARCH_PAX
    point_dep
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    status
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchPax>
      <point_dep>$(point_dep)</point_dep>
      <pax_status>$(status)</pax_status>
      <query>$(surname)</query>
    </SearchPax>
  </query>
</term>}

>> lines=auto
    <trips>
      <trip>
        <point_id>...
        <airline>$(airl)</airline>
        <flt_no>$(flt)</flt_no>
        <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
        <airp_dep>$(airp_dep)</airp_dep>
        <groups>
          <pnr>
            <pnr_id>...
            <airp_arv>$(airp_arv)</airp_arv>
            <subclass>Э</subclass>
            <class>Э</class>
            <passengers>
              <pax>
                <pax_id>...
                <surname>$(surname)</surname>
                <name>$(name)</name>


}) #end-of-macro


$(defmacro CHECK_TCKIN_ROUTE_1
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    pers_type
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(point_dep)</point_dep>
      <point_arv>$(point_arv)</point_arv>
      <airp_dep>ДМД</airp_dep>
      <airp_arv>ПЛК</airp_arv>
      <class>Э</class>
      <transfer>
        <segment>
            <airline>$(get_lat_code awk $(airl))</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <local_date>$(dd +0 en)</local_date>
            <airp_dep>$(get_lat_code aer $(airp_dep))</airp_dep>
            <airp_arv>$(get_lat_code aer $(airp_arv))</airp_arv>
        </segment>
      </transfer>
      <passengers>
        <pax>
          <surname>$(surname)</surname>
          <name>$(name)</name>
          <pers_type>$(pers_type)</pers_type>
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

}) #end-of-macro


$(defmacro CHECK_TCKIN_ROUTE_2
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname
    name
    pers_type
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(point_dep)</point_dep>
      <point_arv>$(point_arv)</point_arv>
      <airp_dep>ДМД</airp_dep>
      <airp_arv>ПЛК</airp_arv>
      <class>Э</class>
      <transfer>
        <segment>
            <airline>$(get_lat_code awk $(airl))</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <local_date>$(dd +0 en)</local_date>
            <airp_dep>$(get_lat_code aer $(airp_dep))</airp_dep>
            <airp_arv>$(get_lat_code aer $(airp_arv))</airp_arv>
            <calc_status>CHECKIN</calc_status>
            <conf_status>1</conf_status>
        </segment>
      </transfer>
      <passengers>
        <pax>
          <surname>$(surname)</surname>
          <name>$(name)</name>
          <pers_type>$(pers_type)</pers_type>
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
          <flight>$(airl)$(flt) $(airp_dep) (EDI)</flight>
          <airline>$(airl)</airline>
          <aircode>...
          <flt_no>$(flt)</flt_no>
          <suffix/>
          <airp>$(airp_dep)</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 03:00:00</scd_out_local>
          <pr_etl_only>1</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>1</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <check_info>
                <pass/>
                <crew/>
              </check_info>
              <target_view>...
              <airp_code>$(airp_arv)</airp_code>
              <city_code>...
            </airp>
          </airps>
          <classes/>
          <gates/>
          <halls/>
        </tripdata>
        <point_dep>-1</point_dep>
        <airp_dep>$(airp_dep)</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>$(airp_arv)</airp_arv>
        <city_arv_code>...
        <tckin_passengers>
          <tckin_pax>
            <trips>
              <trip>
                <airline>$(airl)</airline>
                <point_id>-1</point_id>
                <airp_dep>$(airp_dep)</airp_dep>
                <flt_no>$(flt)</flt_no>
                <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
                <groups>
                  <pnr>
                    <pnr_id>-1</pnr_id>
                    <airp_arv>$(airp_arv)</airp_arv>
                    <subclass>Э</subclass>
                    <class>Э</class>
                    <passengers>
                      <pax>
                        <pax_id>-1</pax_id>
                        <surname>$(surname)</surname>
                        <name>$(name)</name>
                        <seats>1</seats>
                        <pers_type>$(pers_type)</pers_type>
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

}) #end-of-macro


$(defmacro CHECK_TCKIN_ROUTE_GRP_1
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname1
    name1
    pers_type1
    surname2
    name2
    pers_type2
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(point_dep)</point_dep>
      <point_arv>$(point_arv)</point_arv>
      <airp_dep>ДМД</airp_dep>
      <airp_arv>ПЛК</airp_arv>
      <class>Э</class>
      <transfer>
        <segment>
            <airline>$(get_lat_code awk $(airl))</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <local_date>$(dd +0 en)</local_date>
            <airp_dep>$(get_lat_code aer $(airp_dep))</airp_dep>
            <airp_arv>$(get_lat_code aer $(airp_arv))</airp_arv>
        </segment>
      </transfer>
      <passengers>
        <pax>
          <surname>$(surname1)</surname>
          <name>$(name1)</name>
          <pers_type>$(pers_type1)</pers_type>
          <seats>1</seats>
          <transfer>
            <segment>
              <subclass>Y</subclass>
            </segment>
          </transfer>
        </pax>
        <pax>
          <surname>$(surname2)</surname>
          <name>$(name2)</name>
          <pers_type>$(pers_type2)</pers_type>
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

}) #end-of-macro


$(defmacro CHECK_TCKIN_ROUTE_GRP_2
    point_dep
    point_arv
    airl
    flt
    airp_dep
    airp_arv
    surname1
    name1
    pers_type1
    surname2
    name2
    pers_type2
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(point_dep)</point_dep>
      <point_arv>$(point_arv)</point_arv>
      <airp_dep>ДМД</airp_dep>
      <airp_arv>ПЛК</airp_arv>
      <class>Э</class>
      <transfer>
        <segment>
            <airline>$(get_lat_code awk $(airl))</airline>
            <flt_no>$(flt)</flt_no>
            <suffix/>
            <local_date>$(dd +0 en)</local_date>
            <airp_dep>$(get_lat_code aer $(airp_dep))</airp_dep>
            <airp_arv>$(get_lat_code aer $(airp_arv))</airp_arv>
            <calc_status>CHECKIN</calc_status>
            <conf_status>1</conf_status>
        </segment>
      </transfer>
      <passengers>
        <pax>
          <surname>$(surname1)</surname>
          <name>$(name1)</name>
          <pers_type>$(pers_type1)</pers_type>
          <seats>1</seats>
          <transfer>
            <segment>
              <subclass>Y</subclass>
            </segment>
          </transfer>
        </pax>
        <pax>
          <surname>$(surname2)</surname>
          <name>$(name2)</name>
          <pers_type>$(pers_type2)</pers_type>
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
          <flight>$(airl)$(flt) $(airp_dep) (EDI)</flight>
          <airline>$(airl)</airline>
          <aircode>...
          <flt_no>$(flt)</flt_no>
          <suffix/>
          <airp>$(airp_dep)</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 03:00:00</scd_out_local>
          <pr_etl_only>1</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>1</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <check_info>
                <pass/>
                <crew/>
              </check_info>
              <target_view>...
              <airp_code>$(airp_arv)</airp_code>
              <city_code>...
            </airp>
          </airps>
          <classes/>
          <gates/>
          <halls/>
        </tripdata>
        <point_dep>-1</point_dep>
        <airp_dep>$(airp_dep)</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>$(airp_arv)</airp_arv>
        <city_arv_code>...
        <tckin_passengers>
          <tckin_pax>
            <trips>
              <trip>
                <airline>$(airl)</airline>
                <point_id>-1</point_id>
                <airp_dep>$(airp_dep)</airp_dep>
                <flt_no>$(flt)</flt_no>
                <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
                <groups>
                  <pnr>
                    <pnr_id>-1</pnr_id>
                    <airp_arv>$(airp_arv)</airp_arv>
                    <subclass>Э</subclass>
                    <class>Э</class>
                    <passengers>
                      <pax>
                        <pax_id>-1</pax_id>
                        <surname>$(surname1)</surname>
                        <name>$(name1)</name>
                        <seats>1</seats>
                        <pers_type>$(pers_type1)</pers_type>
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
          <tckin_pax>
            <trips>
              <trip>
                <airline>$(airl)</airline>
                <point_id>-1</point_id>
                <airp_dep>$(airp_dep)</airp_dep>
                <flt_no>$(flt)</flt_no>
                <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
                <groups>
                  <pnr>
                    <pnr_id>-1</pnr_id>
                    <airp_arv>$(airp_arv)</airp_arv>
                    <subclass>Э</subclass>
                    <class>Э</class>
                    <passengers>
                      <pax>
                        <pax_id>-2</pax_id>
                        <surname>$(surname2)</surname>
                        <name>$(name2)</name>
                        <seats>1</seats>
                        <pers_type>$(pers_type2)</pers_type>
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

}) #end-of-macro



$(defmacro SAVE_PAX
    pax_id
    point_dep1
    point_arv1
    airl1
    flt1
    airp_dep1
    airp_arv1
    airl2
    flt2
    airp_dep2
    airp_arv2
    surname
    name
    tickno
    pers_type
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep1)</point_dep>
          <point_arv>$(point_arv1)</point_arv>
          <airp_dep>$(airp_dep1)</airp_dep>
          <airp_arv>$(airp_arv1)</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl1)</airline>
            <flt_no>$(flt1)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep1)</airp_dep>
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
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
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
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl2)</airline>
            <flt_no>$(flt2)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep2)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id/>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>$(pers_type)</pers_type>
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
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774441110</rem_text>
                </rem>
              </rems>
              <fqt_rems>
                <fqt_rem>
                  <rem_code>FQTV</rem_code>
                  <airline>$(airl2)</airline>
                  <no>7788990011</no>
                </fqt_rem>
              </fqt_rems>
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

}) #end-of-macro


$(defmacro SAVE_GRP
    pax_id1
    pax_id2
    point_dep1
    point_arv1
    airl1
    flt1
    airp_dep1
    airp_arv1
    airl2
    flt2
    airp_dep2
    airp_arv2
    surname1
    name1
    tickno1
    pers_type1
    surname2
    name2
    tickno2
    pers_type2
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer>
        <segment>
          <airline>$(airl2)</airline>
          <flt_no>$(flt2)</flt_no>
          <suffix/>
          <local_date>$(dd +0 en)</local_date>
          <airp_dep>$(get_lat_code aer $(airp_dep2))</airp_dep>
          <airp_arv>$(get_lat_code aer $(airp_arv2))</airp_arv>
        </segment>
      </transfer>
      <segments>
        <segment>
          <point_dep>$(point_dep1)</point_dep>
          <point_arv>$(point_arv1)</point_arv>
          <airp_dep>$(airp_dep1)</airp_dep>
          <airp_arv>$(airp_arv1)</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl1)</airline>
            <flt_no>$(flt1)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep1)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id>$(pax_id1)</pax_id>
              <surname>$(surname1)</surname>
              <name>$(name1)</name>
              <pers_type>$(pers_type1)</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>$(tickno1)</ticket_no>
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
                <surname>$(surname1)</surname>
                <first_name>$(name1)</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>Э</subclass>              
              <bag_pool_num/>              
              <transfer>
                <segment>
                  <subclass>L</subclass>
                </segment>
              </transfer>
              <rems>
                <rem>
                  <rem_code>DOCS</rem_code>
                  <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
                </rem>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PPZB400522509</rem_text>
                </rem>
                <rem>
                  <rem_code>PSPT</rem_code>
                  <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
                </rem>
                <rem>
                  <rem_code>TKNE</rem_code>
                  <rem_text>TKNE HK1 2982401841689/1</rem_text>
                </rem>
                <rem>
                  <rem_code>INFT</rem_code>
                  <rem_text>INFT HK1 01JAN17 $(surname2)/$(name2)</rem_text>
                </rem>
              </rems>
              <norms/>
            </pax>
            <pax>
              <pax_id>$(pax_id2)</pax_id>
              <surname>$(surname2)</surname>
              <name>$(name2)</name>
              <pers_type>$(pers_type2)</pers_type>
              <seat_no/>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no>$(tickno2)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441112</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>$(surname2)</surname>
                <first_name>$(name2)</first_name>
              </document>
              <doco/>
              <addresses/>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <transfer>
                <segment>
                  <subclass>L</subclass>
                </segment>
              </transfer>
              <rems>
                <rem>
                  <rem_code>DOCS</rem_code>
                  <rem_text>DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR</rem_text>
                </rem>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PPZB400522510</rem_text>
                </rem>
                <rem>
                  <rem_code>PSPT</rem_code>
                  <rem_text>PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M</rem_text>
                </rem>
                <rem>
                  <rem_code>TKNE</rem_code>
                  <rem_text>TKNE HK1 2982401841612/1</rem_text>
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
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <status>K</status>
          <wl_type/>
          <mark_flight>
            <airline>$(airl2)</airline>
            <flt_no>$(flt2)</flt_no>
            <suffix/>
            <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
            <airp_dep>$(airp_dep2)</airp_dep>
            <pr_mark_norms>0</pr_mark_norms>
          </mark_flight>
          <passengers>
            <pax>
              <pax_id/>
              <surname>$(surname1)</surname>
              <name>$(name1)</name>
              <pers_type>$(pers_type1)</pers_type>
              <seat_no>7A</seat_no>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no/>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>0</ticket_confirm>
              <document/>
              <doco>
                <birth_place/>
                <type>V</type>
                <no>4538926</no>
                <issue_place>MOSCOW</issue_place>
                <issue_date>09.01.2011 00:00:00</issue_date>
                <expiry_date>09.01.2021 00:00:00</expiry_date>
                <applic_country>UKR</applic_country>
              </doco>
              <addresses/>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <rems/>
            </pax>
            <pax>
              <pax_id/>
              <surname>$(surname2)</surname>
              <name>$(name2)</name>
              <pers_type>$(pers_type2)</pers_type>
              <seat_no>8A</seat_no>
              <preseat_no/>
              <seat_type/>
              <seats>1</seats>
              <ticket_no/>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>0</ticket_confirm>
              <document/>
              <doco>
                <birth_place>MADRID</birth_place>
                <type>VI</type>
                <no>13452</no>
                <issue_place>MINSK</issue_place>
                <issue_date>20.03.2009 00:00:00</issue_date>
                <expiry_date>20.03.2019 00:00:00</expiry_date>
                <applic_country>ESP</applic_country>
              </doco>
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

}) #end-of-macro

$(defmacro SAVE_GRP_BAGGAGE
    grp_id
    tid
    point_dep1
    point_arv1
    airp_dep1
    airp_arv1
    airp_dep2
    airp_arv2
    pax_id1
    tid1
    surname1
    name1
    pers_type1
    tickno1
    doc_type1
    doc_no1
    doc_nationality1
    doc_birthdate1
    doc_expirydate1
    doc_surname1
    doc_name1
    doc_secname1
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(point_dep1)</point_dep>
          <point_arv>$(point_arv1)</point_arv>
          <airp_dep>$(airp_dep1)</airp_dep>
          <airp_arv>$(airp_arv1)</airp_arv>
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id1)</pax_id>
              <surname>$(surname1)</surname>
              <name>$(name1)</name>
              <pers_type>$(pers_type1)</pers_type>
              <refuse/>
              <ticket_no>$(tickno1)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>$(doc_type1)</type>
                <no>$(doc_no1)</no>
                <nationality>$(doc_nationality1)</nationality>
                <birth_date>$(doc_birthdate1)</birth_date>
                <expiry_date>$(doc_expirydate1)</expiry_date>
                <surname>$(doc_surname1)</surname>
                <first_name>$(doc_name1)</first_name>
                <second_name>$(doc_secname1)</second_name>
              </document>
              <doco/>
              <addresses>
                <doca>
                  <type>B</type>
                </doca>
                <doca>
                  <type>R</type>
                </doca>
                <doca>
                  <type>D</type>
                </doca>
              </addresses>
              <subclass>Э</subclass>
              <bag_pool_num>1</bag_pool_num>
              <subclass>Э</subclass>
              <tid>$(tid1)</tid>
            </pax>
          </passengers>
          <service_payment/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname1)</surname>
              <name>$(name1)</name>
              <pers_type>$(pers_type1)</pers_type>
              <refuse/>
              <ticket_no>$(tickno1)</ticket_no>
              <coupon_no>2</coupon_no>
              <ticket_rem/>
              <ticket_confirm>0</ticket_confirm>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <bag_pool_num>1</bag_pool_num>
              <subclass>Э</subclass>
              <tid>0</tid>
            </pax>
          </passengers>
          <service_payment/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
      <value_bags/>
      <bags>
        <bag>
          <bag_type/>
          <airline>S7</airline>
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
          <airline>S7</airline>
          <num>2</num>
          <pr_cabin>0</pr_cabin>
          <amount>1</amount>
          <weight>12</weight>
          <value_bag_num/>
          <pr_liab_limit>0</pr_liab_limit>
          <to_ramp>0</to_ramp>
          <using_scales>0</using_scales>
          <is_trfer>0</is_trfer>
          <bag_pool_num>1</bag_pool_num>
        </bag>
        <bag>
          <bag_type/>
          <airline>S7</airline>
          <num>3</num>
          <pr_cabin>1</pr_cabin>
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
      <tags pr_print='1'/>
      <unaccomps/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro


$(defmacro REMOVE_ONE_BAG
    grp_id
    tid
    point_dep1
    point_arv1
    airp_dep1
    airp_arv1
    airp_dep2
    airp_arv2
    pax_id1
    tid1
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(point_dep1)</point_dep>
          <point_arv>$(point_arv1)</point_arv>
          <airp_dep>$(airp_dep1)</airp_dep>
          <airp_arv>$(airp_arv1)</airp_arv>
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
      <value_bags/>
      <bags>
        <bag>
          <bag_type/>
          <airline>S7</airline>
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
          <airline>S7</airline>
          <num>2</num>
          <pr_cabin>1</pr_cabin>
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
      <tags pr_print='1'/>
      <unaccomps/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro


$(defmacro REMOVE_ALL_BAGS
    grp_id
    tid
    point_dep1
    point_arv1
    airp_dep1
    airp_arv1
    airp_dep2
    airp_arv2
    pax_id1
    tid1
    surname1
    name1
    pers_type1
    tickno1
    doc_type1
    doc_no1
    doc_nationality1
    doc_birthdate1
    doc_expirydate1
    doc_surname1
    doc_name1
    doc_secname1
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(point_dep1)</point_dep>
          <point_arv>$(point_arv1)</point_arv>
          <airp_dep>$(airp_dep1)</airp_dep>
          <airp_arv>$(airp_arv1)</airp_arv>
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id1)</pax_id>
              <surname>$(surname1)</surname>
              <name>$(name1)</name>
              <pers_type>$(pers_type1)</pers_type>
              <refuse/>
              <ticket_no>$(tickno1)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>0</ticket_confirm>
              <document>
                <type>$(doc_type1)</type>
                <no>$(doc_no1)</no>
                <nationality>$(doc_nationality1)</nationality>
                <birth_date>$(doc_birthdate1)</birth_date>
                <expiry_date>$(doc_expirydate1)</expiry_date>
                <surname>$(doc_surname1)</surname>
                <first_name>$(doc_name1)</first_name>
                <second_name>$(doc_secname1)</second_name>
              </document>
              <doco/>
              <addresses>
                <doca>
                  <type>B</type>
                </doca>
                <doca>
                  <type>R</type>
                </doca>
                <doca>
                  <type>D</type>
                </doca>
              </addresses>
              <subclass>Э</subclass>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>$(tid1)</tid>
            </pax>
          </passengers>
          <service_payment/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname1)</surname>
              <name>$(name1)</name>
              <pers_type>$(pers_type1)</pers_type>
              <refuse/>
              <ticket_no>$(tickno1)</ticket_no>
              <coupon_no>2</coupon_no>
              <ticket_rem/>
              <ticket_confirm>0</ticket_confirm>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
            </pax>
          </passengers>
          <service_payment/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
      <value_bags/>
      <bags/>
      <tags pr_print='1'/>
      <unaccomps/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro


$(defmacro CANCEL_PAX
    pax_id
    grp_id
    tid
    point_dep1
    point_arv1
    airl1
    flt1
    airp_dep1
    airp_arv1
    airl2
    flt2
    airp_dep2
    airp_arv2
    surname
    name
    tickno
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(point_dep1)</point_dep>
          <point_arv>$(point_arv1)</point_arv>
          <airp_dep>$(airp_dep1)</airp_dep>
          <airp_arv>$(airp_arv1)</airp_arv>
          <class>Э</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>ВЗ</pers_type>
              <refuse>А</refuse>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>$(tid)</tid>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>ВЗ</pers_type>
              <refuse>А</refuse>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>1</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
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

}) #end-of-macro


$(defmacro UPDATE_PAX_DOC
    point_dep
    point_arv
    airp_dep
    airp_arv
    grp_id
    pax_id
    tid
    airp_dep2
    airp_arv2
    surname
    name
    second_name
    tickno
    cpnno
{
!!
{<?xml version='1.0' encoding='CP866'?>
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
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>ВЗ</pers_type>
              <refuse/>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>$(cpnno)</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                  <type>P</type>
                  <no>7878787899</no>
                  <nationality>RUS</nationality>
                  <birth_date>01.05.1975 00:00:00</birth_date>
                  <expiry_date>31.12.2049 00:00:00</expiry_date>
                  <surname>$(surname)</surname>
                  <first_name>$(name)</first_name>
                  <second_name>$(second_name)</second_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
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

}) #end-of-macro


$(defmacro REMOVE_PAX_DOC
    point_dep
    point_arv
    airp_dep
    airp_arv
    grp_id
    pax_id
    tid
    airp_dep2
    airp_arv2
    surname
    name
    second_name
    tickno
    cpnno
{
!!
{<?xml version='1.0' encoding='CP866'?>
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
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>ВЗ</pers_type>
              <refuse/>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>$(cpnno)</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
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

}) #end-of-macro

$(defmacro LOAD_PAX_BY_GRP_ID
    point_dep
    grp_id
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinLoadPax>
      <point_id>$(point_dep)</point_id>
      <grp_id>$(grp_id)</grp_id>
    </TCkinLoadPax>
  </query>
</term>}

}) #end-of-macro


$(defmacro LOAD_PAX_BY_REG_NO
    point_dep
    reg_no
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinLoadPax>
      <point_id>$(point_dep)</point_id>
      <reg_no>$(reg_no)</reg_no>
    </TCkinLoadPax>
  </query>
</term>}

}) #end-of-macro


$(defmacro LOAD_PAX_BY_PAX_ID
    point_dep
    pax_id
{
!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinLoadPax>
      <point_id>$(point_dep)</point_id>
      <pax_id>$(pax_id)</pax_id>
    </TCkinLoadPax>
  </query>
</term>}

}) #end-of-macro


$(defmacro ETS_COS_EXCHANGE
    tickno
    cpnno
    status
    pult=МОВРОМ
{

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+$(pult)"
EQN+1:TD"
TKT+$(tickno):T"
CPN+$(cpnno):$(status)"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# ответ от СЭБ
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+$(tickno):T::3"
CPN+$(cpnno):$(status)::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

}) #end-of-macro


$(defmacro ETS_COS_EXCHANGE2
    tickno1
    cpnno1
    tickno2
    cpnno2
    status
    pult=МОВРОМ
{

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref 1)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref 1)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+$(pult)"
EQN+1:TD"
TKT+$(tickno2):T"
CPN+$(cpnno2):$(status)"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref 1)0001"

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+$(pult)"
EQN+1:TD"
TKT+$(tickno1):T"
CPN+$(cpnno1):$(status)"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# ответы от СЭБ
<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+$(tickno1):T::3"
CPN+$(cpnno1):$(status)::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:96:2:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+1:TD"
TKT+$(tickno2):T::3"
CPN+$(cpnno2):$(status)::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

}) #end-of-macro



$(defmacro UPDATE_PAX_DOC_NON_IATCI
    point_dep
    point_arv
    airp_dep
    airp_arv
    grp_id
    pax_id
    tid
    airp_dep2
    airp_arv2
    surname
    name
    tickno
    cpnno
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
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
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>ВЗ</pers_type>
              <refuse/>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>$(cpnno)</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <no>123424124</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <expiry_date>31.12.2049 00:00:00</expiry_date>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>$(tid)</tid>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
    </TCkinSavePax>
  </query>
</term>}

}) #end-of-macro


$(defmacro UPDATE_PAX_REMS
    point_dep
    point_arv
    airp_dep
    airp_arv
    grp_id
    pax_id
    tid
    airp_dep2
    airp_arv2
    surname
    name
    second_name
    tickno
    cpnno
{
!!
{<?xml version='1.0' encoding='CP866'?>
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
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <tid>0</tid>
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774449999</rem_text>
                </rem>
                <rem>
                  <rem_code>OTHS</rem_code>
                  <rem_text>OTHS HK1 DOCS/7777771110/PS</rem_text>
                </rem>
              </rems>
              <fqt_rems>
                <fqt_rem>
                  <rem_code>FQTV</rem_code>
                  <airline>UT</airline>
                  <no>7788990011</no>
                </fqt_rem>
              </fqt_rems>
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

}) #end-of-macro

$(defmacro CANCEL_PAX_REMS
    point_dep
    point_arv
    airp_dep
    airp_arv
    grp_id
    pax_id
    tid
    airp_dep2
    airp_arv2
    surname
    name
    second_name
    tickno
    cpnno
{
!!
{<?xml version='1.0' encoding='CP866'?>
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
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <tid>0</tid>
              <rems/>
              <fqt_rems/>
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

}) #end-of-macro


$(defmacro UPDATE_PAX_REMS_WITH_LONG
    point_dep
    point_arv
    airp_dep
    airp_arv
    grp_id
    pax_id
    tid
    airp_dep2
    airp_arv2
    surname
    name
    second_name
    tickno
    cpnno
{
!! err=MSG.TOO_LONG_SSR_FREE_TEXT
{<?xml version='1.0' encoding='CP866'?>
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
          <class>Э</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>$(airp_dep2)</airp_dep>
          <airp_arv>$(airp_arv2)</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <tid>0</tid>
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774449999</rem_text>
                </rem>
                <rem>
                  <rem_code>OTHS</rem_code>
                  <rem_text>OTHS VERY VERY LONG                                                 TEXT</rem_text>
                </rem>
              </rems>
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

}) #end-of-macro

#########################################################################################
# №1 первичная регистрация
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN IVAN К)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN РБ)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN РБ)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 РБ)
$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+C:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
FSD+1155++GATE1"
PPD+REPIN+C:N+0013929620+IVAN"
PFD+xx+:Y+1"
PSI++TKNE::29861200302972"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>ЮТ103 ДМД</flight>
          <flight_short>ЮТ103...
          <airline>ЮТ</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>ДМД</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:15:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>$(get point_arv)</point_id>
              <airp_code>ПЛК</airp_code>
              <city_code>СПТ</city_code>
              <target_view>САНКТ-ПЕТЕРБУРГ (ПЛК)</target_view>
              <check_info>
                <pass>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </pass>
                <crew>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </crew>
              </check_info>
            </airp>
          </airps>
          <classes>
            <class>
              <code>Э</code>
              <class_view>ЭКОНОМ</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>1</id>
              <name>Зал 1</name>
            </hall>
            <hall>
              <id>0</id>
              <name>Др.</name>
            </hall>
            <hall>
              <id>1141</id>
              <name>VIP</name>
            </hall>
            <hall>
              <id>1439</id>
              <name>Пав. вокз.</name>
            </hall>
            <hall>
              <id>39706</id>
              <name>супер зал</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>ЮТ</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y +0) 10:15:00</scd>
              <airp_dep>ДМД</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>...
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>ДМД</airp_dep>
        <point_arv>$(get point_arv)</point_arv>
        <airp_arv>ПЛК</airp_arv>
        <class>Э</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>...
        <show_ticket_norms>0</show_ticket_norms>
        <show_wt_norms>1</show_wt_norms>
        <city_arv>СПТ</city_arv>
        <mark_flight>
          <airline>ЮТ</airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
          <airp_dep>ДМД</airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>$(get pax_id)</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>РБ</pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>Э</subclass>
            <bag_pool_num/>
            <tid>...
            <ticket_no>2986120030297</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
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
            <ticket_bag_norm>нет</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <service_lists>
              <service_list seg_no='0' category='1'...
              <service_list seg_no='0' category='2'...
              <service_list seg_no='1' category='1'...
              <service_list seg_no='1' category='2'...
            </service_lists>
            <rems>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PP7774441110</rem_text>
              </rem>
            </rems>
            <asvc_rems/>
            <fqt_rems/>
            <norms/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters>
          <item>
            <point_arv>$(get point_arv)</point_arv>
            <class>Э</class>
            <noshow>...
            <trnoshow>...
            <show>...
            <free_ok>...
            <free_goshow>...
            <nooccupy>...
          </item>
        </tripcounters>
        <load_residue/>
      </segment>
      <segment>
        <tripheader>
          <flight>С71027/$(date_format %d.%m +0) ПЛК (EDI)</flight>
          <airline>С7</airline>
          <aircode>421</aircode>
          <flt_no>1027</flt_no>
          <suffix/>
          <airp>ПЛК</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:00:00</scd_out_local>
          <scd_brd_to_local>11:55</scd_brd_to_local>
          <remote_gate>GATE1</remote_gate>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <airp_code>СОЧ</airp_code>
              <city_code>СОЧ</city_code>
              <target_view>СОЧИ (СОЧ)</target_view>
              <check_info>
                <pass/>
                <crew/>
              </check_info>
            </airp>
          </airps>
          <classes/>
          <gates/>
          <halls/>
          <mark_flights/>
        </tripdata>
        <grp_id>-1</grp_id>
        <point_dep>-...
        <airp_dep>ПЛК</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>СОЧ</airp_arv>
        <class/>
        <status>K</status>
        <bag_refuse/>
        <piece_concept>0</piece_concept>
        <tid>0</tid>
        <city_arv>СОЧ</city_arv>
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>РБ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>xx</seat_no>
            <reg_no>1</reg_no>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2986120030297</ticket_no>
            <coupon_no>2</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems/>
            <fqt_rems/>
            <document/>
            <addresses/>
            <doco/>
            <iatci_pax_id>0013929620</iatci_pax_id>
            <iatci_parent_pax_id/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters/>
        <load_residue/>
      </segment>
    </segments>


%%
#########################################################################################
# №2 Отмена регистрации
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
PPD+REPIN+A+0013929620+IVAN"
PFD+xx+:Э"
PSI++TKNE::29861200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))


$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
             ЮТ 103 ДМД ПЛК
             С7 1027 ПЛК СОЧ
             REPIN IVAN 2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 I)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A:N+0013929620+IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+X+P"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=4
    <segments/>


%%
#########################################################################################
# №3 Просмотр информации по пассажиру по grp_id
###


%%
#########################################################################################
# №4 Просмотр информации по пассажиру по reg_no
###


%%
#########################################################################################
# №5 Просмотр информации по пассажиру по pax_id
###



%%
#########################################################################################
# №6 Таймаут на PLF
###


%%
#########################################################################################
# №7 Ошибка на PLF
###


%%
#########################################################################################
# №8 Ошибка на CKI
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+X"
WAD+1:194"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...

!! err=$(utf8 "Пассажир не зарегистрирован")
$(lastRedisplay)

$(ETS_COS_EXCHANGE 2986120030297 1 I SYSTEM)

# ну а дальше ничего и не должно происходить по идее.


%%
#########################################################################################
# №9 Ошибка на CKX
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA IFM1 IFM2)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PFD+xx+:Э"
PSI++TKNE::29861200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))


$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
             ЮТ 103 ДМД ПЛК
             С7 1027 ПЛК СОЧ
             REPIN IVAN 2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 I)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+X+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...


%%
#########################################################################################
# №10 Обновление данных регистрации (Документ)
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+O"
FSD+1155"
PPD+REPIN+A:N++IVAN"
PFD+xx+:Э+22"
PSI++TKNE::29861200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(UPDATE_PAX_DOC $(get point_dep) $(get point_arv) ДМД ПЛК
                 $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                 REPIN IVAN IVANICH 2986120030297 2)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
UAP+R+A:REPIN:IVAN:750501:::RUS++P:7878787899::::491231:::::::REPIN:IVAN:IVANICH"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>ЮТ103 ДМД</flight>
          <flight_short>ЮТ103...
          <airline>ЮТ</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>ДМД</airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:15:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>...
              <airp_code>ПЛК</airp_code>
              <city_code>СПТ</city_code>
              <target_view>САНКТ-ПЕТЕРБУРГ (ПЛК)</target_view>
              <check_info>
                <pass>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </pass>
                <crew>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </crew>
              </check_info>
            </airp>
          </airps>
          <classes>
            <class>
              <code>Э</code>
              <class_view>ЭКОНОМ</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>...
              <name>Зал 1</name>
            </hall>
            <hall>
              <id>...
              <name>Др.</name>
            </hall>
            <hall>
              <id>...
              <name>VIP</name>
            </hall>
            <hall>
              <id>...
              <name>Пав. вокз.</name>
            </hall>
            <hall>
              <id>...
              <name>супер зал</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>ЮТ</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y) 10:15:00</scd>
              <airp_dep>ДМД</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>$(get grp_id)</grp_id>
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>ДМД</airp_dep>
        <point_arv>...
        <airp_arv>ПЛК</airp_arv>
        <class>Э</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>...
        <show_ticket_norms>0</show_ticket_norms>
        <show_wt_norms>1</show_wt_norms>
        <city_arv>СПТ</city_arv>
        <mark_flight>
          <airline>ЮТ</airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
          <airp_dep>ДМД</airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>...
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>ВЗ</pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>Э</subclass>
            <bag_pool_num/>
            <tid>...
            <ticket_no>2986120030297</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
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
            <ticket_bag_norm>нет</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <service_lists>
              <service_list seg_no='0' category='1'...
              <service_list seg_no='0' category='2'...
              <service_list seg_no='1' category='1'...
              <service_list seg_no='1' category='2'...
            </service_lists>
            <rems>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PP7774441110</rem_text>
              </rem>
            </rems>
            <asvc_rems/>
            <fqt_rems/>
            <norms>
              <norm>
                <bag_type/>
                <norm_id/>
                <norm_trfer/>
                <norm_type/>
                <amount/>
                <weight/>
                <per_unit/>
              </norm>
            </norms>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters>
          <item>
            <point_arv>...
            <class>Э</class>
            <noshow>...
            <trnoshow>0</trnoshow>
            <show>1</show>
            <free_ok>...
            <free_goshow>...
            <nooccupy>...
          </item>
        </tripcounters>
        <load_residue/>
      </segment>
      <segment>
        <tripheader>
          <flight>С71027/$(date_format %d.%m) ПЛК (EDI)</flight>
          <airline>С7</airline>
          <aircode>421</aircode>
          <flt_no>1027</flt_no>
          <suffix/>
          <airp>ПЛК</airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:00:00</scd_out_local>
          <scd_brd_to_local>11:55</scd_brd_to_local>
          <remote_gate/>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <airp_code>СОЧ</airp_code>
              <city_code>СОЧ</city_code>
              <target_view>СОЧИ (СОЧ)</target_view>
              <check_info>
                <pass/>
                <crew/>
              </check_info>
            </airp>
          </airps>
          <classes/>
          <gates/>
          <halls/>
          <mark_flights/>
        </tripdata>
        <grp_id>-1</grp_id>
        <point_dep>-...
        <airp_dep>ПЛК</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>СОЧ</airp_arv>
        <class/>
        <status>K</status>
        <bag_refuse/>
        <piece_concept>0</piece_concept>
        <tid>0</tid>
        <city_arv>СОЧ</city_arv>
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>ВЗ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>xx</seat_no>
            <reg_no>22</reg_no>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2986120030297</ticket_no>
            <coupon_no>2</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems/>
            <fqt_rems/>
            <addresses/>
            <doco/>
            <iatci_pax_id/>
            <iatci_parent_pax_id/>
            <document>
              <type>P</type>
              <no>7878787899</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1975 00:00:00</birth_date>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
              <second_name>IVANICH</second_name>
            </document>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters/>
        <load_residue/>
      </segment>
    </segments>


$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(REMOVE_PAX_DOC $(get point_dep) $(get point_arv) ДМД ПЛК
                 $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                 REPIN IVAN IVANICH 2986120030297 2)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
UAP+C+A:REPIN:IVAN:750501:::RUS++P:7878787899::::491231:::::::REPIN:IVAN:IVANICH"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
            <iatci_pax_id/>
            <iatci_parent_pax_id/>
            <document/>

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

# у пассажира документа нет, поэтому в тлг должно пойти добавление

$(UPDATE_PAX_DOC $(get point_dep) $(get point_arv) ДМД ПЛК
                 $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                 REPIN IVAN IVANICH 2986120030297 2)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
UAP+A+A:REPIN:IVAN:750501:::RUS++P:7878787899::::491231:::::::REPIN:IVAN:IVANICH"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
            <document>
              <type>P</type>
              <no>7878787899</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1975 00:00:00</birth_date>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
              <second_name>IVANICH</second_name>
            </document>

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

# нет изменений - тлг недолжна пойти

$(UPDATE_PAX_DOC $(get point_dep) $(get point_arv) ДМД ПЛК
                 $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                 REPIN IVAN IVANICH 2986120030297 2)


%%
#########################################################################################
# №11 Обновление данных регистрации без IATCI
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PFD+xx+:Э"
PSI++TKNE::29861200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS::::M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(UPDATE_PAX_DOC_NON_IATCI $(get point_dep) $(get point_arv) ДМД ПЛК
                           $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                           REPIN IVAN 2986120030297 1)

>> lines=auto
            <document>
              <type>P</type>
              <no>123424124</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
            </document>


%%
#########################################################################################
# №12 Обновление данных регистрации (Ремарки)
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PFD+xx+:Э"
PSI++TKNE::29861200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"



>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(UPDATE_PAX_REMS $(get point_dep) $(get point_arv) ДМД ПЛК
                  $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                  REPIN IVAN IVANICH 4216120030297 2)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
USI++C:FOID::::::FOID PP7774441110+A:FOID::::::FOID PP7774449999+A:OTHS::::::OTHS HK1 DOCS/7777771110/PS+A:FQTV:UT:7788990011"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
PFD+xx+:Э"
PSI++TKNE::42161200302972+FOID::::::FOID PP7774449999+OTHS::::::OTHS HK1 DOCS/7777771110/PS"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
            <rems>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PP7774449999</rem_text>
              </rem>
              <rem>
                <rem_code>OTHS</rem_code>
                <rem_text>OTHS HK1 DOCS/7777771110/PS</rem_text>
              </rem>
            </rems>

$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(CANCEL_PAX_REMS $(get point_dep) $(get point_arv) ДМД ПЛК
                  $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                  REPIN IVAN IVANICH 4216120030297 2)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
USI++C:FOID::::::FOID PP7774449999+C:OTHS::::::OTHS HK1 DOCS/7777771110/PS+C:FQTV:UT:7788990011"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+U+O"
PPD+REPIN+A:N++IVAN"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>ВЗ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>xx</seat_no>
            <reg_no/>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2986120030297</ticket_no>
            <coupon_no>2</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <document>
              <type>PP</type>
              <issue_country>RUS</issue_country>
              <no>5408123432</no>
              <nationality>RUS</nationality>
              <birth_date>10.03.1986 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
            </document>
            <addresses/>
            <doco/>
            <iatci_pax_id/>
            <iatci_parent_pax_id/>
            <rems/>
            <fqt_rems/>


%%
#########################################################################################
# №13 Обновление данных регистрации (Ремарки длинные)
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PFD+xx+:Э"
PSI++TKNE::29861200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))


$(UPDATE_PAX_REMS_WITH_LONG $(get point_dep) $(get point_arv) ДМД ПЛК
                            $(get grp_id) $(get pax_id) $(get tid) ПЛК СОЧ
                            REPIN IVAN IVANICH 2986120030297 2)


%%
#########################################################################################
# №14 Печать ПТ
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(prepare_bp_printing ЮТ 103 ДМД)
$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+O"
PPD+REPIN+A:N++IVAN"
PFD+7A+:Э+32"
PSI++TKNE::29861200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))


!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='print' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetGRPPrintDataBP>
      <grp_id>$(get grp_id)</grp_id>
      <pr_all>1</pr_all>
      <dev_model>506</dev_model>
      <fmt_type>ATB</fmt_type>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>CP866</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <clientData>
        <gate>1</gate>
      </clientData>
    </GetGRPPrintDataBP>
  </query>
</term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQBPR:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+B+X"
ERD+1:35"
UNT+5+1"
UNZ+1+1"

>> lines=auto
    <kick req_ctxt_id...

!! err=$(utf8 "Рейс закрыт")
$(lastRedisplay)


!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='print' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetGRPPrintDataBP>
      <grp_id>$(get grp_id)</grp_id>
      <pr_all>1</pr_all>
      <dev_model>506</dev_model>
      <fmt_type>ATB</fmt_type>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>CP866</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <clientData>
        <gate>1</gate>
      </clientData>
    </GetGRPPrintDataBP>
  </query>
</term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQBPR:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+B+O"
FSD+1155++GATE7"
PPD+REPIN+A:N++IVAN"
PFD+7A+:Э+32"
PSI++TKNE::29861200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)


>> lines=auto
    <data>
      <printBP>
        <pectab>PT{##}?K1Z{#}@;{#}TICK{#}&gt;&gt;/{#}BOARD{#}0101{#}0250E01W{#}0311H01W{#}0411L01W{#}0508H13W{#}0705H40{#}0906L27W{#}0A05M12R{#}0B01H30W{#}0C06L38W{#}0D16O02W{#}0E21Q01Q52W{#}0F15R01R53W{#}2020C53W{#}2120E53W{#}2220H53W{#}2508O53W{#}2705L53O{#}2B01L67W{#}2C06O64W{#}3FB1R30B601031{#}F104D41A54{#}FF72M01W{#}</pectab>
        <passengers>
          <pax pax_id...
            <prn_form hex='0'>CP{#}1C01{#}01K{#}02{#}02REPIN IVAN                                        {#}03ДОМОДЕДОВО {#}04ПУЛКОВО    {#}05 ЮТ103  {#}07$(date_format %d.%m +0){#}091     {#}0A09:50{#}0BЭ{#}0C    1A{#}0DKGS{#}0EETKT2986120030297/1  {#}0F7774441110     {#}20REPIN IVAN          {#}21ДОМОДЕДОВО          {#}22ПУЛКОВО             {#}25 ЮТ103  {#}27$(date_format %d.%m +0){#}2BЭ{#}2C  1A  {#}3Fxxxxxxxxxx{#}F1001{#}FF                                                                        {#}</prn_form>
          </pax>
          <pax pax_id...
            <prn_form hex='0'>CP{#}1C01{#}01K{#}02{#}02REPIN IVAN                                        {#}03ПУЛКОВО    {#}04СОЧИ       {#}05 С71027 {#}07$(date_format %d.%m +0){#}09GATE7 {#}0A11:55{#}0BЭ{#}0C    7A{#}0DKGS{#}0EETKT2986120030297/2  {#}0F5408123432     {#}20REPIN IVAN          {#}21ПУЛКОВО             {#}22СОЧИ                {#}25 С71027 {#}27$(date_format %d.%m +0){#}2BЭ{#}2C  7A  {#}3F2000000000{#}F1032{#}FF                                                                        {#}</prn_form>
          </pax>
        </passengers>
      </printBP>
    </data>


%%
#########################################################################################
# №15 Карта мест
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(prepare_bp_printing ЮТ 103 ДМД)
$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+O"
FSD+1155"
PPD+REPIN+A:N++IVAN"
PFD+3B+:Э"
PSI++TKNE::29861200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))


!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='salonform' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <Show>
      <trip_id>-$(get grp_id)</trip_id>
    </Show>
  </query>
</term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQSMF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
SRP+Y"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRSMF:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+S+X"
ERD+1:5"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...

!! err=$(utf8 "Неверный рейс/дата")
$(lastRedisplay)


!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='salonform' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <Show>
      <trip_id>-$(get grp_id)</trip_id>
    </Show>
  </query>
</term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQSMF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
SRP+Y"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRSMF:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+S+O"
EQD++++++D09"
CBD+F+3:6+++F++A:W+B:A+E:A+F:W"
ROD+3++A::K+B::K+E::K+F::K"
ROD+6++A+B:O+E+F"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
      <salons pr_lat_seat='1' RFISCMode='0'>
        <filterRoutes>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <items>
            <item>
              <point_id>-1</point_id>
              <airp>ПЛК</airp>
            </item>
            <item>
              <point_id>-1</point_id>
              <airp>СОЧ</airp>
            </item>
          </items>
        </filterRoutes>
        <placelist num='0' xcount='5' ycount='4'>
          <place>
            <x>0</x>
            <y>0</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>A</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>1</x>
            <y>0</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>B</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>3</x>
            <y>0</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>E</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>4</x>
            <y>0</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>F</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>0</x>
            <y>1</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>A</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>1</x>
            <y>1</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>B</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>3</x>
            <y>1</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>E</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>4</x>
            <y>1</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>F</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>0</x>
            <y>2</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>A</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>1</x>
            <y>2</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>B</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>3</x>
            <y>2</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>E</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>4</x>
            <y>2</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>F</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>0</x>
            <y>3</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>A</xname>
            <yname>6</yname>
          </place>
          <place>
            <x>1</x>
            <y>3</y>
            <layers>
              <layer>
                <layer_type>CHECKIN</layer_type>
              </layer>
            </layers>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>B</xname>
            <yname>6</yname>
          </place>
          <place>
            <x>3</x>
            <y>3</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>E</xname>
            <yname>6</yname>
          </place>
          <place>
            <x>4</x>
            <y>3</y>
            <elem_type>К</elem_type>
            <class>П</class>
            <xname>F</xname>
            <yname>6</yname>
          </place>
        </placelist>
      </salons>
    </data>


!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='salonform' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <Reseat>
      <trip_id>-$(get grp_id)</trip_id>
      <pax_id>-1</pax_id>
      <xname>A</xname>
      <yname>6</yname>
      <tid>0</tid>
      <question_reseat/>
    </Reseat>
  </query>
</term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:N++IVAN"
USD++006A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+U+O"
FSD+1155"
PPD+REPIN+A:N++IVAN"
PFD+6A+:Э"
PSI++TKNE::29861200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQSMF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
SRP+Y"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRSMF:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+S+O"
EQD++++++D09"
CBD+F+3:6+++F++A:W+B:A+E:A+F:W"
ROD+3++A::K+B::K+E::K+F::K"
ROD+6++A:O+B+E+F"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
    <data>
      <tid>0</tid>
      <seat_no>6A</seat_no>
      <layer_type>CHECKIN</layer_type>
      <update_salons RFISCMode='0'>
        <seats>
          <salon num='0'>
            <place>
              <x>1</x>
              <y>0</y>
            </place>
            <place>
              <x>0</x>
              <y>3</y>
              <layers>
                <layer>
                  <layer_type>CHECKIN</layer_type>
                </layer>
              </layers>
            </place>
          </salon>
        </seats>
      </update_salons>
    </data>


%%
#########################################################################################
# №16 Карта мест до регистрации
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(prepare_bp_printing ЮТ 103 ДМД)
$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
#$(CHECK_ADV_TRIPS_LIST $(get point_dep) ЮТ 103 ДМД)


!!
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='salonform' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <Show>
      <trip_id>-1</trip_id>
    </Show>
  </query>
</term>}


%%
#########################################################################################
# №17 Ошибка в ответ на регистрацию и работа без интерактива
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(deny_ets_interactive ЮТ 103 ДМД)

$(OPEN_CHECKIN $(get point_dep))
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN IVAN К)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+I+F"
ERD+1:17:PASSENGER SURNAME ALREADY CHECKED IN"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!! err="PASSENGER SURNAME ALREADY CHECKED IN"
$(lastRedisplay)


%%
#########################################################################################
# №18 Таймаут на регистрацию - должен пойти IFM/del
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA REMIFM OURIFM)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN IVAN К)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...

!! err=$(utf8 "Нет связи с удаленной DCS")
$(lastRedisplay)


# откат смены статуса в СЭБ
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:I"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


# IFM
>>
REMIFM
.OURIFM
IFM
UT103/$(ddmon +0 en) DME
-S71027/$(ddmon +0 en) LEDAER
DEL
1REPIN/IVAN
ENDIFM



%%
#########################################################################################
# №19 Таймаут и ошибка на отмену регистрации - должен пойти IFM/del
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA REMIFM OURIFM)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN IVAN К)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)


$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
FSD+1155"
PPD+REPIN+A:N++IVAN"
PFD+xx+:Э"
PSI++TKNE::29861200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

# сначала таймаут

$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
             ЮТ 103 ДМД ПЛК
             С7 1027 ПЛК СОЧ
             REPIN IVAN 2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 I)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...

!! err=$(utf8 "Нет связи с удаленной DCS")
$(lastRedisplay)


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

>>
REMIFM
.OURIFM
IFM
UT103/$(ddmon +0 en) DME
-S71027/$(ddmon +0 en) LEDAER
DEL
1REPIN/IVAN
ENDIFM



# потом ошибка

$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
             ЮТ 103 ДМД ПЛК
             С7 1027 ПЛК СОЧ 
             REPIN IVAN 2986120030297)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+X+X"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...

!! err=ignore
$(lastRedisplay)

>>
REMIFM
.OURIFM
IFM
UT103/$(ddmon +0 en) DME
-S71027/$(ddmon +0 en) LEDAER
DEL
1REPIN/IVAN
ENDIFM



%%
#########################################################################################
# №20 первичная регистрация с ошибкой со статусом N
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN IVAN К)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)
$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+N"
ERD+1:35"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!! err=ignore
$(lastRedisplay)


$(ETS_COS_EXCHANGE 2986120030297 1 I SYSTEM)



%%
#########################################################################################
# №21 первичная регистрация группы из двух пассажиров
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)

# подготовка рейса
$(PREPARE_FLIGHT_5 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ
                   REPIN IVAN 2982401841689 1
                   PETROV PETR 2982401841612 1)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_1_id $(get_single_pax_id $(get point_dep) REPIN IVAN))
$(set pax_2_id $(get_single_pax_id $(get point_dep) PETROV PETR))


$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2982401841689 REPIN IVAN)
$(SAVE_ET_DISP $(get point_dep) 2982401841612 PETROV PETR)
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN IVAN К)
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК PETROV PETR К)
$(CHECK_TCKIN_ROUTE_GRP_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ PETROV PETR РБ)
$(CHECK_TCKIN_ROUTE_GRP_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ PETROV PETR РБ)

$(SAVE_GRP $(get pax_1_id) $(get pax_2_id) $(get point_dep) $(get point_arv)
                ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ
                REPIN IVAN 2982401841689 ВЗ
                PETROV PETR 2982401841612 РБ)


$(ETS_COS_EXCHANGE2 2982401841689 1 2982401841612 1 CK)

$(KICK_IN_SILENT)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PAP+A:REPIN:IVAN++V:4538926:UKR:::210109::::MOSCOW::110109"
PPD+PETROV+C:N++PETR"
PRD+Y"
PSD++008A"
PBD+0"
PAP+A:PETROV:PETR++VI:13452:ESP:::190320::::MINSK::090320"
UNT+14+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150217:0747+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1015+LED+AER++T"
RAD+I+O"
FSD+1155"
PPD+REPIN+A:N++IVAN"
PFD+7A++1"
PSI++TKNE::29824018416891+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M"
PAP+:::850724:::TJK++P:400522509:TJK:::250205:M::::::REPIN:IVAN+V:00000:RUS:::210321::::SPT::010311"
PPD+PETROV+C:N++PETR"
PFD+8A++2"
PSI++TKNE::29824018416121+DOCS::::::DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR+FOID::::::FOID PPZB400522510+PSPT::::::PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M"
PAP+:::850724:::TJK++P:400522510:TJK:::250205:M::::::PETROV:PETR+V:13452:USA:::210320::::NY::010310"
UNT+12+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>ЮТ103...
          <flight_short>ЮТ103...
          <airline>ЮТ</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>ДМД</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:15:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>$(get point_arv)</point_id>
              <airp_code>ПЛК</airp_code>
              <city_code>СПТ</city_code>
              <target_view>САНКТ-ПЕТЕРБУРГ (ПЛК)</target_view>
              <check_info>
                <pass>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </pass>
                <crew>
                  <doc/>
                  <doco/>
                  <doca_b/>
                  <doca_r/>
                  <doca_d/>
                  <tkn/>
                </crew>
              </check_info>
            </airp>
          </airps>
          <classes>
            <class>
              <code>Э</code>
              <class_view>ЭКОНОМ</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>1</id>
              <name>Зал 1</name>
            </hall>
            <hall>
              <id>0</id>
              <name>Др.</name>
            </hall>
            <hall>
              <id>1141</id>
              <name>VIP</name>
            </hall>
            <hall>
              <id>1439</id>
              <name>Пав. вокз.</name>
            </hall>
            <hall>
              <id>39706</id>
              <name>супер зал</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>ЮТ</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y +0) 10:15:00</scd>
              <airp_dep>ДМД</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>...
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>ДМД</airp_dep>
        <point_arv>$(get point_arv)</point_arv>
        <airp_arv>ПЛК</airp_arv>
        <class>Э</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>...
        <show_ticket_norms>0</show_ticket_norms>
        <show_wt_norms>1</show_wt_norms>
        <city_arv>СПТ</city_arv>
        <mark_flight>
          <airline>ЮТ</airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
          <airp_dep>ДМД</airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>$(get pax_1_id)</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>ВЗ</pers_type>
            <crew_type/>
            <seat_no>1A</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>Э</subclass>
            <bag_pool_num/>
            <tid>...
            <ticket_no>2982401841689</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
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
            <ticket_bag_norm>нет</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <transfer>
              <segment>
                <subclass>L</subclass>
              </segment>
            </transfer>
            <service_lists>
              <service_list seg_no='0' category='1'...
              <service_list seg_no='0' category='2'...
              <service_list seg_no='1' category='1'...
              <service_list seg_no='1' category='2'...
            </service_lists>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>INFT</rem_code>
                <rem_text>INFT HK1 01JAN17 PETROV/PETR</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
              <rem>
                <rem_code>TKNE</rem_code>
                <rem_text>TKNE HK1 2982401841689/1</rem_text>
              </rem>
            </rems>
            <asvc_rems/>
            <fqt_rems/>
            <norms/>
          </pax>
          <pax>
            <pax_id>$(get pax_2_id)</pax_id>
            <surname>PETROV</surname>
            <name>PETR</name>
            <pers_type>РБ</pers_type>
            <crew_type/>
            <seat_no>1B</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>2</reg_no>
            <subclass>Э</subclass>
            <bag_pool_num/>
            <tid>...
            <ticket_no>2982401841612</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>7774441112</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <gender>M</gender>
              <surname>PETROV</surname>
              <first_name>PETR</first_name>
            </document>
            <ticket_bag_norm>нет</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <transfer>
              <segment>
                <subclass>L</subclass>
              </segment>
            </transfer>
            <service_lists>
              <service_list seg_no='0' category='1'...
              <service_list seg_no='0' category='2'...
              <service_list seg_no='1' category='1'...
              <service_list seg_no='1' category='2'...
            </service_lists>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522510</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M</rem_text>
              </rem>
              <rem>
                <rem_code>TKNE</rem_code>
                <rem_text>TKNE HK1 2982401841612/1</rem_text>
              </rem>
            </rems>
            <asvc_rems/>
            <fqt_rems/>
            <norms/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters>
          <item>
            <point_arv>$(get point_arv)</point_arv>
            <class>Э</class>
            <noshow>...
            <trnoshow>...
            <show>...
            <free_ok>...
            <free_goshow>...
            <nooccupy>...
          </item>
        </tripcounters>
        <load_residue/>
      </segment>
      <segment>
        <tripheader>
          <flight>С71027/$(date_format %d.%m +0) ПЛК (EDI)</flight>
          <airline>С7</airline>
          <aircode>421</aircode>
          <flt_no>1027</flt_no>
          <suffix/>
          <airp>ПЛК</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:15:00</scd_out_local>
          <scd_brd_to_local>11:55</scd_brd_to_local>
          <remote_gate/>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
          <use_jmp>0</use_jmp>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <airp_code>СОЧ</airp_code>
              <city_code>СОЧ</city_code>
              <target_view>СОЧИ (СОЧ)</target_view>
              <check_info>
                <pass/>
                <crew/>
              </check_info>
            </airp>
          </airps>
          <classes/>
          <gates/>
          <halls/>
          <mark_flights/>
        </tripdata>
        <grp_id>-1</grp_id>
        <point_dep>...
        <airp_dep>ПЛК</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>СОЧ</airp_arv>
        <class/>
        <status>K</status>
        <bag_refuse/>
        <piece_concept>0</piece_concept>
        <tid>0</tid>
        <city_arv>СОЧ</city_arv>
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>ВЗ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>7A</seat_no>
            <reg_no>1</reg_no>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841689</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <document>
              <type>P</type>
              <issue_country>TJK</issue_country>
              <no>400522509</no>
              <nationality>TJK</nationality>
              <birth_date>24.07.1985 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
              <expiry_date>05.02.2025 00:00:00</expiry_date>
            </document>
            <addresses/>
            <doco>
              <type>V</type>
              <no>00000</no>
              <issue_place>SPT</issue_place>
              <applic_country>RUS</applic_country>
              <issue_date>11.03.2001 00:00:00</issue_date>
              <expiry_date>21.03.2021 00:00:00</expiry_date>
            </doco>
            <iatci_pax_id/>
            <iatci_parent_pax_id/>
          </pax>
          <pax>
            <pax_id>-2</pax_id>
            <surname>PETROV</surname>
            <name>PETR</name>
            <pers_type>РБ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>8A</seat_no>
            <reg_no>2</reg_no>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841612</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522510/TJK/24JUL85/M/05FEB25/PETROV/PETR</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522510</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522510/TJK/24JUL85/PETROV/PETR/M</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <document>
              <type>P</type>
              <issue_country>TJK</issue_country>
              <no>400522510</no>
              <nationality>TJK</nationality>
              <birth_date>24.07.1985 00:00:00</birth_date>
              <gender>M</gender>
              <surname>PETROV</surname>
              <first_name>PETR</first_name>
              <expiry_date>05.02.2025 00:00:00</expiry_date>
            </document>
            <addresses/>
            <doco>
              <type>V</type>
              <no>13452</no>
              <issue_place>NY</issue_place>
              <applic_country>USA</applic_country>
              <issue_date>10.03.2001 00:00:00</issue_date>
              <expiry_date>20.03.2021 00:00:00</expiry_date>
            </doco>
            <iatci_pax_id/>
            <iatci_parent_pax_id/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters/>
        <load_residue/>
      </segment>
    </segments>


$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(dump_table GRP_IATCI_XML)

# пересадим пассажира PETROV PETR

!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='salonform' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <Reseat>
      <trip_id>-$(get grp_id)</trip_id>
      <pax_id>-2</pax_id>
      <xname>A</xname>
      <yname>6</yname>
      <tid>0</tid>
      <question_reseat/>
    </Reseat>
  </query>
</term>}


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1015+LED+AER"
PPD+PETROV+C:N++PETR"
USD++006A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+LED+AER++T"
RAD+U+O"
PPD+PETROV+C:N++PETR"
PFD+006A"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQSMF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1015+LED+AER"
SRP+Y"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRSMF:96:2:IA+$(last_edifact_ref)"
FDR+С7+1027+$(yymmdd)1000+LED+AER++T"
RAD+S+O"
EQD++++++D09"
CBD+F+3:7+++F++A:W+B:A+E:A+F:W"
ROD+3++A::K+B::K+E::K+F::K"
ROD+6++A:O+B+E+F"
ROD+7++A:O+B+E+F"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)


# отменяем регистрацию одного из пассажиров
$(CANCEL_PAX $(get pax_1_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
             ЮТ 103 ДМД ПЛК
             С7 1027 ПЛК СОЧ
             REPIN IVAN 2982401841689)


$(ETS_COS_EXCHANGE 2982401841689 1 I)

$(KICK_IN_SILENT)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A:N++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+ПЛК+СОЧ++T"
RAD+X+O"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)


%%
#########################################################################################
# №22 младенец и багаж
###

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)

# подготовка рейса
$(PREPARE_FLIGHT_6 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ
                   REPIN ADULT 2982401841689 1
                   REPIN INFANT 2982401841612 1)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_1_id $(get_single_pax_id $(get point_dep) REPIN ADULT))
$(set pax_2_id $(get_single_pax_id $(get point_dep) REPIN INFANT))


$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2982401841689 REPIN ADULT)
$(SAVE_ET_DISP $(get point_dep) 2982401841612 REPIN INFANT)
# $(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN ADULT К)
# $(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN INFANT К)
$(CHECK_TCKIN_ROUTE_GRP_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN ADULT ВЗ REPIN INFANT РМ)
$(CHECK_TCKIN_ROUTE_GRP_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN ADULT ВЗ REPIN INFANT РМ)

$(deny_ets_interactive ЮТ 103 ДМД)

$(SAVE_GRP $(get pax_1_id) $(get pax_2_id) $(get point_dep) $(get point_arv)
           ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ
           REPIN ADULT 2982401841689 ВЗ
           REPIN INFANT 2982401841612 РМ)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:Y++ADULT+REPIN:INFANT"
PRD+Y"
PSD++007A"
PBD+0"
PAP+A:REPIN:ADULT++V:4538926:UKR:::210109::::MOSCOW::110109"
PAP+IN:REPIN:INFANT++VI:13452:ESP:::190320::::MINSK::090320"
UNT+10+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150217:0745+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
FSD+1155"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
PRD+Y"
PFD+001A+:Y+001:002"
PSI++TKNE::29824018416891+TKNE::INF29824018416121+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1+INFT::::::INFT HK1 01JAN17 REPIN/INFANT"
PAP+A:REPIN:ADULT:760501:::RUS++P:7774441110:RUS::::M::::::REPIN:ADULT+V:4538926:UKR:::210109::::MOSCOW::110109"
PAP+IN:REPIN:INFANT:760501:::RUS++P:7774441112:RUS::::M::::::REPIN:INFANT+VI:13452:ESP:::190320::::MINSK::090320"
UNT+10+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>ADULT</name>
            <pers_type>ВЗ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>1A</seat_no>
            <reg_no>001</reg_no>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841689</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
              <rem>
                <rem_code>INFT</rem_code>
                <rem_text>INFT HK1 01JAN17 REPIN/INFANT</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>7774441110</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>ADULT</first_name>
            </document>
            <addresses/>
            <doco>
              <type>V</type>
              <no>4538926</no>
              <issue_place>MOSCOW</issue_place>
              <applic_country>UKR</applic_country>
              <issue_date>09.01.2011 00:00:00</issue_date>
              <expiry_date>09.01.2021 00:00:00</expiry_date>
            </doco>
            <iatci_pax_id>0013949613</iatci_pax_id>
            <iatci_parent_pax_id/>
          </pax>
          <pax>
            <pax_id>-2</pax_id>
            <surname>REPIN</surname>
            <name>INFANT</name>
            <pers_type>РМ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no/>
            <reg_no>002</reg_no>
            <seat_type/>
            <seats>0</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841612</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
              <rem>
                <rem_code>INFT</rem_code>
                <rem_text>INFT HK1 01JAN17 REPIN/INFANT</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>7774441112</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>INFANT</first_name>
            </document>
            <addresses/>
            <doco>
              <type>V</type>
              <no>13452</no>
              <issue_place>MINSK</issue_place>
              <applic_country>ESP</applic_country>
              <issue_date>20.03.2009 00:00:00</issue_date>
              <expiry_date>20.03.2019 00:00:00</expiry_date>
            </doco>
            <iatci_pax_id>0013949614</iatci_pax_id>
            <iatci_parent_pax_id>-1</iatci_parent_pax_id>
          </pax>
        </passengers>


$(set grp_id $(get_single_grp_id $(get point_dep) REPIN ADULT))
$(set tid $(get_single_tid $(get point_dep) REPIN ADULT))


!!
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(get point_dep)</point_dep>
          <point_arv>$(get point_arv)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>ПЛК</airp_dep>
          <airp_arv>СОЧ</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>REPIN</surname>
              <name>ADULT</name>
              <pers_type>ВЗ</pers_type>
              <refuse/>
              <ticket_no>29824018416891</ticket_no>
              <coupon_no>2</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>REPIN</surname>
                <first_name>ADULT</first_name>
              </document>
              <doco>
                <birth_place/>
                <type>V</type>
                <no>88888888</no>
                <issue_place>MOSCOW</issue_place>
                <issue_date>01.01.2011 00:00:00</issue_date>
                <expiry_date>02.01.2031 00:00:00</expiry_date>
                <applic_country>RUS</applic_country>
              </doco>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
            </pax>
            <pax>
              <pax_id>-2</pax_id>
              <surname>REPIN</surname>
              <name>INFANT</name>
              <pers_type>РМ</pers_type>
              <refuse/>
              <ticket_no>2982401841612</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                  <type>P</type>
                  <no>123456789</no>
                  <nationality>RUS</nationality>
                  <birth_date>01.05.2016 00:00:00</birth_date>
                  <expiry_date>31.12.2049 00:00:00</expiry_date>
                  <surname>REPIN</surname>
                  <first_name>INFANT</first_name>
                  <second_name>IVANOVICH</second_name>
              </document>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
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

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
UAP+R+A:REPIN:ADULT++V:88888888:RUS:::310102::::MOSCOW::110101"
UAP+R+IN:REPIN:INFANT:160501:::RUS++P:123456789::::491231:::::::REPIN:INFANT:IVANOVICH"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150217:0745+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+U+O"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
PRD+Y"
PFD+001A+:Y+001:002"
PSI++TKNE::29824018416891+TKNE::INF29824018416121+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1+INFT::::::INFT HK1 01JAN17 REPIN/INFANT"
PAP+A:REPIN:ADULT:760501:::RUS++P:987654321::::491231:::::::REPIN:ADULT:PETROVICH+V:88888888:RUS:::310102::::MOSCOW::110101"
PAP+IN:REPIN:INFANT:160501:::RUS++P:123456789::::491231:::::::REPIN:INFANT:IVANOVICH"
UNT+10+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>ADULT</name>
            <pers_type>ВЗ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>1A</seat_no>
            <reg_no>001</reg_no>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841689</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
              <rem>
                <rem_code>INFT</rem_code>
                <rem_text>INFT HK1 01JAN17 REPIN/INFANT</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>7774441110</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>ADULT</first_name>
            </document>
            <addresses/>
            <iatci_pax_id>0013949613</iatci_pax_id>
            <iatci_parent_pax_id/>
            <doco>
              <birth_place/>
              <type>V</type>
              <no>88888888</no>
              <issue_place>MOSCOW</issue_place>
              <issue_date>01.01.2011 00:00:00</issue_date>
              <expiry_date>02.01.2031 00:00:00</expiry_date>
              <applic_country>RUS</applic_country>
            </doco>
          </pax>
          <pax>
            <pax_id>-2</pax_id>
            <surname>REPIN</surname>
            <name>INFANT</name>
            <pers_type>РМ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no/>
            <reg_no>002</reg_no>
            <seat_type/>
            <seats>0</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841612</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
              <rem>
                <rem_code>INFT</rem_code>
                <rem_text>INFT HK1 01JAN17 REPIN/INFANT</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <addresses/>
            <doco>
              <type>V</type>
              <no>13452</no>
              <issue_place>MINSK</issue_place>
              <applic_country>ESP</applic_country>
              <issue_date>20.03.2009 00:00:00</issue_date>
              <expiry_date>20.03.2019 00:00:00</expiry_date>
            </doco>
            <iatci_pax_id>0013949614</iatci_pax_id>
            <iatci_parent_pax_id>-1</iatci_parent_pax_id>
            <document>
              <type>P</type>
              <no>123456789</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.2016 00:00:00</birth_date>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
              <surname>REPIN</surname>
              <first_name>INFANT</first_name>
              <second_name>IVANOVICH</second_name>
            </document>
          </pax>
        </passengers>

$(set tid_new $(get_single_tid $(get point_dep) REPIN ADULT))

!!
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(get point_dep)</point_dep>
          <point_arv>$(get point_arv)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid)</tid>
          <passengers/>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>ПЛК</airp_dep>
          <airp_arv>СОЧ</airp_arv>
          <class>Э</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>REPIN</surname>
              <name>ADULT</name>
              <pers_type>ВЗ</pers_type>
              <refuse/>
              <ticket_no>29824018416891</ticket_no>
              <coupon_no>2</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                  <type>P</type>
                  <no>987654321</no>
                  <nationality>RUS</nationality>
                  <birth_date>01.05.1976 00:00:00</birth_date>
                  <expiry_date>31.12.2049 00:00:00</expiry_date>
                  <surname>REPIN</surname>
                  <first_name>ADULT</first_name>
                  <second_name>PETROVICH</second_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num>1</bag_pool_num>
              <subclass>Э</subclass>
              <tid>0</tid>
            </pax>
            <pax>
              <pax_id>-2</pax_id>
              <surname>REPIN</surname>
              <name>INFANT</name>
              <pers_type>РМ</pers_type>
              <refuse/>
              <ticket_no>2982401841612</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                  <type>P</type>
                  <no>123456789</no>
                  <nationality>RUS</nationality>
                  <birth_date>01.05.2016 00:00:00</birth_date>
                  <expiry_date>31.12.2049 00:00:00</expiry_date>
                  <surname>REPIN</surname>
                  <first_name>INFANT</first_name>
                  <second_name>IVANOVICH</second_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
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



$(sql "insert into TRIP_BT(POINT_ID, TAG_TYPE) values($(get point_dep), 'ЮТ')")

$(set pax_tid $(get_single_pax_tid $(get point_dep) REPIN ADULT))
$(set tid $(get_single_tid $(get point_dep) REPIN ADULT))

$(SAVE_GRP_BAGGAGE $(get grp_id) $(get tid)
                   $(get point_dep) $(get point_arv)
                   ДМД ПЛК ПЛК СОЧ
                   $(get pax_1_id) $(get pax_tid) REPIN ADULT ВЗ 2982401841689
                   P 987654321 RUS
                   "01.05.1976 00:00:00" "31.12.2049 00:00:00"
                   REPIN ADULT PETROVICH)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
UBD+R:2:25+R:1:5+R:NP+R:UT:1:2:AER:298"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150217:0745+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)+LED+AER++T"
RAD+U+P"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

$(set tid_new $(get_single_tid $(get point_dep) REPIN ADULT))


$(REMOVE_ONE_BAG $(get grp_id) $(get tid_new)
                 $(get point_dep) $(get point_arv)
                 ДМД ПЛК ПЛК СОЧ)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
UBD+R:1:13+R:1:5+R:NP+R:UT:3:1:AER:298"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150217:0745+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)+LED+AER++T"
RAD+U+P"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

$(set tid_new $(get_single_tid $(get point_dep) REPIN ADULT))
$(set pax_tid $(get_single_pax_tid $(get point_dep) REPIN ADULT))

$(REMOVE_ALL_BAGS $(get grp_id) $(get tid_new)
                  $(get point_dep) $(get point_arv)
                  ДМД ПЛК ПЛК СОЧ
                  $(get pax_1_id) $(get pax_tid) REPIN ADULT ВЗ 2982401841689
                  P 987654321 RUS
                  "01.05.1976 00:00:00" "31.12.2049 00:00:00"
                  REPIN ADULT PETROVICH)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
UBD+R:0+R:0+R:NP"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150217:0745+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)+LED+AER++T"
RAD+U+P"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)


$(set tid_new $(get_single_tid $(get point_dep) REPIN ADULT))
$(set adult_tid $(get_single_pax_tid $(get point_dep) REPIN ADULT))
$(set infant_tid $(get_single_pax_tid $(get point_dep) REPIN INFANT))

!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(get point_dep)</point_dep>
          <point_arv>$(get point_arv)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid_new)</tid>
          <passengers>
            <pax>
              <pax_id>$(get pax_1_id)</pax_id>
              <surname>REPIN</surname>
              <name>ADULT</name>
              <pers_type>ВЗ</pers_type>
              <ticket_no>2982401841689</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>REPIN</surname>
                <first_name>ADULT</first_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>$(get adult_tid)</tid>
            </pax>
            <pax>
              <pax_id>$(get pax_2_id)</pax_id>
              <surname>REPIN</surname>
              <name>INFANT</name>
              <pers_type>РМ</pers_type>
              <ticket_no>2982401841612</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>REPIN</surname>
                <first_name>INFANT</first_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>$(get infant_tid)</tid>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>ПЛК</airp_dep>
          <airp_arv>СОЧ</airp_arv>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>REPIN</surname>
              <name>ADULT</name>
              <pers_type>ВЗ</pers_type>
              <ticket_no>2982401841689</ticket_no>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>1</ticket_confirm>
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
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
            </pax>
            <pax>
              <pax_id>-2</pax_id>
              <surname>REPIN</surname>
              <name>INFANT</name>
              <pers_type>РМ</pers_type>
              <ticket_no>2982401841612</ticket_no>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>1</ticket_confirm>
              <addresses>
                <doca>
                  <type>B</type>
                  <country>RUS</country>
                  <city>MOSCOW</city>
                  <postal_code>127650</postal_code>
                </doca>
              </addresses>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
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

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)1000+LED+AER"
PPD+REPIN+A:Y+xxxxxxxxxx:xxxxxxxxxx+ADULT+REPIN:INFANT"
UAP"
ADD+R+703:ADDRESS:CITY::REGION:USA:112233+700:RESIDENCE ADDRESS:RESIDENCE CITY::RESIDENCE REGION:BLR:001122"
UAP++IN"
ADD+R+701::MOSCOW:::RUS:127650"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+150217:0745+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)+LED+AER++T"
RAD+U+O"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
PRD+Y"
PFD+001A+:Y+001:002"
PSI++TKNE::29824018416891+TKNE::INF29824018416121+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+FOID::::::FOID PPZB400522509+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+TKNE::::::TKNE HK1 2982401841689/1+INFT::::::INFT HK1 01JAN17 REPIN/INFANT"
PAP+A:REPIN:ADULT:760501:::RUS++P:987654321::::491231:::::::REPIN:ADULT:PETROVICH"
PAP+IN:REPIN:INFANT:160501:::RUS++P:123456789::::491231:::::::REPIN:INFANT:IVANOVICH"
ADD++703:ADDRESS:CITY::REGION:USA:112233+700:RESIDENCE ADDRESS:RESIDENCE CITY::RESIDENCE REGION:BLR:001122"
UNT+10+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>ADULT</name>
            <pers_type>ВЗ</pers_type>
            <refuse/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no>1A</seat_no>
            <reg_no>001</reg_no>
            <seat_type/>
            <seats>1</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841689</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
              <rem>
                <rem_code>INFT</rem_code>
                <rem_text>INFT HK1 01JAN17 REPIN/INFANT</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <document>
              <type>P</type>
              <issue_country>RUS</issue_country>
              <no>7774441110</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>ADULT</first_name>
            </document>
            <iatci_pax_id>0013949613</iatci_pax_id>
            <iatci_parent_pax_id/>
            <doco>
              <birth_place/>
              <type>V</type>
              <no>88888888</no>
              <issue_place>MOSCOW</issue_place>
              <issue_date>01.01.2011 00:00:00</issue_date>
              <expiry_date>02.01.2031 00:00:00</expiry_date>
              <applic_country>RUS</applic_country>
            </doco>
            <bag_pool_num/>
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
          </pax>
          <pax>
            <pax_id>-2</pax_id>
            <surname>REPIN</surname>
            <name>INFANT</name>
            <pers_type>РМ</pers_type>
            <refuse/>
            <bag_pool_num/>
            <tid>0</tid>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <seat_no/>
            <reg_no>002</reg_no>
            <seat_type/>
            <seats>0</seats>
            <subclass>Э</subclass>
            <ticket_no>2982401841612</ticket_no>
            <coupon_no>1</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <rems>
              <rem>
                <rem_code>DOCS</rem_code>
                <rem_text>DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN</rem_text>
              </rem>
              <rem>
                <rem_code>FOID</rem_code>
                <rem_text>FOID PPZB400522509</rem_text>
              </rem>
              <rem>
                <rem_code>PSPT</rem_code>
                <rem_text>PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M</rem_text>
              </rem>
              <rem>
                <rem_code>INFT</rem_code>
                <rem_text>INFT HK1 01JAN17 REPIN/INFANT</rem_text>
              </rem>
            </rems>
            <fqt_rems/>
            <doco>
              <type>V</type>
              <no>13452</no>
              <issue_place>MINSK</issue_place>
              <applic_country>ESP</applic_country>
              <issue_date>20.03.2009 00:00:00</issue_date>
              <expiry_date>20.03.2019 00:00:00</expiry_date>
            </doco>
            <iatci_pax_id>xxxxxxxxxx</iatci_pax_id>
            <iatci_parent_pax_id>-1</iatci_parent_pax_id>
            <document>
              <type>P</type>
              <no>123456789</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.2016 00:00:00</birth_date>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
              <surname>REPIN</surname>
              <first_name>INFANT</first_name>
              <second_name>IVANOVICH</second_name>
            </document>
            <addresses>
              <doca>
                <type>B</type>
                <country>RUS</country>
                <city>MOSCOW</city>
                <postal_code>127650</postal_code>
              </doca>
            </addresses>
          </pax>
        </passengers>


$(set tid_new $(get_single_tid $(get point_dep) REPIN ADULT))
$(set adult_tid $(get_single_pax_tid $(get point_dep) REPIN ADULT))
$(set infant_tid $(get_single_pax_tid $(get point_dep) REPIN INFANT))

# отмена

!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(get point_dep)</point_dep>
          <point_arv>$(get point_arv)</point_arv>
          <airp_dep>ДМД</airp_dep>
          <airp_arv>ПЛК</airp_arv>
          <class>Э</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(get tid_new)</tid>
          <passengers>
            <pax>
              <pax_id>$(get pax_1_id)</pax_id>
              <surname>REPIN</surname>
              <name>ADULT</name>
              <pers_type>ВЗ</pers_type>
              <refuse>А</refuse>
              <ticket_no>2982401841689</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>REPIN</surname>
                <first_name>ADULT</first_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>$(get adult_tid)</tid>
            </pax>
            <pax>
              <pax_id>$(get pax_2_id)</pax_id>
              <surname>REPIN</surname>
              <name>INFANT</name>
              <pers_type>РМ</pers_type>
              <refuse>А</refuse>
              <ticket_no>2982401841612</ticket_no>
              <coupon_no>1</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <issue_country>RUS</issue_country>
                <no>7774441110</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <gender>M</gender>
                <surname>REPIN</surname>
                <first_name>INFANT</first_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>$(get infant_tid)</tid>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
        <segment>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <airp_dep>ПЛК</airp_dep>
          <airp_arv>СОЧ</airp_arv>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>REPIN</surname>
              <name>ADULT</name>
              <pers_type>ВЗ</pers_type>
              <refuse>А</refuse>
              <ticket_no>2982401841689</ticket_no>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>1</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
            </pax>
            <pax>
              <pax_id>-2</pax_id>
              <surname>REPIN</surname>
              <name>INFANT</name>
              <pers_type>РМ</pers_type>
              <refuse>А</refuse>
              <ticket_no>2982401841612</ticket_no>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>1</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>Э</subclass>
              <tid>0</tid>
            </pax>
          </passengers>
          <paid_bag_emd/>
        </segment>
      </segments>
      <hall>1</hall>
      <bag_refuse/>
      <value_bags/>
      <bags/>
      <tags pr_print='1'/>
      <unaccomps/>
    </TCkinSavePax>
  </query>
</term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A:Y+0013949613:0013949614+ADULT+REPIN:INFANT"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"



%%
#########################################################################################
# №23 DCRCKA без TKNE
###


$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_dcs С7 TA OA)
$(init_eds ЮТ UTET UTDC)


$(PREPARE_FLIGHT_3 ЮТ 103 ДМД ПЛК С7 1027 ПЛК СОЧ REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN))

$(deny_ets_interactive ЮТ 103 ДМД)

$(OPEN_CHECKIN $(get point_dep))
$(CHECK_SEARCH_PAX $(get point_dep) ЮТ 103 ДМД ПЛК REPIN IVAN К)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) С7 1027 ПЛК СОЧ REPIN IVAN ВЗ)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) ЮТ 103 ДМД ПЛК
                                                           С7 1027 ПЛК СОЧ
                                                           REPIN IVAN
                                                           2986120030297 ВЗ)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A:N++IVAN"
PRD+Y"
PSD++007A"
PBD+0"
PSI++FOID::::::FOID PP7774441110+FQTV:S7:7788990011"
UNT+9+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
WAD+1:65+1:193"
FSD+1255+1+GATE++N"
PPD+KROTOVSA+M:N+DA79B93001+DINA"
PRD+Y+OK+++FXDIJI"
PFD+004C+N:Y:::LED:AER:733:Y+00001+Y++PLT"
WAD+1:258:API OK"
UNT+10+8180"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

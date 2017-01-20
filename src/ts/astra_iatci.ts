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
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+��:���++++Y+::RU+������"
TKT+$(tick_no)"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+091030:0529+$(last_edifact_ref)0001+++T"
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

$(KICK_IN)

}
) #end-of-macro

$(defmacro CHECK_ADV_TRIPS_LIST
    point_dep
    airl
    flt
    airp_dep
{
!! capture=on
{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='trips' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <GetAdvTripList>
       <date>$(date_format %d.%m.%Y +0) 00:00:00</date>
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
    <date>$(date_format %d.%m.%Y +0) 00:00:00</date>
    <trips>
      <trip>
        <point_id>$(point_dep)</point_id>
        <name>$(airl)$(flt)</name>
        <airp>$(airp_dep)</airp>
        <name_sort_order>0</name_sort_order>
      </trip>
    </trips>

}) #end-of-macro


$(defmacro CHECK_FLIGHT
    point_dep
    airl
    flt
    airp_dep
    airp_arv
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='trips' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetTripInfo>
      <point_id>$(point_dep)</point_id>
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
      <point_id>$(point_dep)</point_id>
      <tripheader>
        <point_id>$(point_dep)</point_id>
        <flight>$(airl)$(flt)...
        <airline>$(airl)</airline>
        <aircode>...
        <flt_no>$(flt)</flt_no>
        <suffix/>
        <airp>$(airp_dep)</airp>
        <scd_out_local>$(date_format %d.%m.%Y +0) 10:00:00</scd_out_local>
        <scd_out>$(date_format %d.%m.%Y +0) 10:00:00</scd_out>
        <real_out>10:00</real_out>
        <act_out/>
        <craft>��5</craft>
        <bort/>
        <park/>
        <classes>...
        <route>$(airp_dep)-$(airp_arv)</route>
        <places>$(airp_dep)-$(airp_arv)</places>
        <trip_type>�</trip_type>
        <litera/>
        <remark/>
        <pr_tranzit>0</pr_tranzit>
        <trip>$(airl)$(flt)...
        <status>���������</status>
        <stage_time>09:20</stage_time>
        <ckin_stage>20</ckin_stage>
        <tranzitable>0</tranzitable>
        <pr_tranz_reg>0</pr_tranz_reg>
        <pr_etstatus>0</pr_etstatus>
        <pr_etl_only>0</pr_etl_only>
        <pr_no_ticket_check>0</pr_no_ticket_check>

}) #end-of-macro

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
            <subclass>�</subclass>
            <class>�</class>
            <passengers>
              <pax>
                <pax_id>...
                <surname>$(surname)</surname>
                <name>$(name)</name>
                <rems/>
                <fqt_rems/>
              </pax>
            </passengers>
            <transfer>
              <segment>
                <num>1</num>
                <airline>�7</airline>
                <flt_no>1027</flt_no>
                <local_date>...
                <airp_dep>���</airp_dep>
                <airp_arv>���</airp_arv>
                <subclass>Y</subclass>
                <trfer_permit>0</trfer_permit>
              </segment>
            </transfer>
            <pnr_addrs>
              <pnr_addr>
                <airline>��</airline>
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

}) #end-of-macro


$(defmacro CHECK_DCS_ADDR_SET
{
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
        <title>���� ��⥬ ॣ����樨</title>

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
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(point_dep)</point_dep>
      <point_arv>$(point_arv)</point_arv>
      <airp_dep>���</airp_dep>
      <airp_arv>���</airp_arv>
      <class>�</class>
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
          <pers_type>��</pers_type>
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
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CheckTCkinRoute>
      <point_dep>$(point_dep)</point_dep>
      <point_arv>$(point_arv)</point_arv>
      <airp_dep>���</airp_dep>
      <airp_arv>���</airp_arv>
      <class>�</class>
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
          <pers_type>��</pers_type>
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
                    <subclass>�</subclass>
                    <class>�</class>
                    <passengers>
                      <pax>
                        <pax_id>-1</pax_id>
                        <surname>$(surname)</surname>
                        <name>$(name)</name>
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
        <class_code>�</class_code>
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
          <class>�</class>
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
              <pers_type>��</pers_type>
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
              <subclass>�</subclass>
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
          <class>�</class>
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
              <pers_type>��</pers_type>
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
              <subclass>�</subclass>
              <bag_pool_num/>
              <rems>
                <rem>
                  <rem_code>FOID</rem_code>
                  <rem_text>FOID PP7774441110</rem_text>
                </rem>
              </rems>
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
    tickno2
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
          <class>�</class>
          <grp_id>$(get grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>��</pers_type>
              <refuse>�</refuse>
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
              <subclass>�</subclass>
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
              <pers_type>��</pers_type>
              <refuse>�</refuse>
              <ticket_no>42161200302972</ticket_no>
              <coupon_no/>
              <ticket_rem/>
              <ticket_confirm>1</ticket_confirm>
              <document/>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>�</subclass>
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
          <class>�</class>
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
          <class>�</class>
          <grp_id>-1</grp_id>
          <tid>0</tid>
          <passengers>
            <pax>
              <pax_id>-1</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>��</pers_type>
              <refuse/>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>$(cpnno)</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                  <type>P</type>
                  <no>99999999999</no>
                  <nationality>RUS</nationality>
                  <birth_date>01.05.1976 00:00:00</birth_date>
                  <expiry_date>31.12.2049 00:00:00</expiry_date>
                  <surname>$(surname)</surname>
                  <first_name>$(name)</first_name>
                  <second_name>$(second_name)</second_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>�</subclass>
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
    pult=������
{

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+��:���++++Y+::RU+$(pult)"
EQN+1:TD"
TKT+$(tickno):T"
CPN+$(cpnno):$(status)"
TVL+$(ddmmyy)+���+���+��+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

# �⢥� �� ���
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
          <class>�</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(tid)</tid>
          <passengers>
            <pax>
              <pax_id>$(pax_id)</pax_id>
              <surname>$(surname)</surname>
              <name>$(name)</name>
              <pers_type>��</pers_type>
              <refuse/>
              <ticket_no>$(tickno)</ticket_no>
              <coupon_no>$(cpnno)</coupon_no>
              <ticket_rem>TKNE</ticket_rem>
              <ticket_confirm>1</ticket_confirm>
              <document>
                <type>P</type>
                <no>99999999999</no>
                <nationality>RUS</nationality>
                <birth_date>01.05.1976 00:00:00</birth_date>
                <expiry_date>31.12.2049 00:00:00</expiry_date>
                <surname>$(surname)</surname>
                <first_name>$(name)</first_name>
              </document>
              <doco/>
              <addresses/>
              <bag_pool_num/>
              <subclass>�</subclass>
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
          <class>�</class>
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
          <class>�</class>
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
          <class>�</class>
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
          <class>�</class>
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
          <class>�</class>
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
# �1 ��ࢨ筠� ॣ������
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(CHECK_FLIGHT $(get point_dep) �� 103 ��� ���)
$(CHECK_SEARCH_PAX $(get point_dep) �� 103 ��� ��� REPIN IVAN �)
$(CHECK_DCS_ADDR_SET)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)
$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�+1"
PSI+++TKNE::29861200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>��103 ���</flight>
          <airline>��</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>$(get point_arv)</point_id>
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>�����-��������� (���)</target_view>
              <check_doc_info/>
              <check_doco_info/>
              <check_tkn_info/>
            </airp>
          </airps>
          <classes>
            <class>
              <code>�</code>
              <class_view>������</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>1</id>
              <name>��� 1</name>
            </hall>
            <hall>
              <id>0</id>
              <name>��.</name>
            </hall>
            <hall>
              <id>1141</id>
              <name>VIP</name>
            </hall>
            <hall>
              <id>1439</id>
              <name>���. ����.</name>
            </hall>
            <hall>
              <id>39706</id>
              <name>�㯥� ���</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>��</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y +0) 10:00:00</scd>
              <airp_dep>���</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>...
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>���</airp_dep>
        <point_arv>$(get point_arv)</point_arv>
        <airp_arv>���</airp_arv>
        <class>�</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>...
        <city_arv>���</city_arv>
        <mark_flight>
          <airline>��</airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
          <airp_dep>���</airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>$(get pax_id)</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>��</pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>�</subclass>
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
            <ticket_bag_norm>���</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
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
            <class>�</class>
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
          <flight>�71027/$(date_format %d.%m +0) ��� (EDI)</flight>
          <airline>�7</airline>
          <aircode>421</aircode>
          <flt_no>1027</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>���� (���)</target_view>
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
        <airp_dep>���</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>���</airp_arv>
        <class>�</class>
        <status>K</status>
        <bag_refuse/>
        <piece_concept>0</piece_concept>
        <tid>0</tid>
        <city_arv>���</city_arv>
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>��</pers_type>
            <seat_no>xx</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>�</subclass>
            <bag_pool_num/>
            <tid>0</tid>
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
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <rems/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters/>
        <load_residue/>
      </segment>
    </segments>


%%
#########################################################################################
# �2 �⬥�� ॣ����樨
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972"
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
             �� 103 ��� ���
             �7 1027 ��� ���
             REPIN IVAN 2986120030297 42161200302972)

$(ETS_COS_EXCHANGE 2986120030297 1 I)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+X+P"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=4
    <segments/>


%%
#########################################################################################
# �3 ��ᬮ�� ���ଠ樨 �� ���ᠦ��� �� grp_id
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302982"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(LOAD_PAX_BY_GRP_ID $(get point_dep) $(get grp_id))

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+P+O"
PPD+REPIN+A++IVAN"
PFD+2A+:�+2"
PSI+++TKNE::42161200302552+FQTV::::::FQTV S7 55555555555555555"
PAP+:::860310:::RUS++PP:1111111111:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>��103 ���</flight>
          <airline>��</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>...
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>�����-��������� (���)</target_view>
              <check_doc_info/>
              <check_doco_info/>
              <check_tkn_info/>
            </airp>
          </airps>
          <classes>
            <class>
              <code>�</code>
              <class_view>������</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>1</id>
              <name>��� 1</name>
            </hall>
            <hall>
              <id>0</id>
              <name>��.</name>
            </hall>
            <hall>
              <id>1141</id>
              <name>VIP</name>
            </hall>
            <hall>
              <id>1439</id>
              <name>���. ����.</name>
            </hall>
            <hall>
              <id>39706</id>
              <name>�㯥� ���</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>��</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y +0) 10:00:00</scd>
              <airp_dep>���</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>$(get grp_id)</grp_id>
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>���</airp_dep>
        <point_arv>$(get point_arv)</point_arv>
        <airp_arv>���</airp_arv>
        <class>�</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>$(get tid)</tid>
        <city_arv>���</city_arv>
        <mark_flight>
          <airline>��</airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y +0) 00:00:00</scd>
          <airp_dep>���</airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>$(get pax_id)</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>��</pers_type>
            <crew_type/>
            <seat_no>1A</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>�</subclass>
            <bag_pool_num/>
            <tid>$(get tid)</tid>
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
            <ticket_bag_norm>���</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
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
            <class>�</class>
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
          <flight>�71027/$(date_format %d.%m +0) ��� (EDI)</flight>
          <airline>�7</airline>
          <aircode>421</aircode>
          <flt_no>1027</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y +0) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>���� (���)</target_view>
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
        <airp_dep>���</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>���</airp_arv>
        <class>�</class>
        <status>K</status>
        <bag_refuse/>
        <piece_concept>0</piece_concept>
        <tid>0</tid>
        <city_arv>���</city_arv>
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>��</pers_type>
            <seat_no>2A</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>2</reg_no>
            <subclass>�</subclass>
            <bag_pool_num/>
            <tid>0</tid>
            <ticket_no>4216120030255</ticket_no>
            <coupon_no>2</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <document>
              <type>PP</type>
              <issue_country>RUS</issue_country>
              <no>1111111111</no>
              <nationality>RUS</nationality>
              <birth_date>10.03.1986 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
            </document>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <rems>
              <rem>
                <rem_code>FQTV</rem_code>
                <rem_text>FQTV S7 55555555555555555</rem_text>
              </rem>
            </rems>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters/>
        <load_residue/>
      </segment>
    </segments>


%%
#########################################################################################
# �4 ��ᬮ�� ���ଠ樨 �� ���ᠦ��� �� reg_no
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302982"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(LOAD_PAX_BY_REG_NO $(get point_dep) 1)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+P+O"
PPD+REPIN+A++IVAN"
PFD+1A+:�"
PSI+++TKNE::42161200302551"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)


%%
#########################################################################################
# �5 ��ᬮ�� ���ଠ樨 �� ���ᠦ��� �� pax_id
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302982"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))


$(LOAD_PAX_BY_PAX_ID $(get point_dep) $(get pax_id))

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+P+O"
PPD+REPIN+A++IVAN"
PFD+1A+:�"
PSI+++TKNE::42161200302551+TKNE::::::TKNE HK1 4216120030255/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)


%%
#########################################################################################
# �6 ������� �� PLF
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302982"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(LOAD_PAX_BY_GRP_ID $(get point_dep) $(get grp_id))

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)


>> lines=auto
    <kick req_ctxt_id...

!! err=MSG.DCS_CONNECT_ERROR
$(lastRedisplay)


%%
#########################################################################################
# �7 �訡�� �� PLF
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302982"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(LOAD_PAX_BY_GRP_ID $(get point_dep) $(get grp_id))

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+P+F"
ERD+1:193:PASSENGER SURNAME NOT CHECKED-IN"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!! err="PASSENGER SURNAME NOT CHECKED-IN"
$(lastRedisplay)


%%
#########################################################################################
# �8 �訡�� �� CKI
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+F"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...

!! err="UNABLE TO PROCESS - SYSTEM ERROR"
$(lastRedisplay)

$(ETS_COS_EXCHANGE 2986120030297 1 I SYSTEM)

# �� � ����� ��祣� � �� ������ �ந�室��� �� ����.


%%
#########################################################################################
# �9 �訡�� �� CKX
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA IFM1 IFM2)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972"
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
             �� 103 ��� ���
             �7 1027 ��� ���
             REPIN IVAN 2986120030297 42161200302972)

$(ETS_COS_EXCHANGE 2986120030297 1 I)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+X+F"
ERD+1:102:UNABLE TO PROCESS - SYSTEM ERROR"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...


%%
#########################################################################################
# �10 ���������� ������ ॣ����樨 (���㬥��)
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972+OTHS::::::FREE TEXT"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(UPDATE_PAX_DOC $(get point_dep) $(get point_arv) ��� ���
                 $(get grp_id) $(get pax_id) $(get tid) ��� ���
                 REPIN IVAN IVANICH 4216120030297 2)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A++IVAN"
UAP+R+:::760501:::RUS++P:99999999999::::491231:::::::REPIN:IVAN:IVANICH"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+U+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�+22"
PSI+++TKNE::42161200302972"
PAP+:::760501:::RUS++P:99999999999:USA:::491231:M::::::REPIN:IVAN:IVANICH"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN)

>> lines=auto
    <segments>
      <segment>
        <tripheader>
          <flight>��103 ���</flight>
          <airline>��</airline>
          <aircode>298</aircode>
          <flt_no>103</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
          <pr_auto_pt_print>0</pr_auto_pt_print>
          <pr_auto_pt_print_reseat>0</pr_auto_pt_print_reseat>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>...
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>�����-��������� (���)</target_view>
              <check_doc_info/>
              <check_doco_info/>
              <check_tkn_info/>
            </airp>
          </airps>
          <classes>
            <class>
              <code>�</code>
              <class_view>������</class_view>
              <cfg>...
            </class>
          </classes>
          <gates/>
          <halls>
            <hall>
              <id>...
              <name>��� 1</name>
            </hall>
            <hall>
              <id>...
              <name>��.</name>
            </hall>
            <hall>
              <id>...
              <name>VIP</name>
            </hall>
            <hall>
              <id>...
              <name>���. ����.</name>
            </hall>
            <hall>
              <id>...
              <name>�㯥� ���</name>
            </hall>
          </halls>
          <mark_flights>
            <flight>
              <airline>��</airline>
              <flt_no>103</flt_no>
              <suffix/>
              <scd>$(date_format %d.%m.%Y) 10:00:00</scd>
              <airp_dep>���</airp_dep>
              <pr_mark_norms>0</pr_mark_norms>
            </flight>
          </mark_flights>
        </tripdata>
        <grp_id>$(get grp_id)</grp_id>
        <point_dep>$(get point_dep)</point_dep>
        <airp_dep>���</airp_dep>
        <point_arv>...
        <airp_arv>���</airp_arv>
        <class>�</class>
        <status>K</status>
        <bag_refuse/>
        <bag_types_id>0</bag_types_id>
        <piece_concept>0</piece_concept>
        <tid>...
        <city_arv>���</city_arv>
        <mark_flight>
          <airline>��</airline>
          <flt_no>103</flt_no>
          <suffix/>
          <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
          <airp_dep>���</airp_dep>
          <pr_mark_norms>0</pr_mark_norms>
        </mark_flight>
        <passengers>
          <pax>
            <pax_id>...
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>��</pers_type>
            <crew_type/>
            <seat_no>...
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>1</reg_no>
            <subclass>�</subclass>
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
            <ticket_bag_norm>���</ticket_bag_norm>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
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
            <point_arv>...
            <class>�</class>
            <noshow>1</noshow>
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
          <flight>�71027/$(date_format %d.%m) ��� (EDI)</flight>
          <airline>�7</airline>
          <aircode>421</aircode>
          <flt_no>1027</flt_no>
          <suffix/>
          <airp>���</airp>
          <scd_out_local>$(date_format %d.%m.%Y) 10:00:00</scd_out_local>
          <pr_etl_only>0</pr_etl_only>
          <pr_etstatus>0</pr_etstatus>
          <pr_no_ticket_check>0</pr_no_ticket_check>
        </tripheader>
        <tripdata>
          <airps>
            <airp>
              <point_id>-1</point_id>
              <airp_code>���</airp_code>
              <city_code>���</city_code>
              <target_view>���� (���)</target_view>
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
        <airp_dep>���</airp_dep>
        <point_arv>-1</point_arv>
        <airp_arv>���</airp_arv>
        <class>�</class>
        <status>K</status>
        <bag_refuse/>
        <piece_concept>0</piece_concept>
        <tid>0</tid>
        <city_arv>���</city_arv>
        <passengers>
          <pax>
            <pax_id>-1</pax_id>
            <surname>REPIN</surname>
            <name>IVAN</name>
            <pers_type>��</pers_type>
            <seat_no>xx</seat_no>
            <seat_type/>
            <seats>1</seats>
            <refuse/>
            <reg_no>22</reg_no>
            <subclass>�</subclass>
            <bag_pool_num/>
            <tid>0</tid>
            <ticket_no>4216120030297</ticket_no>
            <coupon_no>2</coupon_no>
            <ticket_rem>TKNE</ticket_rem>
            <ticket_confirm>1</ticket_confirm>
            <document>
              <type>P</type>
              <issue_country>USA</issue_country>
              <no>99999999999</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <gender>M</gender>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
              <second_name>IVANICH</second_name>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
            </document>
            <pr_norec>0</pr_norec>
            <pr_bp_print>0</pr_bp_print>
            <pr_bi_print>0</pr_bi_print>
            <rems/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters/>
        <load_residue/>
      </segment>
    </segments>

%%
#########################################################################################
# �11 ���������� ������ ॣ����樨 ��� IATCI
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS::::M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(UPDATE_PAX_DOC_NON_IATCI $(get point_dep) $(get point_arv) ��� ���
                           $(get grp_id) $(get pax_id) $(get tid) ��� ���
                           REPIN IVAN 2986120030297 1)

>> lines=auto
            <document>
              <type>P</type>
              <no>99999999999</no>
              <nationality>RUS</nationality>
              <birth_date>01.05.1976 00:00:00</birth_date>
              <expiry_date>31.12.2049 00:00:00</expiry_date>
              <surname>REPIN</surname>
              <first_name>IVAN</first_name>
            </document>


%%
#########################################################################################
# �12 ���������� ������ ॣ����樨 (����ન)
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"



>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(UPDATE_PAX_REMS $(get point_dep) $(get point_arv) ��� ���
                  $(get grp_id) $(get pax_id) $(get tid) ��� ���
                  REPIN IVAN IVANICH 4216120030297 2)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKU:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A++IVAN"
USI++C:FOID::::::FOID PP7774441110+A:FOID::::::FOID PP7774449999+A:OTHS::::::OTHS HK1 DOCS/7777771110/PS"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+U+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972+FOID::::::FOID PP7774449999+OTHS::::::OTHS HK1 DOCS/7777771110/PS"
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


%%
#########################################################################################
# �13 ���������� ������ ॣ����樨 (����ન ������)
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))


$(UPDATE_PAX_REMS_WITH_LONG $(get point_dep) $(get point_arv) ��� ���
                            $(get grp_id) $(get pax_id) $(get tid) ��� ���
                            REPIN IVAN IVANICH 4216120030297 2)


%%
#########################################################################################
# �14 ����� ��
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(prepare_bp_printing �� 103 ���)
$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+7A+:�+32"
PSI+++TKNE::42161200302972+FOID::::::FOID PP7774441110"
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
UNH+1+DCQBPR:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
PPD+REPIN+A++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+B+O"
PPD+REPIN+A++IVAN"
PFD+7A+:�+32"
PSI+++TKNE::42161200302972+FOID::::::FOID PP7774441110"
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
            <prn_form hex='0'>0,20,0,ACT:             '$(date_format %d.%m.%Y) 10:00:00                     ' 0,25,0,AGENT:           'PIKE                                    ' 0,30,0,AIRLINE:         '��                                      ' 0,35,0,AIRLINE_NAME:    '��� ������������ �����                  ' 0,40,0,AIRLINE_SHORT:   '�����                                   ' 0,45,0,AIRP_ARV:        '���                                     ' 0,50,0,AIRP_ARV_NAME:   '�������                                 ' 0,55,0,AIRP_DEP:        '���                                     ' 0,60,0,AIRP_DEP_NAME:   '����������                              ' 0,65,0,BAG_AMOUNT:      '0                                       ' 0,70,0,BAGGAGE:         '                                        ' 0,75,0,BAG_WEIGHT:      '0                                       ' 0,80,0,BCBP_M_2:        'M1REPIN/IVAN          E0840Z6 DMELEDUT 0103 xxxY001A0001 128&gt;2180OO    B                00000000$(get pax_id)' 0,85,0,BRD_FROM:        '$(date_format %d.%m.%Y) 09:15:00                     ' 0,90,0,BRD_TO:          '$(date_format %d.%m.%Y) 09:35:00                     ' 0,95,0,CHD:             '                                        ' 0,100,0,CITY_ARV_NAME:  '�����-���������                         ' 0,105,0,CITY_DEP_NAME:  '������                                  ' 0,110,0,CLASS:          '�                                       ' 0,115,0,CLASS_NAME:     '������                                  ' 0,120,0,DESK:           '������                                  ' 0,125,0,DOCUMENT:       '7774441110                              ' 0,130,0,DUPLICATE:      '                                        ' 0,135,0,EST:            '$(date_format %d.%m.%Y) 10:00:00                     ' 0,140,0,ETICKET_NO:     '2986120030297/1                         ' 0,145,0,ETKT:           'ETKT2986120030297/1                     ' 0,150,0,EXCESS:         '0                                       ' 0,155,0,FLT_NO:         '103                                     ' 0,160,0,FQT:            '                                        ' 0,165,0,FULLNAME:       'REPIN IVAN                              ' 0,170,0,FULL_PLACE_ARV: '�����-��������� �������                 ' 0,175,0,FULL_PLACE_DEP: '������ ����������                       ' 0,180,0,GATE:           '1                                       ' 0,185,0,GATES:          '                                        ' 0,190,0,HALL:           '��� 1                                   ' 0,195,0,INF:            '                                        ' 0,200,0,LIST_SEAT_NO:   'xx                                      ' 0,205,0,LONG_ARV:       '�����-���������(���)/ST PETERSBURG(LED) ' 0,210,0,LONG_DEP:       '������(���)/MOSCOW(DME)                 ' 0,215,0,NAME:           'IVAN                                    ' 0,220,0,NO_SMOKE:       'X                                       ' 0,225,0,ONE_SEAT_NO:    'xx                                      ' 0,230,0,PAX_ID:         '000000$(get pax_id)                              ' 0,235,0,PAX_TITLE:      '�-�                                     ' 0,240,0,PLACE_ARV:      '�����-���������(���)                    ' 0,245,0,PLACE_DEP:      '������(���)                             ' 0,250,0,PNR:            '0840Z6                                  ' 0,255,0,REG_NO:         '001                                     ' 0,260,0,REM:            '                                        ' 0,265,0,RK_AMOUNT:      '0                                       ' 0,270,0,RK_WEIGHT:      '0                                       ' 0,275,0,RSTATION:       '                                        ' 0,280,0,SCD:            '$(date_format %d.%m.%Y) 10:00:00                     ' 0,285,0,SEAT_NO:        'xx                                      ' 0,290,0,STR_SEAT_NO:    'xx                                      ' 0,295,0,SUBCLS:         '�                                       ' 0,300,0,SURNAME:        'REPIN                                   ' 0,305,0,TAGS:           '                                        ' 0,310,0,TEST_SERVER:    '                                        ' 0,315,0,TIME_PRINT:     '$(date_format %d.%m.%Y) xx:xx:xx                     '</prn_form>
          </pax>
          <pax pax_id...
            <prn_form hex='0'>0,20,0,ACT:             '$(date_format %d.%m.%Y) 10:00:00                     ' 0,25,0,AGENT:           'PIKE                                    ' 0,30,0,AIRLINE:         '�7                                      ' 0,35,0,AIRLINE_NAME:    '������������ ������                     ' 0,40,0,AIRLINE_SHORT:   '������                                  ' 0,45,0,AIRP_ARV:        '���                                     ' 0,50,0,AIRP_ARV_NAME:   '����                                    ' 0,55,0,AIRP_DEP:        '���                                     ' 0,60,0,AIRP_DEP_NAME:   '�������                                 ' 0,65,0,BAG_AMOUNT:      '0                                       ' 0,70,0,BAGGAGE:         '                                        ' 0,75,0,BAG_WEIGHT:      '0                                       ' 0,80,0,BCBP_M_2:        '' 0,85,0,BRD_FROM:        '$(date_format %d.%m.%Y) xx:xx:xx                     ' 0,90,0,BRD_TO:          '$(date_format %d.%m.%Y) xx:xx:xx                     ' 0,95,0,CHD:             '                                        ' 0,100,0,CITY_ARV_NAME:  '����                                    ' 0,105,0,CITY_DEP_NAME:  '�����-���������                         ' 0,110,0,CLASS:          '                                        ' 0,115,0,CLASS_NAME:     '                                        ' 0,120,0,DESK:           '������                                  ' 0,125,0,DOCUMENT:       '5408123432                              ' 0,130,0,DUPLICATE:      '                                        ' 0,135,0,EST:            '$(date_format %d.%m.%Y) 10:00:00                     ' 0,140,0,ETICKET_NO:     '4216120030297/2                         ' 0,145,0,ETKT:           'ETKT4216120030297/2                     ' 0,150,0,EXCESS:         '0                                       ' 0,155,0,FLT_NO:         '1027                                    ' 0,160,0,FQT:            '                                        ' 0,165,0,FULLNAME:       'REPIN IVAN                              ' 0,170,0,FULL_PLACE_ARV: '����                                    ' 0,175,0,FULL_PLACE_DEP: '�����-��������� �������                 ' 0,180,0,GATE:           '                                        ' 0,185,0,GATES:          '                                        ' 0,190,0,HALL:           '                                        ' 0,195,0,INF:            '                                        ' 0,200,0,LIST_SEAT_NO:   '7A                                      ' 0,205,0,LONG_ARV:       '����(���)/SOCHI(AER)                    ' 0,210,0,LONG_DEP:       '�����-���������(���)/ST PETERSBURG(LED) ' 0,215,0,NAME:           'IVAN                                    ' 0,220,0,NO_SMOKE:       'X                                       ' 0,225,0,ONE_SEAT_NO:    '7A                                      ' 0,230,0,PAX_ID:         '2000000000                              ' 0,235,0,PAX_TITLE:      '                                        ' 0,240,0,PLACE_ARV:      '����(���)                               ' 0,245,0,PLACE_DEP:      '�����-���������(���)                    ' 0,250,0,PNR:            '                                        ' 0,255,0,REG_NO:         '032                                     ' 0,260,0,REM:            '                                        ' 0,265,0,RK_AMOUNT:      '0                                       ' 0,270,0,RK_WEIGHT:      '0                                       ' 0,275,0,RSTATION:       '                                        ' 0,280,0,SCD:            '$(date_format %d.%m.%Y) 10:00:00                     ' 0,285,0,SEAT_NO:        '7A                                      ' 0,290,0,STR_SEAT_NO:    '7A                                      ' 0,295,0,SUBCLS:         '�                                       ' 0,300,0,SURNAME:        'REPIN                                   ' 0,305,0,TAGS:           '                                        ' 0,310,0,TEST_SERVER:    '                                        ' 0,315,0,TIME_PRINT:     '$(date_format %d.%m.%Y) xx:xx:xx                     '</prn_form>
          </pax>
        </passengers>
      </printBP>
    </data>


%%
#########################################################################################
# �15 ���� ����
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(prepare_bp_printing �� 103 ���)
$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+3B+:�"
PSI+++TKNE::42161200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(KICK_IN_SILENT)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))

$(dump_table GRP_IATCI_XML)

!! err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='salonform' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <Show>
      <trip_id>-$(get grp_id)1</trip_id>
    </Show>
  </query>
</term>}

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQSMF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRSMF:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+S+O"
EQD++++++D09"
CBD+F+3:6+++F++A:W+B:A+E:A+F:W"
ROD+3++A::K+B::K+E::K+F::K"
ROD+6++A+B:O+E+F"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN)

>> lines=auto
      <salons RFISCMode='0'>
        <filterRoutes>
          <point_dep>-1</point_dep>
          <point_arv>-1</point_arv>
          <items>
            <item>
              <point_id>-1</point_id>
              <airp>���</airp>
            </item>
            <item>
              <point_id>-1</point_id>
              <airp>���</airp>
            </item>
          </items>
        </filterRoutes>
        <placelist num='0' xcount='5' ycount='4'>
          <place>
            <x>0</x>
            <y>0</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>A</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>1</x>
            <y>0</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>B</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>3</x>
            <y>0</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>E</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>4</x>
            <y>0</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>F</xname>
            <yname>3</yname>
          </place>
          <place>
            <x>0</x>
            <y>1</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>A</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>1</x>
            <y>1</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>B</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>3</x>
            <y>1</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>E</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>4</x>
            <y>1</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>F</xname>
            <yname>4</yname>
          </place>
          <place>
            <x>0</x>
            <y>2</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>A</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>1</x>
            <y>2</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>B</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>3</x>
            <y>2</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>E</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>4</x>
            <y>2</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>F</xname>
            <yname>5</yname>
          </place>
          <place>
            <x>0</x>
            <y>3</y>
            <elem_type>�</elem_type>
            <class>F</class>
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
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>B</xname>
            <yname>6</yname>
          </place>
          <place>
            <x>3</x>
            <y>3</y>
            <elem_type>�</elem_type>
            <class>F</class>
            <xname>E</xname>
            <yname>6</yname>
          </place>
          <place>
            <x>4</x>
            <y>3</y>
            <elem_type>�</elem_type>
            <class>F</class>
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
      <trip_id>-$(get grp_id)1</trip_id>
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
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A++IVAN"
USD++6A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+U+O"
PPD+REPIN+A++IVAN"
PFD+6A+:�"
PSI+++TKNE::42161200302972+FOID::::::FOID PP7774441110"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQSMF:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRSMF:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
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
# �16 ���� ���� �� ॣ����樨
###

$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(prepare_bp_printing �� 103 ���)
$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)


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
# �17 �訡�� � �⢥� �� ॣ������ � ࠡ�� ��� ���ࠪ⨢�
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(deny_ets_interactive �� 103 ���)

$(OPEN_CHECKIN $(get point_dep))
#!! $(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
#!! $(CHECK_FLIGHT $(get point_dep) �� 103 ��� ���)
$(CHECK_SEARCH_PAX $(get point_dep) �� 103 ��� ��� REPIN IVAN �)
$(CHECK_DCS_ADDR_SET)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
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
# �18 ������� �� ॣ������ - ������ ���� IFM/del
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA REMIFM OURIFM)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(CHECK_FLIGHT $(get point_dep) �� 103 ��� ���)
$(CHECK_SEARCH_PAX $(get point_dep) �� 103 ��� ��� REPIN IVAN �)
$(CHECK_DCS_ADDR_SET)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)

$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...

!! err="MSG.DCS_CONNECT_ERROR"
$(lastRedisplay)


# �⪠� ᬥ�� ����� � ���
>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+��:���++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:I"
TVL+$(ddmmyy)+���+���+��+103: ++1"
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
# �18 ������� � �訡�� �� �⬥�� ॣ����樨 - ������ ���� IFM/del
###


$(init)
$(init_jxt_pult ������)
$(login)
$(init_dcs �7 TA OA REMIFM OURIFM)
$(init_eds �� UTET UTDC)


$(PREPARE_FLIGHT_3 �� 103 ��� ��� �7 1027 ��� ��� REPIN IVAN)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set pax_id $(get_single_pax_id $(get point_dep) REPIN IVAN K))

$(OPEN_CHECKIN $(get point_dep))
$(SAVE_ET_DISP $(get point_dep) 2986120030297)
$(CHECK_ADV_TRIPS_LIST $(get point_dep) �� 103 ���)
$(CHECK_FLIGHT $(get point_dep) �� 103 ��� ���)
$(CHECK_SEARCH_PAX $(get point_dep) �� 103 ��� ��� REPIN IVAN �)
$(CHECK_DCS_ADDR_SET)
$(CHECK_TCKIN_ROUTE_1 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(CHECK_TCKIN_ROUTE_2 $(get point_dep) $(get point_arv) �7 1027 ��� ��� REPIN IVAN)
$(SAVE_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 103 ��� ���
                                                           �7 1027 ��� ���
                                                           REPIN IVAN
                                                           2986120030297)


$(ETS_COS_EXCHANGE 2986120030297 1 CK)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER++UT+103+$(yymmdd)++DME+LED"
PPD+REPIN+A++IVAN"
PSD++7A"
PSI+++FOID::::::FOID PP7774441110"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+S7+1027+$(yymmdd)1000+LED+AER++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI+++TKNE::42161200302972"
PAP+:::860310:::RUS++PP:5408123432:RUS:::491231:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

# ᭠砫� ⠩����

$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
             �� 103 ��� ���
             �7 1027 ��� ���
             REPIN IVAN 2986120030297 42161200302972)

$(ETS_COS_EXCHANGE 2986120030297 1 I)

$(KICK_IN_SILENT)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...

!! err="MSG.DCS_CONNECT_ERROR"
$(lastRedisplay)


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+��:���++++Y+::RU+SYSTEM"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+���+���+��+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

>>
REMIFM
.OURIFM
IFM
-S71027/$(ddmon +0 en) LEDAER
DEL
1REPIN/IVAN
ENDIFM



# ��⮬ �訡��

$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
             �� 103 ��� ���
             �7 1027 ��� ��� 
             REPIN IVAN 2986120030297 42161200302972)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKX:94:1:IA+$(last_edifact_ref)"
LOR+UT:DME"
FDQ+S7+1027+$(yymmdd)+LED+AER"
PPD+REPIN+A++IVAN"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+X+F"
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
-S71027/$(ddmon +0 en) LEDAER
DEL
1REPIN/IVAN
ENDIFM

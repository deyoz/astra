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

>> lines=auto
    <kick req_ctxt_id...


!!
$(lastRedisplay)

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
        <pr_mixed_norms>0</pr_mixed_norms>

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
          <classes>
            <class>
              <code>�</code>
              <class_view>������</class_view>
              <cfg>-1</cfg>
            </class>
          </classes>
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

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::29861200302972"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

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
        <point_dep>-1</point_dep>
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
            <rems/>
          </pax>
        </passengers>
        <paid_bag_emd/>
        <tripcounters/>
        <load_residue/>
      </segment>
    </segments>
    <transfer/>
    <value_bags/>
    <bags/>
    <tags/>
    <paid_bags>
      <paid_bag>
        <bag_type/>
        <weight>0</weight>
        <rate_id/>
        <rate/>
        <rate_cur/>
        <rate_trfer/>
      </paid_bag>
    </paid_bags>
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
  </answer>
</term>


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

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302972"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQCKX:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
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


>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302982"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQPLF:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
SPD+REPIN:IVAN::1"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+P+O"
PPD+REPIN+A++IVAN"
PFD+2A+:�"
PSI++TKNE::42161200302552+FQTV::::::FQTV S7 55555555555555555"
PAP+:::100386:::RUS++PP:1111111111:RUS:::311249:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...


!! capture=on
$(lastRedisplay)

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
              <cfg>1</cfg>
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
            <noshow>0</noshow>
            <trnoshow>0</trnoshow>
            <show>1</show>
            <free_ok>0</free_ok>
            <free_goshow>0</free_goshow>
            <nooccupy>0</nooccupy>
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
        <point_dep>-1</point_dep>
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
            <reg_no>1</reg_no>
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
    <transfer/>
    <value_bags/>
    <bags/>
    <tags/>
    <paid_bags>
      <paid_bag>
        <bag_type/>
        <weight>0</weight>
        <rate_id/>
        <rate/>
        <rate_cur/>
        <rate_trfer/>
      </paid_bag>
    </paid_bags>
  </answer>
</term>


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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302982"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))

$(LOAD_PAX_BY_REG_NO $(get point_dep) 1)

>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQPLF:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
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
PSI++TKNE::42161200302551"
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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302982"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQPLF:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
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
PSI++TKNE::42161200302551+TKNE::::::TKNE HK1 4216120030255/1+DOCS::::::DOCS HK1/P/TJK/400522509/TJK/24JUL85/M/05FEB25/REPIN/IVAN+PSPT::::::PSPT HK1 ZB400522509/TJK/24JUL85/REPIN/IVAN/M+FOID::::::FOID PPZB400522509"
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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302982"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQPLF:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302982"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQPLF:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302972"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQCKX:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
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

!! err="UNABLE TO PROCESS - SYSTEM ERROR"
$(lastRedisplay)


$(ETS_COS_EXCHANGE 2986120030297 1 CK SYSTEM)

# �� � ����� ��祣� � �� ������ �ந�室��� �� ����.


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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302972+OTHS::::::FREE TEXT"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQCKU:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
PPD+REPIN+A++IVAN"
UAP+R+:::010576:::RUS++P:99999999999::::311249:::::::REPIN:IVAN:IVANICH"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+U+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302972"
PAP+:::010576:::RUS++P:99999999999:USA:::311249:M::::::REPIN:IVAN:IVANICH"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)


>> lines=auto
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
            <rems/>
          </pax>

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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302972"
PAP+:::100386:::RUS++PP:5408123432:RUS::::M::::::REPIN:IVAN"
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
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302972+FOID::::::FOID PP7774441110"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
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
UNH+1+DCQCKU:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���"
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
PSI++TKNE::42161200302972+FOID::::::FOID PP7774449999+OTHS::::::OTHS HK1 DOCS/7777771110/PS"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick req_ctxt_id...

!! capture=on
$(lastRedisplay)

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

>> lines=auto
    <kick req_ctxt_id...

$(lastRedisplay)


>>
UNB+SIRE:1+OA+TA+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+DCQCKI:96:2:IA+$(last_edifact_ref)"
LOR+��:���"
FDQ+�7+1027+$(yymmdd)+���+���++��+103+$(yymmdd)++���+���"
PPD+REPIN+A++IVAN"
PSD++7A"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+TA+OA+151027:1527+$(last_edifact_ref)0001+++T"
UNH+1+DCRCKA:96:2:IA+$(last_edifact_ref)"
FDR+�7+1027+$(yymmdd)1000+���+���++T"
RAD+I+O"
PPD+REPIN+A++IVAN"
PFD+xx+:�"
PSI++TKNE::42161200302972+FOID::::::FOID PP7774441110"
PAP+:::100386:::RUS++PP:5408123432:RUS:::311249:M::::::REPIN:IVAN"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

$(set grp_id $(get_single_grp_id $(get point_dep) REPIN IVAN))
$(set tid $(get_single_tid $(get point_dep) REPIN IVAN))


$(UPDATE_PAX_REMS_WITH_LONG $(get point_dep) $(get point_arv) ��� ���
                            $(get grp_id) $(get pax_id) $(get tid) ��� ���
                            REPIN IVAN IVANICH 4216120030297 2)

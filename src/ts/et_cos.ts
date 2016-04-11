include(ts/macro.ts)

# meta: suite eticket


$(defmacro CHECKIN_PAX
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
            <scd>$(date_format %d.%m.%Y) 00:00:00</scd>
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
) #end-of-macro


$(defmacro OPEN_CHECKIN
{
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

}
) #end-of-macro


$(defmacro SAVE_ET_DISP point_id
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='ETSearchForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SearchETByTickNo>
      <point_id>$(point_id)</point_id>
      <TickNoEdit>2986120030297</TickNoEdit>
    </SearchETByTickNo>
  </query>
</term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:131"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
TKT+2986120030297"
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
TKT+2986120030297:T:1:3"
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

### test 1 - успешная смена статуса
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_ONE_FLIGHT UT DME LED 103)

$(set dep_point_id $(get_dep_point_id ДМД ЮТ 103 $(yymmdd +0)))
$(create_random_trip_comp $(get dep_point_id) Э)

# открываем регистрацию
$(OPEN_CHECKIN)


$(set first_point_id $(last_point_id_spp))
$(set next_point_id $(get_next_trip_point_id $(get first_point_id)))


$(set pax_id $(get_single_pax_id $(get first_point_id) REPIN IVAN K))

$(SAVE_ET_DISP $(last_point_id_spp))


!! err=ignore
$(CHECKIN_PAX)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+160408:0828+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:142+3"
EQN+2:TD"
TKT+2986120030297:T::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...


!! capture=on
$(lastRedisplay)


>> lines=auto
            <ticket_no>2986120030297...

%%
### test 2 - ошибка в ответе на COS-req
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_ONE_FLIGHT UT DME LED 103)

$(set dep_point_id $(get_dep_point_id ДМД ЮТ 103 $(yymmdd +0)))
$(create_random_trip_comp $(get dep_point_id) Э)

# открываем регистрацию
$(OPEN_CHECKIN)

$(set first_point_id $(last_point_id_spp))
$(set next_point_id $(get_next_trip_point_id $(get first_point_id)))


$(set pax_id $(get_single_pax_id $(get first_point_id) REPIN IVAN K))

$(SAVE_ET_DISP $(last_point_id_spp))

!! err=ignore
$(CHECKIN_PAX)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


<<
UNB+SIRE:1+UTET+UTDC+160408:0828+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:142+7"
ERC+401"
UNT+4+1"
UNZ+1+$(last_edifact_ref)0001"


>> lines=auto
    <kick req_ctxt_id...


!! capture=on
$(lastRedisplay)


>> lines=auto
    <ets_error>СЭБ: ОШИБКА 401



%%
### test 3 - таймаут на COS-req
#########################################################################################

$(init)
$(init_jxt_pult МОВРОМ)
$(login)
$(init_eds ЮТ UTET UTDC)

$(PREPARE_ONE_FLIGHT UT DME LED 103)

$(set dep_point_id $(get_dep_point_id ДМД ЮТ 103 $(yymmdd +0)))
$(create_random_trip_comp $(get dep_point_id) Э)

# открываем регистрацию
$(OPEN_CHECKIN)

$(set first_point_id $(last_point_id_spp))
$(set next_point_id $(get_next_trip_point_id $(get first_point_id)))


$(set pax_id $(get_single_pax_id $(get first_point_id) REPIN IVAN K))

$(SAVE_ET_DISP $(last_point_id_spp))

!! err=ignore
$(CHECKIN_PAX)

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:142"
ORG+ЮТ:МОВ++++Y+::RU+МОВРОМ"
EQN+1:TD"
TKT+2986120030297:T"
CPN+1:CK"
TVL+$(ddmmyy)+ДМД+ПЛК+ЮТ+103: ++1"
UNT+8+1"
UNZ+1+$(last_edifact_ref)0001"


$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...

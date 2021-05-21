# meta: suite typeb

include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)

$(init_term)

$(set_user_time_type LocalAirp PIKE)

$(cache PIKE RU TYPEB_ORIGINATORS $(cache_iface_ver TYPEB_ORIGINATORS) ""
  insert addr:QQQQQQQ
         first_date:$(date_format %d.%m.%Y -3h)
         descr:main)

$(set airline UT)
$(set flt_no 280)
$(set craft TU5)
$(set airp_dep DME)
$(set time_dep "$(date_format %d.%m.%Y +1) 07:00")
$(set time_arv "$(date_format %d.%m.%Y +1) 10:00")
$(set airp_arv AER)

$(NEW_SPP_FLIGHT_ONE_LEG $(get airline) $(get flt_no) $(get craft) $(get airp_dep) $(get time_dep) $(get time_arv) $(get airp_arv))

<<
MOWKK1H
.TJMRM1T $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
-$(get airp_arv)000C
1ПУПКИН/ВАСЯ
ENDPART1


$(set point_dep $(last_point_id_spp))

$(dump_table typeb_originators)

!! capture=on err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <CreateTlg>
      <point_id>$(get point_dep)</point_id>
      <tlg_type>PRL</tlg_type>
      <crs/>
      <pr_lat>1</pr_lat>
      <addrs>YYYYYYY </addrs>
    </CreateTlg>
  </query>
</term>

>> lines=auto mode=regex
.*<tlg_id>([0-9]+)</tlg_id>.*

!! capture=on err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetTlgOut>
      <tlg_id>$(capture 1)</tlg_id>
    </GetTlgOut>
  </query>
</term>

>> lines=auto
        <time_create>xx.xx.xxxx xx:xx:xx</time_create>

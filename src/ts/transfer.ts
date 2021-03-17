include(ts/macro.ts)
include(ts/spp/read_trips_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/pnl/btm_ptm.ts)

# meta: suite transfer

$(defmacro TRANSFER_FLT
  trip
  airp_dep
  airp_arv
  subcl
{        <trip>$(trip)</trip>
        <airp>$(airp_arv)</airp>
        <airp_dep>$(airp_dep)</airp_dep>
        <airp_arv>$(airp_arv)</airp_arv>\
$(if $(eq $(subcl) "") {
        <subcl/>} {
        <subcl>$(subcl)</subcl>})})

$(defmacro PAX_TRANSFER_GRP
  bag_amount
  seats
  surname
  name
  surname2
  name2
{          <grp>
            <bag_amount>$(bag_amount)</bag_amount>
            <bag_weight/>
            <rk_weight/>
            <weight_unit/>
            <seats>$(seats)</seats>
            <passengers>
              <pax>
                <surname>$(surname)</surname>\
$(if $(eq $(name) "") "" {
                <name>$(name)</name>})
              </pax>\
$(if $(eq $(surname2) "") "" {
              <pax>
                <surname>$(surname2)</surname>\
$(if $(eq $(name2) "") "" {
                <name>$(name2)</name>})
              </pax>})
            </passengers>
          </grp>})

$(defmacro BAG_TRANSFER_GRP
  bag_amount
  bag_weight
  rk_weight
  range
  surname
  name
  range2
  surname2
  name2
{          <grp>
            <bag_amount>$(bag_amount)</bag_amount>
            <bag_weight>$(bag_weight)</bag_weight>
            <rk_weight>$(rk_weight)</rk_weight>
            <weight_unit>K</weight_unit>
            <seats>-2147483648</seats>
            <tag_ranges>
              <range>$(range)</range>\
$(if $(eq $(range2) "") "" {
              <range>$(range2)</range>})
            </tag_ranges>
            <passengers>
              <pax>
                <surname>$(surname)</surname>\
$(if $(eq $(name) "") "" {
                <name>$(name)</name>})
              </pax>\
$(if $(eq $(surname2) "") "" {
              <pax>
                <surname>$(surname2)</surname>\
$(if $(eq $(name2) "") "" {
                <name>$(name2)</name>})
              </pax>})
            </passengers>
          </grp>})

### test 1 - отметки на главном экране "Перевозки" о входящем и исходящем трансфере (стрелочки ->)
#########################################################################################

$(init_term)

$(set today $(date_format %d.%m.%Y +0))
$(set tomor $(date_format %d.%m.%Y +1))

$(NEW_SPP_FLIGHT_ONE_LEG UT 576 "" KRR "$(get today) 07:00" "$(get today) 15:00" VKO)
$(NEW_SPP_FLIGHT_ONE_LEG UT 351 "" VKO "$(get tomor) 05:00" "$(get tomor) 07:00" HMA)
$(NEW_SPP_FLIGHT_ONE_LEG UT 375 "" VKO "$(get today) 16:00" "$(get today) 17:30" SCW)
$(NEW_SPP_FLIGHT_ONE_LEG UT 453 "" VKO "$(get today) 17:00" "$(get today) 19:00" TJM)
$(NEW_SPP_FLIGHT_ONE_LEG UT 533 "" VKO "$(get today) 20:00" "$(get tomor) 01:00" DYR)
$(NEW_SPP_FLIGHT_ONE_LEG UT 595 "" VKO "$(get tomor) 09:30" "$(get tomor) 11:45" USK)
$(NEW_SPP_FLIGHT_ONE_LEG UT 4321 "" VKO "$(get tomor) 11:00" "$(get tomor) 13:15" AER) #до кучи

$(set move_id_576 $(get_move_id $(get_point_dep_for_flight UT 576 "" $(yymmdd) KRR)))
$(set move_id_351 $(get_move_id $(get_point_dep_for_flight UT 351 "" $(yymmdd +1) VKO)))
$(set move_id_375 $(get_move_id $(get_point_dep_for_flight UT 375 "" $(yymmdd) VKO)))
$(set move_id_453 $(get_move_id $(get_point_dep_for_flight UT 453 "" $(yymmdd) VKO)))
$(set move_id_533 $(get_move_id $(get_point_dep_for_flight UT 533 "" $(yymmdd) VKO)))
$(set move_id_595 $(get_move_id $(get_point_dep_for_flight UT 595 "" $(yymmdd +1) VKO)))
$(set move_id_4321 $(get_move_id $(get_point_dep_for_flight UT 4321 "" $(yymmdd +1) VKO)))

$(PTM_UT_576_KRR)
$(BTM_UT_576_KRR)


!! capture=on
$(READ_TRIPS $(get today) EN)

>> mode=regex
<\?xml version='1.0' encoding='CP866'\?>
<term>
  <answer .*>
    <data>
      <flight_date>$(get today) 00:00:00</flight_date>
      <trips>
        <trip>
          <move_id>$(get move_id_576)</move_id>
          <point_id>\d+</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>KRR</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>576</flt_no_out>
          <scd_out>$(get today) 07:00:00</scd_out>
          <triptype_out>p</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp>VKO</airp>
          </places_out>
.*
        </trip>
        <trip>
          <move_id>$(get move_id_576)</move_id>
          <point_id>\d+</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>576</flt_no_in>
          <scd_in>$(get today) 15:00:00</scd_in>
          <triptype_in>p</triptype_in>
          <places_in>
            <airp>KRR</airp>
          </places_in>
          <airp>VKO</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
          <trfertype>15</trfertype>
          <trfer_from>-&gt;</trfer_from>
        </trip>
        <trip>
          <move_id>$(get move_id_375)</move_id>
          <point_id>\d+</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>375</flt_no_out>
          <scd_out>$(get today) 16:00:00</scd_out>
          <triptype_out>p</triptype_out>
          <pr_reg>1</pr_reg>
          <trfertype>240</trfertype>
          <trfer_from>-&gt;</trfer_from>
          <places_out>
            <airp>SCW</airp>
          </places_out>
.*
        </trip>
        <trip>
          <move_id>$(get move_id_375)</move_id>
          <point_id>\d+</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>375</flt_no_in>
          <scd_in>$(get today) 17:30:00</scd_in>
          <triptype_in>p</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>SCW</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_453)</move_id>
          <point_id>\d+</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>453</flt_no_out>
          <scd_out>$(get today) 17:00:00</scd_out>
          <triptype_out>p</triptype_out>
          <pr_reg>1</pr_reg>
          <trfertype>240</trfertype>
          <trfer_from>-&gt;</trfer_from>
          <places_out>
            <airp>TJM</airp>
          </places_out>
.*
        </trip>
        <trip>
          <move_id>$(get move_id_453)</move_id>
          <point_id>\d+</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>453</flt_no_in>
          <scd_in>$(get today) 19:00:00</scd_in>
          <triptype_in>p</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>TJM</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_533)</move_id>
          <point_id>\d+</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>533</flt_no_out>
          <scd_out>$(get today) 20:00:00</scd_out>
          <triptype_out>p</triptype_out>
          <pr_reg>1</pr_reg>
          <trfertype>240</trfertype>
          <trfer_from>-&gt;</trfer_from>
          <places_out>
            <airp>DYR</airp>
          </places_out>
.*
        </trip>
      </trips>
    </data>
  </answer>
</term>
$()

!! capture=on
$(READ_TRIPS $(get tomor) RU)

>> mode=regex
<\?xml version='1.0' encoding='CP866'\?>
<term>
  <answer .*>
    <data>
      <flight_date>$(get tomor) 00:00:00</flight_date>
      <trips>
        <trip>
          <move_id>$(get move_id_351)</move_id>
          <point_id>\d+</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>351</flt_no_out>
          <scd_out>$(get tomor) 05:00:00</scd_out>
          <triptype_out>п</triptype_out>
          <pr_reg>1</pr_reg>
          <trfertype>240</trfertype>
          <trfer_from>-&gt;</trfer_from>
          <places_out>
            <airp>HMA</airp>
          </places_out>
.*
        </trip>
        <trip>
          <move_id>$(get move_id_351)</move_id>
          <point_id>\d+</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>351</flt_no_in>
          <scd_in>$(get tomor) 07:00:00</scd_in>
          <triptype_in>п</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>HMA</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_533)</move_id>
          <point_id>\d+</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>533</flt_no_in>
          <scd_in>$(get tomor) 01:00:00</scd_in>
          <triptype_in>п</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>DYR</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_595)</move_id>
          <point_id>\d+</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>595</flt_no_out>
          <scd_out>$(get tomor) 09:30:00</scd_out>
          <triptype_out>п</triptype_out>
          <pr_reg>1</pr_reg>
          <trfertype>240</trfertype>
          <trfer_from>-&gt;</trfer_from>
          <places_out>
            <airp>USK</airp>
          </places_out>
.*
        </trip>
        <trip>
          <move_id>$(get move_id_595)</move_id>
          <point_id>\d+</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>595</flt_no_in>
          <scd_in>$(get tomor) 11:45:00</scd_in>
          <triptype_in>п</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>USK</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_4321)</move_id>
          <point_id>\d+</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>4321</flt_no_out>
          <scd_out>$(get tomor) 11:00:00</scd_out>
          <triptype_out>п</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp>AER</airp>
          </places_out>
.*
        </trip>
        <trip>
          <move_id>$(get move_id_4321)</move_id>
          <point_id>\d+</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>4321</flt_no_in>
          <scd_in>$(get tomor) 13:15:00</scd_in>
          <triptype_in>п</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>AER</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
      </trips>
    </data>
  </answer>
</term>
$()

%%
### test 2 - информация о трансферных пассажирах/багаже, прибывающих рейсом
#########################################################################################

$(init_term)

$(set today $(date_format %d.%m.%Y +0))
$(set tomor $(date_format %d.%m.%Y +1))

$(NEW_SPP_FLIGHT_ONE_LEG UT 576 "" KRR "$(get today) 07:00" "$(get today) 15:00" VKO)

$(set point_dep $(get_point_dep_for_flight UT 576 "" $(yymmdd) KRR))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(PTM_UT_576_KRR)
$(BTM_UT_576_KRR)

$(GET_PAX_TRANSFER_REQUEST capture=on $(get point_arv) pr_out=0 pr_tlg=1 RU)

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <trip>ЮТ576 КПА</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT ЮТ375/17 ВНК СЫВ Э)
        <grps>
$(PAX_TRANSFER_GRP 0 1 GRITSENKO SVETLANA)
$(PAX_TRANSFER_GRP 0 1 EGOROV IURII)
$(PAX_TRANSFER_GRP 1 1 PLOSKOVA "VALENTINA NIKOLAEVNA")
$(PAX_TRANSFER_GRP 0 1 IARKOVA "SOFIA DMITRIEVNA")
$(PAX_TRANSFER_GRP 0 1 CHERNIAEV "GRIGORII GENNADEVICH")
$(PAX_TRANSFER_GRP 0 2 CHERNIAEV "IAROSLAV GRIGORE"
                       CHERNIAEV "TIMUR GRIGOREVICH")
$(PAX_TRANSFER_GRP 1 1 KOVALENKO "TAMARA ANDREEVNA")
$(PAX_TRANSFER_GRP 0 1 ROMANOV "IURII VLADIMIROVICH")
$(PAX_TRANSFER_GRP 0 2 ROMANOVA "EKATERINA IUREVNA"
                       ROMANOVA "MARINA VASILEVNA")
$(PAX_TRANSFER_GRP 0 1 GREBENIUK "SERGEI VLADIMIROVICH")
$(PAX_TRANSFER_GRP 1 1 ZIUZEV "ANATOLII AFANASEVICH")
$(PAX_TRANSFER_GRP 0 1 GRECHKIN "IVAN ALEKSANDROVICH")
$(PAX_TRANSFER_GRP 0 2 GRECHKINA "ARINA ALEKSANDROVN"
                       GRECHKINA "IRINA VALEREVNA")
$(PAX_TRANSFER_GRP 0 1 IVANCHENKO "OLGA NIKOLAEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ЮТ453/17 ВНК РЩН Э)
        <grps>
$(PAX_TRANSFER_GRP 1 1 VORONOI "IURII VLADIMIROVICH")
$(PAX_TRANSFER_GRP 1 1 BONDAR "PAVEL SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 RABOCHII "NIKOLAI ANATOLEVICH")
$(PAX_TRANSFER_GRP 1 1 ARUTIUNIAN "IURII OTARIKOVICH")
$(PAX_TRANSFER_GRP 0 1 KASIANOV "VALERII VALEREVICH")
$(PAX_TRANSFER_GRP 1 1 SNOPKO "NIKITA IUREVICH")
$(PAX_TRANSFER_GRP 0 1 STANISLAVSKII "VLADIMIR VALEREVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ЮТ533/17 ВНК АНЫ Э)
        <grps>
$(PAX_TRANSFER_GRP 1 1 GASHPAR "PAVEL EVGENEVICH")
$(PAX_TRANSFER_GRP 1 1 NOVIK "MARINA NIKOLAEVNA")
$(PAX_TRANSFER_GRP 1 1 PUSHECHKIN "IURII GEORGIEVICH")
$(PAX_TRANSFER_GRP 1 1 KANISHCHEV "SERGEI NIKOLAEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "ANGELINA SERGEEVNA")
$(PAX_TRANSFER_GRP 2 1 DUSHKO "DENIS SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "NIKITA SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "SERGEI ALEKSEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "ARTEM SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "MARIIA GENNADEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ЮТ351/18 ВНК ХАС Э)
        <grps>
$(PAX_TRANSFER_GRP 0 1 SHESTAKOV "ALEKSEI SERGEEVICH")
$(PAX_TRANSFER_GRP 2 1 CHALIN "ALEKSANDR NIKOLAEVICH")
$(PAX_TRANSFER_GRP 0 1 NADOLNYI "EVGENII ANDREEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ЮТ369/18 ВНК ПЛК П)
        <grps>
$(PAX_TRANSFER_GRP 0 1 KHAMIDOV "BAKHTIER SAIDAKRAMOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ЮТ595/18 ВНК УСН Э)
        <grps>
$(PAX_TRANSFER_GRP 1 1 BULYCHEV "OLEG NIKOLAEVICH")
$(PAX_TRANSFER_GRP 1 1 PRIKHODKO "VLADIMIR MIKHAILOVICH")
        </grps>
      </trfer_flt>
    </transfer>



$(GET_BAG_TRANSFER_REQUEST capture=on $(get point_arv) pr_out=0 pr_tlg=1 EN)

>>  lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <trip>ЮТ576 КПА</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT UT375/17 VKO SCW Y)
        <grps>
$(BAG_TRANSFER_GRP 1 16 0 0298453919 PLOSKOVA "VALENTINA NIKOLAEVNA")
$(BAG_TRANSFER_GRP 1 13 0 0298453920 KOVALENKO "TAMARA ANDREEVNA")
$(BAG_TRANSFER_GRP 1 10 0 0298453921 ZIUZEV "ANATOLII AFANASEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT453/17 VKO TJM Y)
        <grps>
$(BAG_TRANSFER_GRP 1 18 0 0298446624 VORONOI "IURII VLADIMIROVICH")
$(BAG_TRANSFER_GRP 1 19 0 0298453311 BONDAR "PAVEL SERGEEVICH")
$(BAG_TRANSFER_GRP 1 17 0 0298453312 RABOCHII "NIKOLAI ANATOLEVICH")
$(BAG_TRANSFER_GRP 1 15 0 0298453313 ARUTIUNIAN "IURII OTARIKOVICH")
$(BAG_TRANSFER_GRP 2 25 0 0298406080 UNACCOMPANIED ""
                          0298406090)
$(BAG_TRANSFER_GRP 1 14 0 0298453314 SNOPKO "NIKITA IUREVICH"
                          0298453316-326 )
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT533/17 VKO DYR)
        <grps>
$(BAG_TRANSFER_GRP 1  5 0 0298454800 UNAC)
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT533/17 VKO DYR Y)
        <grps>
$(BAG_TRANSFER_GRP 1 20 7 0298454719 GASHPAR "PAVEL EVGENEVICH")
$(BAG_TRANSFER_GRP 1 20 0 0298454720 NOVIK "MARINA NIKOLAEVNA")
$(BAG_TRANSFER_GRP 1 16 0 0298454721 PUSHECHKIN "IURII GEORGIEVICH")
$(BAG_TRANSFER_GRP 1 19 0 0298454722 KANISHCHEV "SERGEI NIKOLAEVICH")
$(BAG_TRANSFER_GRP 1 17 0 0298454723 DUSHKO "ANGELINA SERGEEVNA")
$(BAG_TRANSFER_GRP 2 24 0 0298454724 BOKOV ANTON
                          0298454729 DUSHKO "DENIS SERGEEVICH")
$(BAG_TRANSFER_GRP 1 11 0 0298454725 DUSHKO "NIKITA SERGEEVICH")
$(BAG_TRANSFER_GRP 1 16 0 0298454726 DUSHKO "SERGEI ALEKSEEVICH")
$(BAG_TRANSFER_GRP 1 12 0 0298454727 DUSHKO "ARTEM SERGEEVICH")
$(BAG_TRANSFER_GRP 1 12 0 0298454728 DUSHKO "MARIIA GENNADEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT351/18 VKO HMA Y)
        <grps>
$(BAG_TRANSFER_GRP 2 21 0 0298428065-066 CHALIN "ALEKSANDR NIKOLAEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT595/18 VKO USK Y)
        <grps>
$(BAG_TRANSFER_GRP 1 18 0 0298449290 BULYCHEV "OLEG NIKOLAEVICH")
$(BAG_TRANSFER_GRP 1 15 0 0298449289 PRIKHODKO "VLADIMIR MIKHAILOVICH")
        </grps>
      </trfer_flt>
    </transfer>






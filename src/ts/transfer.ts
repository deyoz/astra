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

$(defmacro TRANSFER_FLT2
  trip
  airp_dep
  airp_arv
  subcl
{        <trip>$(trip)</trip>
        <airp>$(airp_dep)</airp>
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

$(defmacro PAX_TRANSFER_GRP_WITH_WEIGHT
  bag_amount
  bag_weight
  seats
  surname
  name
  surname2
  name2
{          <grp>
            <bag_amount>$(bag_amount)</bag_amount>
            <bag_weight>$(bag_weight)</bag_weight>
            <rk_weight/>
            <weight_unit>K</weight_unit>
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

$(defmacro BAG_TRANSFER_GRP2
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
              <range>$(range)<alarm>TRFER_UNATTACHED</alarm></range>\
$(if $(eq $(range2) "") "" {
              <range>$(range2)<alarm>TRFER_UNATTACHED</alarm></range>})
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

### test 1 - Æ‚¨•‚™® ≠† £´†¢≠Æ¨ Ì™‡†≠• "è•‡•¢Æß™®" Æ ¢ÂÆ§ÔÈ•¨ ® ®·ÂÆ§ÔÈ•¨ ‚‡†≠·‰•‡• (·‚‡•´ÆÁ™® ->)
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
$(NEW_SPP_FLIGHT_ONE_LEG UT 4321 "" VKO "$(get tomor) 11:00" "$(get tomor) 13:15" AER) #§Æ ™„Á®

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

>> lines=1-20,88-120,188-218,286-316,384-389
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <data>
      <flight_date>$(get today) 00:00:00</flight_date>
      <trips>
        <trip>
          <move_id>$(get move_id_576)</move_id>
          <point_id>...</point_id>
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
...
        </trip>
        <trip>
          <move_id>$(get move_id_576)</move_id>
          <point_id>...</point_id>
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
          <point_id>...</point_id>
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
...
        </trip>
        <trip>
          <move_id>$(get move_id_375)</move_id>
          <point_id>...</point_id>
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
          <point_id>...</point_id>
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
...
        </trip>
        <trip>
          <move_id>$(get move_id_453)</move_id>
          <point_id>...</point_id>
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
          <point_id>...</point_id>
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
...
        </trip>
      </trips>
    </data>
  </answer>
</term>


!! capture=on
$(READ_TRIPS $(get tomor) RU)

>> lines=1-22,90-134,202-230,298-316
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <data>
      <flight_date>$(get tomor) 00:00:00</flight_date>
      <trips>
        <trip>
          <move_id>$(get move_id_351)</move_id>
          <point_id>...</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>351</flt_no_out>
          <scd_out>$(get tomor) 05:00:00</scd_out>
          <triptype_out>Ø</triptype_out>
          <pr_reg>1</pr_reg>
          <trfertype>240</trfertype>
          <trfer_from>-&gt;</trfer_from>
          <places_out>
            <airp>HMA</airp>
          </places_out>
...
        </trip>
        <trip>
          <move_id>$(get move_id_351)</move_id>
          <point_id>...</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>351</flt_no_in>
          <scd_in>$(get tomor) 07:00:00</scd_in>
          <triptype_in>Ø</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>HMA</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_533)</move_id>
          <point_id>...</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>533</flt_no_in>
          <scd_in>$(get tomor) 01:00:00</scd_in>
          <triptype_in>Ø</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>DYR</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_595)</move_id>
          <point_id>...</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>595</flt_no_out>
          <scd_out>$(get tomor) 09:30:00</scd_out>
          <triptype_out>Ø</triptype_out>
          <pr_reg>1</pr_reg>
          <trfertype>240</trfertype>
          <trfer_from>-&gt;</trfer_from>
          <places_out>
            <airp>USK</airp>
          </places_out>
...
        </trip>
        <trip>
          <move_id>$(get move_id_595)</move_id>
          <point_id>...</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>595</flt_no_in>
          <scd_in>$(get tomor) 11:45:00</scd_in>
          <triptype_in>Ø</triptype_in>
          <places_in>
            <airp>VKO</airp>
          </places_in>
          <airp>USK</airp>
          <pr_del_out>-1</pr_del_out>
          <pr_reg>0</pr_reg>
        </trip>
        <trip>
          <move_id>$(get move_id_4321)</move_id>
          <point_id>...</point_id>
          <pr_del_in>-1</pr_del_in>
          <airp>VKO</airp>
          <airline_out>UT</airline_out>
          <flt_no_out>4321</flt_no_out>
          <scd_out>$(get tomor) 11:00:00</scd_out>
          <triptype_out>Ø</triptype_out>
          <pr_reg>1</pr_reg>
          <places_out>
            <airp>AER</airp>
          </places_out>
...
        </trip>
        <trip>
          <move_id>$(get move_id_4321)</move_id>
          <point_id>...</point_id>
          <airline_in>UT</airline_in>
          <flt_no_in>4321</flt_no_in>
          <scd_in>$(get tomor) 13:15:00</scd_in>
          <triptype_in>Ø</triptype_in>
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

%%
### test 2 - ®≠‰Æ‡¨†Ê®Ô Æ ‚‡†≠·‰•‡≠ÎÂ Ø†··†¶®‡†Â/°†£†¶•, Ø‡®°Î¢†ÓÈ®Â ‡•©·Æ¨
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
    <trip>ûí576 äèÄ</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT ûí375/$(dd) Ççä ëõÇ ù)
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
$(TRANSFER_FLT ûí453/$(dd) Ççä ëìê ù)
        <grps>
$(PAX_TRANSFER_GRP 1 1 VORONOI "IURII VLADIMIROVICH")
$(PAX_TRANSFER_GRP 1 1 BONDAR "PAVEL SERGEEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ûí453/$(dd) Ççä êôç ù)
        <grps>
$(PAX_TRANSFER_GRP 1 1 RABOCHII "NIKOLAI ANATOLEVICH")
$(PAX_TRANSFER_GRP 1 1 ARUTIUNIAN "IURII OTARIKOVICH")
$(PAX_TRANSFER_GRP 0 1 KASIANOV "VALERII VALEREVICH")
$(PAX_TRANSFER_GRP 1 1 SNOPKO "NIKITA IUREVICH")
$(PAX_TRANSFER_GRP 0 1 STANISLAVSKII "VLADIMIR VALEREVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ûí533/$(dd) Ççä Äçõ è)
        <grps>
$(PAX_TRANSFER_GRP 1 1 KANISHCHEV "SERGEI NIKOLAEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ûí533/$(dd) Ççä Äçõ Å)
        <grps>
$(PAX_TRANSFER_GRP 1 1 DUSHKO "ANGELINA SERGEEVNA")
$(PAX_TRANSFER_GRP 2 1 DUSHKO "DENIS SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "NIKITA SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "SERGEI ALEKSEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "ARTEM SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 DUSHKO "MARIIA GENNADEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ûí533/$(dd) Ççä Äçõ ù)
        <grps>
$(PAX_TRANSFER_GRP 1 1 GASHPAR "PAVEL EVGENEVICH")
$(PAX_TRANSFER_GRP 1 1 NOVIK "MARINA NIKOLAEVNA")
$(PAX_TRANSFER_GRP 1 1 PUSHECHKIN "IURII GEORGIEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ûí351/$(dd +1) Ççä ïÄë ù)
        <grps>
$(PAX_TRANSFER_GRP 0 1 SHESTAKOV "ALEKSEI SERGEEVICH")
$(PAX_TRANSFER_GRP 2 1 CHALIN "ALEKSANDR NIKOLAEVICH")
$(PAX_TRANSFER_GRP 0 1 NADOLNYI "EVGENII ANDREEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ûí369/$(dd +1) Ççä èãä è)
        <grps>
$(PAX_TRANSFER_GRP 0 1 KHAMIDOV "BAKHTIER SAIDAKRAMOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT ûí595/$(dd +1) Ççä ìëç ù)
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
    <trip>ûí576 äèÄ</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT UT375/$(dd) VKO SCW Y)
        <grps>
$(BAG_TRANSFER_GRP 1 16 0 0298453919 PLOSKOVA "VALENTINA NIKOLAEVNA")
$(BAG_TRANSFER_GRP 1 13 0 0298453920 KOVALENKO "TAMARA ANDREEVNA")
$(BAG_TRANSFER_GRP 1 10 0 0298453921 ZIUZEV "ANATOLII AFANASEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT453/$(dd) VKO SGC Y)
        <grps>
$(BAG_TRANSFER_GRP 1 18 0 0298446624 VORONOI "IURII VLADIMIROVICH")
$(BAG_TRANSFER_GRP 1 19 0 0298453311 BONDAR "PAVEL SERGEEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT453/$(dd) VKO TJM)
        <grps>
$(BAG_TRANSFER_GRP 2 25 0 0298406080 UNACCOMPANIED ""
                          0298406090)
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT453/$(dd) VKO TJM Y)
        <grps>
$(BAG_TRANSFER_GRP 1 17 0 0298453312 RABOCHII "NIKOLAI ANATOLEVICH")
$(BAG_TRANSFER_GRP 1 15 0 0298453313 ARUTIUNIAN "IURII OTARIKOVICH")
$(BAG_TRANSFER_GRP 1 14 0 0298453314 SNOPKO "NIKITA IUREVICH"
                          0298453316-326 )
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT533/$(dd) VKO DYR)
        <grps>
$(BAG_TRANSFER_GRP 1  5 0 0298454800 UNAC)
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT533/$(dd) VKO DYR F)
        <grps>
$(BAG_TRANSFER_GRP 1 19 0 0298454722 KANISHCHEV "SERGEI NIKOLAEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT533/$(dd) VKO DYR C)
        <grps>
$(BAG_TRANSFER_GRP 1 17 0 0298454723 DUSHKO "ANGELINA SERGEEVNA")
$(BAG_TRANSFER_GRP 2 24 0 0298454724 DUSHKO "DENIS SERGEEVICH"
                          0298454729 ÅùÉÉàçë îêéÑé)
$(BAG_TRANSFER_GRP 1 11 0 0298454725 DUSHKO "NIKITA SERGEEVICH")
$(BAG_TRANSFER_GRP 1 16 0 0298454726 DUSHKO "SERGEI ALEKSEEVICH")
$(BAG_TRANSFER_GRP 1 12 0 0298454727 DUSHKO "ARTEM SERGEEVICH")
$(BAG_TRANSFER_GRP 1 12 0 0298454728 DUSHKO "MARIIA GENNADEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT533/$(dd) VKO DYR Y)
        <grps>
$(BAG_TRANSFER_GRP 1 20 7 0298454719 GASHPAR "PAVEL EVGENEVICH")
$(BAG_TRANSFER_GRP 1 20 0 0298454720 NOVIK "MARINA NIKOLAEVNA")
$(BAG_TRANSFER_GRP 1 16 0 0298454721 PUSHECHKIN "IURII GEORGIEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT351/$(dd +1) VKO HMA Y)
        <grps>
$(BAG_TRANSFER_GRP 2 21 0 0298428065-066 CHALIN "ALEKSANDR NIKOLAEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT UT595/$(dd +1) VKO USK Y)
        <grps>
$(BAG_TRANSFER_GRP 1 18 0 0298449290 BULYCHEV "OLEG NIKOLAEVICH")
$(BAG_TRANSFER_GRP 1 15 0 0298449289 PRIKHODKO "VLADIMIR MIKHAILOVICH")
        </grps>
      </trfer_flt>
    </transfer>

%%
### test 3 - ®≠‰Æ‡¨†Ê®Ô Æ ‚‡†≠·‰•‡≠ÎÂ Ø†··†¶®‡†Â/°†£†¶•, Æ‚Ø‡†¢´ÔÓÈ®Â·Ô ‡•©·Æ¨
#########################################################################################

$(init_term)

$(set today $(date_format %d.%m.%Y +0))
$(set tomor $(date_format %d.%m.%Y +1))

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point UT 453 "" "" ""                   VKO "$(get today) 17:00")
  $(new_spp_point UT 453 "" "" "$(get today) 19:00" TJM "$(get today) 20:00")
  $(new_spp_point_last         "$(get today) 23:00" SGC ) })

$(NEW_SPP_FLIGHT_ONE_LEG UT 533 "" VKO "$(get today) 20:00" "$(get tomor) 01:00" DYR)

$(set point_id_453 $(get_point_dep_for_flight UT 453 "" $(yymmdd) VKO))
$(set point_id_533 $(get_point_dep_for_flight UT 533 "" $(yymmdd) VKO))

$(INBOUND_TRANSFER_UT_453_VKO)
$(INBOUND_TRANSFER_UT_533_VKO)

$(GET_PAX_TRANSFER_REQUEST capture=on $(get point_id_453) pr_out=1 pr_tlg=1 EN)

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <trip>ûí453 Ççä</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT2 UT248/$(dd) SGC VKO)
        <grps>
$(PAX_TRANSFER_GRP 0 1 IARTSEVA "SVETLANA NIKOLAEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT250/$(dd) AER VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 RYBIN ARTEM)
$(PAX_TRANSFER_GRP 0 1 RYBINA "MARINA SERGEEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT334/$(dd) NNM VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 ROCHEVA ZOIA)
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT370/$(dd) LED VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 DUDINA "DARIA ANDREEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT374/$(dd) MRV VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 ZAKHARENKO KIRILL)
$(PAX_TRANSFER_GRP 1 1 ZAKHARENKO "OLESIA IUREVNA")
$(PAX_TRANSFER_GRP 1 1 PREMOVA ELENA)
$(PAX_TRANSFER_GRP 1 1 GERASIMENKO "OLEG EVGENEVICH")
$(PAX_TRANSFER_GRP 1 1 GERASIMENKO "MARINA GENNADEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT400/$(dd) GRV VKO)
        <grps>
$(PAX_TRANSFER_GRP 0 1 CHAGAEV "ALI MAGOMEDOVICH")
$(PAX_TRANSFER_GRP 0 1 MUTUZOVA "ASMA ALIEVNA")
$(PAX_TRANSFER_GRP 0 2 KHASIEVA "DESHI SAIPIEVNA"
                       KHASIEVA "MEDINA ARTUROVNA")
$(PAX_TRANSFER_GRP 0 1 TAIDAEV ADAM)
$(PAX_TRANSFER_GRP 0 1 AKHMADOVA ZALINA)
$(PAX_TRANSFER_GRP 0 1 TOKKHADZHIEV ABDULKERIM)
$(PAX_TRANSFER_GRP 0 1 BARAKHOEVA "LARISA SULTANOVNA")
$(PAX_TRANSFER_GRP 0 1 BARAKHOEV "BAGDAN ZAKRIEVICH")
$(PAX_TRANSFER_GRP 0 2 BARAKHOEVA "ALINA ZAKRIEVNA"
                       BARAKHOEVA "AMINA ZAKRIEVNA")
$(PAX_TRANSFER_GRP 1 1 MAKHMADOV SULEIMAN)
$(PAX_TRANSFER_GRP 1 1 TARAMOV "RIZVAN RAMZANOVICH MR")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT460/$(dd) AAQ VKO)
        <grps>
$(PAX_TRANSFER_GRP 0 1 ANISIMOV "ALEKSANDR ANATOLEVICH")
$(PAX_TRANSFER_GRP 0 1 GOLOVENKO "KONSTANTIN NIKOLAEVICH")
$(PAX_TRANSFER_GRP 0 1 GOGORIAN "RUBIK AKOPOVICH")
$(PAX_TRANSFER_GRP 3 1 SAVELEV "DMITRII SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 SAVELEVA "ALENA VIKTOROVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT558/$(dd) MCX VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 AGAEV "RAMAZAN KHALIMBEKOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT576/$(dd) KRR VKO)
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
$(TRANSFER_FLT2 UT736/$(dd) ZNZ VKO)
        <grps>
$(PAX_TRANSFER_GRP_WITH_WEIGHT 1 10 1 KHLYSTOV O)
$(PAX_TRANSFER_GRP_WITH_WEIGHT 1 12 1 KHLYSTOVA D)
        </grps>
      </trfer_flt>
    </transfer>

$(GET_BAG_TRANSFER_REQUEST capture=on $(get point_id_453) pr_out=1 pr_tlg=1 RU)

>>  lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <trip>ûí453 Ççä</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT2 ûí250/$(dd) ëéó Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 17 10 0298441792 RYBIN ARTEM)
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí334/$(dd) ççê Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 14 0 0298465400 ROCHEVA ZOIA)
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí370/$(dd) èãä Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 14 0 0298440640 DUDINA "DARIA ANDREEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí374/$(dd) åêÇ Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 16 5 0298455193 ZAKHARENKO KIRILL)
$(BAG_TRANSFER_GRP2 1 20 0 0298455194 ZAKHARENKO "OLESIA IUREVNA")
$(BAG_TRANSFER_GRP2 1  9 0 0298455195 PREMOVA ELENA)
$(BAG_TRANSFER_GRP2 1 15 0 0298455192 GERASIMENKO "OLEG EVGENEVICH")
$(BAG_TRANSFER_GRP2 1 20 3 0298455191 GERASIMENKO "MARINA GENNADEVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí400/$(dd) Éêç Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1  6 0 0298413058 MAKHMADOV SULEIMAN)
$(BAG_TRANSFER_GRP2 1  7 0 0298413059 TARAMOV "RIZVAN RAMZANOVICH MR")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí460/$(dd) ÄçÄ Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 3 39 5 0298444608-609 SAVELEV "DMITRII SERGEEVICH"
                           0298444611 )
$(BAG_TRANSFER_GRP2 1 17 5 0298444610     SAVELEVA "ALENA VIKTOROVNA")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí558/$(dd) åïã Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 16 5 0298446119 AGAEV "RAMAZAN KHALIMBEKOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí576/$(dd) äèÄ Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 18 0 0298446624 VORONOI "IURII VLADIMIROVICH")
$(BAG_TRANSFER_GRP2 1 19 0 0298453311 BONDAR "PAVEL SERGEEVICH")
$(BAG_TRANSFER_GRP2 1 17 0 0298453312 RABOCHII "NIKOLAI ANATOLEVICH")
$(BAG_TRANSFER_GRP2 1 15 0 0298453313 ARUTIUNIAN "IURII OTARIKOVICH")
$(BAG_TRANSFER_GRP2 1 14 0 0298453314 SNOPKO "NIKITA IUREVICH"
                           0298453316-326 )
$(BAG_TRANSFER_GRP2 2 25 0 0298406080 UNACCOMPANIED ""
                           0298406090 )
        </grps>
      </trfer_flt>
    </transfer>

$(GET_PAX_TRANSFER_REQUEST capture=on $(get point_id_533) pr_out=1 pr_tlg=1 EN)

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <trip>ûí533 Ççä</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT2 UT248/$(dd) SGC VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 IPOLITOVA "NATALIA EVGENEVNA")
$(PAX_TRANSFER_GRP 0 1 PIMENOVA "OLGA KIRILLOVNA")
$(PAX_TRANSFER_GRP 2 1 IPOLITOV "SERGEI IVANOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT370/$(dd) LED VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 KRIUCHKOV "VIKTOR FEDOROVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT374/$(dd) MRV VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 IARAKHMEDOV "RUSTAM NASRULLAKHOVICH")
$(PAX_TRANSFER_GRP 1 1 BULANOV VLADIMIR)
$(PAX_TRANSFER_GRP 1 1 MARKOV "OLEG SERGEEVICH")
$(PAX_TRANSFER_GRP 1 1 ZADOROZHNII "NIKOLAI NIKOLAEVICH")
$(PAX_TRANSFER_GRP 1 1 ZADOROZHNIAIA "MARINA VLADIMIROVNA")
$(PAX_TRANSFER_GRP 1 1 ZADOROZHNII "NIKOLAI NIKOLAEVICH")
$(PAX_TRANSFER_GRP 1 1 BELEVTSOV "DMITRII GENADEVICH")
$(PAX_TRANSFER_GRP 1 1 NEMCHENKO "IVAN BORISOVICH")
$(PAX_TRANSFER_GRP 1 1 SUNUGATULIN "MARAT TAGEROVICH")
$(PAX_TRANSFER_GRP 1 1 KOZHIN "SERGEI ALEKSANDROVICH")
$(PAX_TRANSFER_GRP 1 1 BORISENKO "ALEKSEI IUREVICH")
$(PAX_TRANSFER_GRP 1 1 BORISENKO "MIKHAIL NIKOLAEVICH")
$(PAX_TRANSFER_GRP 1 1 ZABABURIN "ANDREI GENNADEVICH")
$(PAX_TRANSFER_GRP 1 1 MIKHAILOV "VITALII ALEKSANDROVICH")
$(PAX_TRANSFER_GRP 1 1 DZHULAEV "VALERII IVANOVICH")
$(PAX_TRANSFER_GRP 1 1 BOGDANOV "PAVEL IVANOVICH")
$(PAX_TRANSFER_GRP 1 1 PASHKOV "NIKOLAI VLADIMIROVICH")
$(PAX_TRANSFER_GRP 1 1 TRAPEZNIKOV "ROMAN NIKOLAEVICH")
$(PAX_TRANSFER_GRP 1 1 MAZHIRIN "OLEG VLADIMIROVICH")
$(PAX_TRANSFER_GRP 1 1 VASILEV "DENIS IUREVICH")
$(PAX_TRANSFER_GRP 1 1 PRILUKA "SERGEI GENNADEVICH")
$(PAX_TRANSFER_GRP 1 1 ERESKO "DANIL VIKTOROVICH")
$(PAX_TRANSFER_GRP 1 1 GAVRILENKO "SERGEI SERGEEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT380/$(dd) SCW VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 CHURKIN "ANATOLII IVANOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT454/$(dd) TJM VKO)
        <grps>
$(PAX_TRANSFER_GRP 1 1 USPENSKII "ALEKSANDR ALEKSANDROVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 UT576/$(dd) KRR VKO)
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
    </transfer>


$(GET_BAG_TRANSFER_REQUEST capture=on $(get point_id_533) pr_out=1 pr_tlg=1 RU)

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <trip>ûí533 Ççä</trip>
    <transfer>
      <trfer_flt>
$(TRANSFER_FLT2 ûí248/$(dd) ëìê Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 20 0 0298441694     IPOLITOVA "NATALIA EVGENEVNA")
$(BAG_TRANSFER_GRP2 2 49 0 0298441695-696 IPOLITOV "SERGEI IVANOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí370/$(dd) èãä Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 20 0 0298445909 KRIUCHKOV "VIKTOR FEDOROVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí374/$(dd) åêÇ Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 10 5 0298446362 IARAKHMEDOV "RUSTAM NASRULLAKHOVICH")
$(BAG_TRANSFER_GRP2 1 20 0 0298446363 BULANOV VLADIMIR)
$(BAG_TRANSFER_GRP2 1  7 0 0298446364 MARKOV "OLEG SERGEEVICH")
$(BAG_TRANSFER_GRP2 1 20 0 0298446365 ZADOROZHNII "NIKOLAI NIKOLAEVICH")
$(BAG_TRANSFER_GRP2 1 20 0 0298446366 ZADOROZHNIAIA "MARINA VLADIMIROVNA")
$(BAG_TRANSFER_GRP2 1 19 0 0298446367 ZADOROZHNII "NIKOLAI NIKOLAEVICH")
$(BAG_TRANSFER_GRP2 1  6 0 0298446368 BELEVTSOV "DMITRII GENADEVICH")
$(BAG_TRANSFER_GRP2 1 19 0 0298446369 NEMCHENKO "IVAN BORISOVICH")
$(BAG_TRANSFER_GRP2 1 16 5 0298446370 SUNUGATULIN "MARAT TAGEROVICH")
$(BAG_TRANSFER_GRP2 1 13 5 0298446371 KOZHIN "SERGEI ALEKSANDROVICH")
$(BAG_TRANSFER_GRP2 1  8 0 0298446372 BORISENKO "ALEKSEI IUREVICH")
$(BAG_TRANSFER_GRP2 1 18 0 0298446373 BORISENKO "MIKHAIL NIKOLAEVICH")
$(BAG_TRANSFER_GRP2 1  6 5 0298446374 ZABABURIN "ANDREI GENNADEVICH")
$(BAG_TRANSFER_GRP2 1  9 0 0298446375 MIKHAILOV "VITALII ALEKSANDROVICH")
$(BAG_TRANSFER_GRP2 1 19 0 0298446376 DZHULAEV "VALERII IVANOVICH")
$(BAG_TRANSFER_GRP2 1 15 5 0298446377 BOGDANOV "PAVEL IVANOVICH")
$(BAG_TRANSFER_GRP2 1 11 0 0298446378 PASHKOV "NIKOLAI VLADIMIROVICH")
$(BAG_TRANSFER_GRP2 1 12 0 0298446379 TRAPEZNIKOV "ROMAN NIKOLAEVICH")
$(BAG_TRANSFER_GRP2 1 13 0 0298446380 MAZHIRIN "OLEG VLADIMIROVICH")
$(BAG_TRANSFER_GRP2 1 17 0 0298446381 VASILEV "DENIS IUREVICH")
$(BAG_TRANSFER_GRP2 1 13 0 0298446382 PRILUKA "SERGEI GENNADEVICH")
$(BAG_TRANSFER_GRP2 1 16 0 0298446383 ERESKO "DANIL VIKTOROVICH")
$(BAG_TRANSFER_GRP2 1  8 0 0298446384 GAVRILENKO "SERGEI SERGEEVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí380/$(dd) ëõÇ Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 13 4 0298463700 CHURKIN "ANATOLII IVANOVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí454/$(dd) êôç Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 20 0 0298442956 USPENSKII "ALEKSANDR ALEKSANDROVICH")
        </grps>
      </trfer_flt>
      <trfer_flt>
$(TRANSFER_FLT2 ûí576/$(dd) äèÄ Ççä)
        <grps>
$(BAG_TRANSFER_GRP2 1 20 7 0298454719 GASHPAR "PAVEL EVGENEVICH")
$(BAG_TRANSFER_GRP2 1 20 0 0298454720 NOVIK "MARINA NIKOLAEVNA")
$(BAG_TRANSFER_GRP2 1 16 0 0298454721 PUSHECHKIN "IURII GEORGIEVICH")
$(BAG_TRANSFER_GRP2 1 19 0 0298454722 KANISHCHEV "SERGEI NIKOLAEVICH")
$(BAG_TRANSFER_GRP2 1 17 0 0298454723 DUSHKO "ANGELINA SERGEEVNA")
$(BAG_TRANSFER_GRP2 2 24 0 0298454724 DUSHKO "DENIS SERGEEVICH"
                           0298454729 ÅùÉÉàçë îêéÑé)
$(BAG_TRANSFER_GRP2 1 11 0 0298454725 DUSHKO "NIKITA SERGEEVICH")
$(BAG_TRANSFER_GRP2 1 16 0 0298454726 DUSHKO "SERGEI ALEKSEEVICH")
$(BAG_TRANSFER_GRP2 1 12 0 0298454727 DUSHKO "ARTEM SERGEEVICH")
$(BAG_TRANSFER_GRP2 1 12 0 0298454728 DUSHKO "MARIIA GENNADEVNA")
$(BAG_TRANSFER_GRP2 1  5 0 0298454800 UNAC )
        </grps>
      </trfer_flt>
    </transfer>


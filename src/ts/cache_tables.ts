include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite adm

$(defmacro LOAD_CACHE
  code
  sql_params
{
!! capture=on err=ignore
$(cache PIKE RU $(code) "" "")

>> lines=auto
    <data>
      <code>$(code)</code>

})

$(defmacro LOAD_BAG_NORMS_RATES
  code
  point_id
  use_mark_flt
  airline_mark
  flt_no_mark
{
!! capture=on err=ignore
$(cache PIKE RU $(code) "" ""
        $(cache_sql_param point_id integer $(point_id))
        $(cache_sql_param use_mark_flt integer $(use_mark_flt))
        $(cache_sql_param airline_mark string $(airline_mark))
        $(cache_sql_param flt_no_mark integer $(flt_no_mark))
)

>> lines=auto
    <data>
      <code>$(code)</code>

})

### test 1
### ��ࢮ��砫쭠� ����㧪� ��� ��襩
#########################################################################################

$(init_term)

$(LOAD_CACHE AGENCIES)
$(LOAD_CACHE AIRLINE_BAG_NORMS)
$(LOAD_CACHE AIRLINE_BAG_RATES)
$(LOAD_CACHE AIRLINE_BI_SET)
$(LOAD_CACHE AIRLINE_BP_SET)
$(LOAD_CACHE AIRLINE_BRD_WITH_REG_SET)
$(LOAD_CACHE AIRLINE_BT_SET)
$(LOAD_CACHE AIRLINE_EMDA_SET)
$(LOAD_CACHE AIRLINE_EXAM_WITH_BRD_SET)
$(LOAD_CACHE AIRLINE_EXCHANGE_RATES)
$(LOAD_CACHE AIRLINE_KIOSK_CKIN_SETS)
$(LOAD_CACHE AIRLINE_MISC_SET)
$(LOAD_CACHE AIRLINE_OFFICES)
$(LOAD_CACHE AIRLINE_PAID_CKIN_SETS)
$(LOAD_CACHE AIRLINE_PERS_WEIGHTS)
$(LOAD_CACHE AIRLINE_PROFILES)
$(LOAD_CACHE AIRLINES)
$(LOAD_CACHE AIRLINE_TRANZIT_SET)
$(LOAD_CACHE AIRLINE_VALUE_BAG_TAXES)
$(LOAD_CACHE AIRLINE_VO_SET)
$(LOAD_CACHE AIRLINE_WEB_CKIN_SETS)
$(LOAD_CACHE AIRPS)
$(LOAD_CACHE AIRP_TERMINALS)
$(LOAD_CACHE ALARM_TYPES)
$(LOAD_CACHE APIS_FORMATS)
$(LOAD_CACHE APIS_SETS)
$(LOAD_CACHE APIS_TRANSPORTS)
$(LOAD_CACHE APPS_FORMATS)
$(LOAD_CACHE APPS_SETS)
$(LOAD_CACHE BAG_NORMS)
$(LOAD_CACHE BAG_NORM_TYPES)
$(LOAD_CACHE BAG_RATES)
$(LOAD_CACHE BAG_TYPES)
$(LOAD_CACHE BAG_UNACCOMP)
$(LOAD_CACHE BALANCE_SETS)
$(LOAD_CACHE BALANCE_TYPES)
$(LOAD_CACHE BASIC_BAG_NORMS)
$(LOAD_CACHE BASIC_BAG_RATES)
$(LOAD_CACHE BASIC_EXCHANGE_RATES)
$(LOAD_CACHE BASIC_PERS_WEIGHTS)
$(LOAD_CACHE BASIC_VALUE_BAG_TAXES)
$(LOAD_CACHE BCBP_REPRINT_OPTIONS)
$(LOAD_CACHE BI_AIRLINE_SERVICE)
$(LOAD_CACHE BI_BLANK_LIST)
$(LOAD_CACHE BI_HALLS)
$(LOAD_CACHE BI_HALLS_AND_TERMINALS)
$(LOAD_CACHE BI_MODELS)
$(LOAD_CACHE BI_PRINT_RULES)
$(LOAD_CACHE BI_PRINT_TYPES)
$(LOAD_CACHE BI_SET)
$(LOAD_CACHE BI_TYPES)
$(LOAD_CACHE BP_BLANK_LIST)
$(LOAD_CACHE BP_MODELS)
$(LOAD_CACHE BP_SET)
$(LOAD_CACHE BP_TYPES)
$(LOAD_CACHE BRAND_FARES)
$(LOAD_CACHE BRANDS)
$(LOAD_CACHE BR_BLANK_LIST)
$(LOAD_CACHE BRD_WITH_REG_SET)
$(LOAD_CACHE BR_MODELS)
$(LOAD_CACHE BT_BLANK_LIST)
$(LOAD_CACHE BT_MODELS)
$(LOAD_CACHE BT_SET)
$(LOAD_CACHE CANON_NAMES)
$(LOAD_CACHE CITIES)
$(LOAD_CACHE CKIN_REM_TYPES)
$(LOAD_CACHE CKIN_REM_TYPES2)
$(LOAD_CACHE CLASSES)
$(LOAD_CACHE CODESHARE_SETS)
$(LOAD_CACHE CODE4_FMT)
$(LOAD_CACHE COMP_REM_TYPES)
$(LOAD_CACHE COMP_SUBCLS_SETS)
$(LOAD_CACHE CONFIRMATION_SETS)
$(LOAD_CACHE COUNTRIES)
$(LOAD_CACHE CRAFTS)
$(LOAD_CACHE CRS)
$(LOAD_CACHE CRS_SET)
$(LOAD_CACHE CRYPT_REQ_DATA)
$(LOAD_CACHE CRYPT_SETS)
$(LOAD_CACHE CURRENCY)
$(LOAD_CACHE CUSTOM_ALARM_SETS)
$(LOAD_CACHE CUSTOM_ALARM_TYPES)
$(LOAD_CACHE DATE_TIME_ZONESPEC)
$(LOAD_CACHE DCS_ACTIONS1)
$(LOAD_CACHE DCS_ACTIONS2)
$(LOAD_CACHE DCS_ADDR_SET)
$(LOAD_CACHE DCS_SERVICE_APPLYING)
$(LOAD_CACHE DELAYS)
$(LOAD_CACHE DESK_GRP)
$(LOAD_CACHE DESK_GRP_SETS)
$(LOAD_CACHE DESK_LOGGING)
$(LOAD_CACHE DESK_OWNERS)
$(LOAD_CACHE DESK_OWNERS_ADD)
$(LOAD_CACHE DESK_OWNERS_GRP)
$(LOAD_CACHE DESKS)
$(LOAD_CACHE DESK_TRACES)
$(LOAD_CACHE DEV_FMT_OPERS)
$(LOAD_CACHE DEV_FMT_TYPES)
$(LOAD_CACHE DEV_MODELS)
$(LOAD_CACHE DEV_OPER_TYPES)
$(LOAD_CACHE DOC_NUM_COPIES)
$(LOAD_CACHE EDI_ADDRS)
$(LOAD_CACHE EDIFACT_PROFILES)
$(LOAD_CACHE EMDA_BLANK_LIST)
$(LOAD_CACHE EMDA_MODELS)
$(LOAD_CACHE EMDA_SET)
$(LOAD_CACHE EMDA_TYPES)
$(LOAD_CACHE ENCODING_FMT)
$(LOAD_CACHE ENCODINGS)
$(LOAD_CACHE ET_ADDR_SET)
$(LOAD_CACHE EXAM_WITH_BRD_SET)
$(LOAD_CACHE EXCHANGE_RATES)
$(LOAD_CACHE EXTRA_ROLE_ACCESS)
$(LOAD_CACHE EXTRA_USER_ACCESS)
$(LOAD_CACHE FILE_TYPES)
$(LOAD_CACHE FORM_PACKS)
$(LOAD_CACHE FORM_TYPES)
$(LOAD_CACHE FQT_REM_TYPES)
$(LOAD_CACHE FQT_TIER_LEVELS)
$(LOAD_CACHE FQT_TIER_LEVELS_EXTENDED)
$(LOAD_CACHE FRANCHISE_SETS)
$(LOAD_CACHE GENDER_TYPES)
$(LOAD_CACHE GRAPH_STAGES)
$(LOAD_CACHE GRAPH_STAGES_WO_INACTIVE)
$(LOAD_CACHE GRAPH_TIMES)
$(LOAD_CACHE GRP_STATUS_TYPES)
$(LOAD_CACHE HALLS)
$(LOAD_CACHE HOTEL_ACMD)
$(LOAD_CACHE HOTEL_ROOM_TYPES)
$(LOAD_CACHE IN_FILE_ENCODING)
$(LOAD_CACHE IN_FILE_PARAM_SETS)
$(LOAD_CACHE KIOSK_ADDR)
$(LOAD_CACHE KIOSK_ALIASES)
$(LOAD_CACHE KIOSK_ALIASES_LIST)
$(LOAD_CACHE KIOSK_APP_LIST)
$(LOAD_CACHE KIOSK_BP_SET)
$(LOAD_CACHE KIOSK_CKIN_DESK_GRP)
$(LOAD_CACHE KIOSK_CKIN_DESKS)
$(LOAD_CACHE KIOSK_CKIN_SETS)
$(LOAD_CACHE KIOSK_CONFIG)
$(LOAD_CACHE KIOSK_CONFIG_LIST)
$(LOAD_CACHE KIOSK_GRP)
$(LOAD_CACHE KIOSK_GRP_NAMES)
$(LOAD_CACHE KIOSK_LANG)
$(LOAD_CACHE LIBRA_CLASSES)
$(LOAD_CACHE MISC_SET)
$(LOAD_CACHE MISC_SET_TYPES)
$(LOAD_CACHE NO_TXT_REPORT_TYPES)
$(LOAD_CACHE OPERATORS)
$(LOAD_CACHE OUT_FILE_ENCODING)
$(LOAD_CACHE OUT_FILE_PARAM_SETS)
$(LOAD_CACHE PACTS)
$(LOAD_CACHE PAID_CKIN_SETS)
$(LOAD_CACHE PAX_CATS)
$(LOAD_CACHE PAX_DOC_COUNTRIES)
$(LOAD_CACHE PAX_DOCO_TYPES)
$(LOAD_CACHE PAX_DOCO_TYPES2)
$(LOAD_CACHE PAX_DOC_TYPES)
$(LOAD_CACHE PAX_DOC_TYPES2)
$(LOAD_CACHE PAY_CLIENTS)
$(LOAD_CACHE PAY_METHODS_SET)
$(LOAD_CACHE PAY_METHODS_TYPES)
$(LOAD_CACHE PAY_TYPES)
$(LOAD_CACHE PERSONS)
$(LOAD_CACHE PLACE_CALC)
$(LOAD_CACHE POS_TERM_SETS)
$(LOAD_CACHE POS_TERM_VENDORS)
$(LOAD_CACHE PRN_FORMS)
$(LOAD_CACHE PRN_FORMS_LAYOUT)
$(LOAD_CACHE PRN_FORM_VERS)
$(LOAD_CACHE PROFILED_RIGHTS_LIST)
$(LOAD_CACHE RATE_COLORS)
$(LOAD_CACHE RCPT_DOC_TYPES)
$(LOAD_CACHE REFUSE)
$(LOAD_CACHE REMARKS)
$(LOAD_CACHE REM_EVENT_SETS)
$(LOAD_CACHE REM_GRP)
$(LOAD_CACHE REM_TXT_SETS)
$(LOAD_CACHE REPORT_TYPES)
$(LOAD_CACHE RFIC_TYPES)
$(LOAD_CACHE RFISC_BAG_PROPS)
$(LOAD_CACHE RFISC_COMP_PROPS)
$(LOAD_CACHE RFISC_RATES)
$(LOAD_CACHE RFISC_RATES_SELF_CKIN)
$(LOAD_CACHE RFISC_SETS)
$(LOAD_CACHE RFISC_TYPES)
$(LOAD_CACHE RIGHTS)
$(LOAD_CACHE ROLES)
$(LOAD_CACHE ROT)
$(LOAD_CACHE SALE_DESKS)
$(LOAD_CACHE SALE_POINTS)
$(LOAD_CACHE SEASON_TYPES)
$(LOAD_CACHE SEAT_ALGO_SETS)
$(LOAD_CACHE SEAT_ALGO_TYPES)
$(LOAD_CACHE SEAT_DESCRIPT)
$(LOAD_CACHE SEAT_TYPES)
$(LOAD_CACHE SELF_CKIN_SET)
$(LOAD_CACHE SELF_CKIN_SET_TYPES)
$(LOAD_CACHE SELF_CKIN_TYPES)
$(LOAD_CACHE SERVICE_TYPES)
$(LOAD_CACHE SOPP_STAGE_STATUSES)
$(LOAD_CACHE STAGE_NAMES)
$(LOAD_CACHE STAGE_SETS)
$(LOAD_CACHE STATION_HALLS)
$(LOAD_CACHE STATION_MODES)
$(LOAD_CACHE STATIONS)
$(LOAD_CACHE SUBCLS)
$(LOAD_CACHE TAG_COLORS)
$(LOAD_CACHE TAG_TYPES)
$(LOAD_CACHE TAG_TYPES_PRINTABLE)
$(LOAD_CACHE TERM_PROFILE_RIGHTS)
$(LOAD_CACHE TIMATIC_SETS)
$(LOAD_CACHE TIME_FMT)
$(LOAD_CACHE TLGS_IN_TYPES)
$(LOAD_CACHE TRANZIT_SET)
$(LOAD_CACHE TRFER_SETS)
$(LOAD_BAG_NORMS_RATES TRIP_BAG_NORMS 123 0 "��" 580)
$(LOAD_BAG_NORMS_RATES TRIP_BAG_NORMS2 123 0 "��" 580)
$(LOAD_BAG_NORMS_RATES TRIP_BAG_RATES 123 0 "��" 580)
$(LOAD_BAG_NORMS_RATES TRIP_BAG_RATES2 123 0 "��" 580)
$(LOAD_CACHE TRIP_BAG_UNACCOMP)
$(LOAD_CACHE TRIP_BI)
$(LOAD_CACHE TRIP_BP)
$(LOAD_CACHE TRIP_BRD_WITH_REG)
$(LOAD_CACHE TRIP_BT)
$(LOAD_CACHE TRIP_EMDA)
$(LOAD_CACHE TRIP_EXAM_WITH_BRD)
$(LOAD_CACHE TRIP_EXCHANGE_RATES)
$(LOAD_CACHE TRIP_KIOSK_CKIN)
$(LOAD_CACHE TRIP_LIST_DAYS)
$(LOAD_CACHE TRIP_LITERS)
$(LOAD_CACHE TRIP_PAID_CKIN)
$(LOAD_CACHE TRIP_SUFFIXES)
$(LOAD_CACHE TRIP_TYPES)
$(LOAD_CACHE TRIP_VALUE_BAG_TAXES)
$(LOAD_CACHE TRIP_VO)
$(LOAD_CACHE TRIP_WEB_CKIN)
$(LOAD_CACHE TYPEB_ADDR_OWNERS_OTHERS)
$(LOAD_CACHE TYPEB_ADDR_OWNERS_SIRENA)
$(LOAD_CACHE TYPEB_ADDRS)
$(LOAD_CACHE TYPEB_ADDRS_BSM)
$(LOAD_CACHE TYPEB_ADDRS_COM)
$(LOAD_CACHE TYPEB_ADDRS_ETL)
$(LOAD_CACHE TYPEB_ADDRS_ETL_MARK)
$(LOAD_CACHE TYPEB_ADDRS_FORWARDING)
$(LOAD_CACHE TYPEB_ADDRS_LCI)
$(LOAD_CACHE TYPEB_ADDRS_LDM)
$(LOAD_CACHE TYPEB_ADDRS_MARK)
$(LOAD_CACHE TYPEB_ADDRS_MVT)
$(LOAD_CACHE TYPEB_ADDRS_PIL)
$(LOAD_CACHE TYPEB_ADDRS_PNL_MARK)
$(LOAD_CACHE TYPEB_ADDRS_PRL)
$(LOAD_CACHE TYPEB_ADDRS_PRL_MARK)
$(LOAD_CACHE TYPEB_ADDRS_SOM)
$(LOAD_CACHE TYPEB_ADDR_TRANS_PARAMS)
$(LOAD_CACHE TYPEB_COM_VERSION)
$(LOAD_CACHE TYPEB_CREATE_POINTS)
$(LOAD_CACHE TYPEB_LCI_ACTION_CODE)
$(LOAD_CACHE TYPEB_LCI_SEAT_RESTRICT)
$(LOAD_CACHE TYPEB_LCI_VERSION)
$(LOAD_CACHE TYPEB_LCI_WEIGHT_AVAIL)
$(LOAD_CACHE TYPEB_LDM_VERSION)
$(LOAD_CACHE TYPEB_ORIGINATORS)
$(LOAD_CACHE TYPEB_PRL_CREATE_POINT)
$(LOAD_CACHE TYPEB_PRL_PAX_STATE)
$(LOAD_CACHE TYPEB_PRL_VERSION)
$(LOAD_CACHE TYPEB_SEND)
$(LOAD_CACHE TYPEB_TRANSPORTS_OTHERS)
$(LOAD_CACHE TYPEB_TYPES)
$(LOAD_CACHE TYPEB_TYPES_ALL)
$(LOAD_CACHE TYPEB_TYPES_BSM)
$(LOAD_CACHE TYPEB_TYPES_COM)
$(LOAD_CACHE TYPEB_TYPES_ETL)
$(LOAD_CACHE TYPEB_TYPES_FORWARDING)
$(LOAD_CACHE TYPEB_TYPES_LCI)
$(LOAD_CACHE TYPEB_TYPES_LDM)
$(LOAD_CACHE TYPEB_TYPES_MARK)
$(LOAD_CACHE TYPEB_TYPES_MVT)
$(LOAD_CACHE TYPEB_TYPES_PIL)
$(LOAD_CACHE TYPEB_TYPES_PNL)
$(LOAD_CACHE TYPEB_TYPES_PRL)
$(LOAD_CACHE TYPEB_TYPES_SOM)
$(LOAD_CACHE USERS)
$(LOAD_CACHE USER_TYPES)
$(LOAD_CACHE VALIDATOR_TYPES)
$(LOAD_CACHE VALUE_BAG_TAXES)
$(LOAD_CACHE VO_BLANK_LIST)
$(LOAD_CACHE VO_MODELS)
$(LOAD_CACHE VO_SET)
$(LOAD_CACHE VO_TYPES)
$(LOAD_CACHE WEB_CKIN_SETS)
$(LOAD_CACHE WEB_CLIENTS)
$(LOAD_CACHE WEB_SALES)

%%

### test 2
#########################################################################################

$(init_term)

### FILE_TYPES
#########################################################################################

!! capture=on
$(cache PIKE RU FILE_TYPES "" "")

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>1CCEK</col>
          <col>���㧪� 1� (�����)</col>
        </row>
        <row pr_del='0'>
          <col>AODBI</col>
          <col>AODB ���㧪�</col>
        </row>
        <row pr_del='0'>
          <col>AODBO</col>
          <col>AODB ���㧪�</col>
        </row>
        <row pr_del='0'>
          <col>APIS_ES</col>
          <col>���� (�ᯠ���)</col>
        </row>
        <row pr_del='0'>
          <col>APIS_LT</col>
          <col>���� (��⢠)</col>
        </row>
        <row pr_del='0'>
          <col>APIS_TR</col>
          <col>Turkish Apis</col>
        </row>
        <row pr_del='0'>
          <col>CENTR</col>
          <col>����஢��</col>
        </row>
        <row pr_del='0'>
          <col>CHCKD</col>
          <col>�������� ॣ����樨</col>
        </row>
        <row pr_del='0'>
          <col>FIDS</col>
          <col>���㧪� Fids</col>
        </row>
        <row pr_del='0'>
          <col>MERIDIAN</col>
          <col>��ਤ���</col>
        </row>
        <row pr_del='0'>
          <col>MINTRANS</col>
          <col>���㧪� ��� ����࠭�</col>
        </row>
        <row pr_del='0'>
          <col>MQRF</col>
          <col>��ࠢ�� ������� �� ३�� � Rabbit MQ</col>
        </row>
        <row pr_del='0'>
          <col>MQRO</col>
          <col>��ࠢ�� ������� ॣ����樨 � Rabbit MQ</col>
        </row>
        <row pr_del='0'>
          <col>ROZYSK_SIR</col>
          <col>��७�-஧��</col>
        </row>
        <row pr_del='0'>
          <col>SOFI</col>
          <col>���㧪� ���</col>
        </row>
        <row pr_del='0'>
          <col>SPCEK</col>
          <col>���㧪� ��� (�����)</col>
        </row>
        <row pr_del='0'>
          <col>UTG</col>
          <col>UTG ���㧪�</col>
        </row>
      </rows>

### IN_FILE_ENCODING, OUT_FILE_ENCODING
#########################################################################################

!! capture=on
$(cache PIKE RU IN_FILE_ENCODING $(cache_iface_ver IN_FILE_ENCODING) ""
  insert type:AODBI
         point_addr:RASTRV
         encoding:UTF-8)

$(set id_in $(last_history_row_id FILE_ENCODING))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id_in)</col>
          <col>AODBI</col>
          <col>RASTRV</col>
          <col>UTF-8</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU OUT_FILE_ENCODING $(cache_iface_ver OUT_FILE_ENCODING) ""
  insert type:AODBO
         point_addr:RASTRV
         encoding:CP1251)

$(set id_out $(last_history_row_id FILE_ENCODING))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id_out)</col>
          <col>AODBO</col>
          <col>RASTRV</col>
          <col>CP1251</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU OUT_FILE_ENCODING $(cache_iface_ver OUT_FILE_ENCODING) ""
  update id:$(get id_out)   old_id:$(get id_out)
         type:AODBO         old_type:AODBO
         point_addr:RASTRN  old_point_addr:RASTRV
         encoding:CP866     old_encoding:CP1251)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id_out)</col>
          <col>AODBO</col>
          <col>RASTRN</col>
          <col>CP866</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU IN_FILE_ENCODING $(cache_iface_ver IN_FILE_ENCODING) ""
  update id:$(get id_in)    old_id:$(get id_in)
         type:AODBI         old_type:AODBI
         point_addr:RASTRN  old_point_addr:RASTRV
         encoding:CP866     old_encoding:UTF-8)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id_in)</col>
          <col>AODBI</col>
          <col>RASTRN</col>
          <col>CP866</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU IN_FILE_ENCODING $(cache_iface_ver IN_FILE_ENCODING) ""
  delete old_id:$(get id_in)
         old_type:AODBI
         old_point_addr:RASTRN
         old_encoding:CP866)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>IN_FILE_ENCODING</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'/>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>...</message>
    </command>
  </answer>
</term>

!! capture=on
$(cache PIKE RU OUT_FILE_ENCODING $(cache_iface_ver OUT_FILE_ENCODING) ""
  delete old_id:$(get id_out)
         old_type:AODBO
         old_point_addr:RASTRN
         old_encoding:CP866)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>OUT_FILE_ENCODING</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'/>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>...</message>
    </command>
  </answer>
</term>

### IN_FILE_PARAM_SETS, OUT_FILE_PARAM_SETS
#########################################################################################

!! capture=on
$(cache PIKE EN IN_FILE_PARAM_SETS $(cache_iface_ver IN_FILE_PARAM_SETS) ""
  insert type:AODBI
         point_addr:RASTRV
         airline:��
         flt_no:
         airp:���
         param_name:WORKDIR
         param_value:d:\temp\aodbi)

$(set id_in $(last_history_row_id FILE_PARAM_SETS))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id_in)</col>
          <col>AODBI</col>
          <col>RASTRV</col>
          <col>��</col>
          <col>UT</col>
          <col/>
          <col>���</col>
          <col>VKO</col>
          <col>WORKDIR</col>
          <col>d:\temp\aodbi</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN OUT_FILE_PARAM_SETS $(cache_iface_ver OUT_FILE_PARAM_SETS) ""
  insert type:AODBO
         point_addr:RASTRV
         airline:
         flt_no:777
         airp:
         param_name:WORKDIR
         param_value:d:\temp\aodbo)

$(set id_out $(last_history_row_id FILE_PARAM_SETS))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id_out)</col>
          <col>AODBO</col>
          <col>RASTRV</col>
          <col/>
          <col/>
          <col>777</col>
          <col/>
          <col/>
          <col>WORKDIR</col>
          <col>d:\temp\aodbo</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN OUT_FILE_PARAM_SETS $(cache_iface_ver OUT_FILE_PARAM_SETS) ""
  update id:$(get id_out)   old_id:$(get id_out)
         type:AODBO         old_type:AODBO
         point_addr:RASTRN  old_point_addr:RASTRV
         airline:��         old_airline:
         flt_no:            old_flt_no:777
         airp:���           old_airp:
         param_name:���     old_param_name:WORKDIR
         param_value:d:\    old_param_value:d:\temp\aodbo)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id_out)</col>
          <col>AODBO</col>
          <col>RASTRN</col>
          <col>��</col>
          <col>UT</col>
          <col/>
          <col>���</col>
          <col>AAQ</col>
          <col>���</col>
          <col>d:\</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN OUT_FILE_PARAM_SETS $(cache_iface_ver OUT_FILE_PARAM_SETS) ""
  delete old_id:$(get id_out)
         old_type:AODBO
         old_point_addr:RASTRN
         old_airline:��
         old_flt_no:
         old_airp:���
         old_param_name:���
         old_param_value:d:\)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>OUT_FILE_PARAM_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'/>
    </data>
    <command>
      <message lexema_id='MSG.CHANGED_DATA_COMMIT' code='0'>...</message>
    </command>
  </answer>
</term>


%%

### test 3
### AIRLINES
#########################################################################################

$(init_term)

$(set max_tid 681836912)
$(set not_max_tid 681825960)
$(set readonly 0)
$(set iface_ver $(cache_iface_ver AIRLINES))

### tid ����䥩� ᮢ������, tid ������ ���ᨬ����
### ��祣� �� ��㧨�

!! capture=on
$(cache PIKE RU AIRLINES $(get iface_ver) $(get max_tid))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>AIRLINES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>$(get readonly)</ReadOnly>
      <keep_locally>1</keep_locally>
      <keep_deleted_rows>1</keep_deleted_rows>
      <user_depend>0</user_depend>
    </data>
  </answer>
</term>


### tid ����䥩� ᮢ������, tid ������ �� ���ᨬ����
### ��㧨� �������騥 �����

!! capture=on
$(cache PIKE RU AIRLINES $(get iface_ver) $(get not_max_tid))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>AIRLINES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>$(get readonly)</ReadOnly>
      <keep_locally>1</keep_locally>
      <keep_deleted_rows>1</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='$(get max_tid)'>
        <row pr_del='0'>
          <col>0Z</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col>MIRNY AIR ENTERPRISE</col>
          <col>MIRNY AIR ENTERPRISE</col>
          <col>���</col>
          <col>���</col>
          <col>494045</col>
        </row>
        <row pr_del='1'>
          <col>5F</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col>ARCTIC CIRCLE AIR SERVICE</col>
          <col>ARCTIC CIRCLE AIR SERVICE</col>
          <col/>
          <col/>
          <col>21364</col>
        </row>
        <row pr_del='1'>
          <col>AAA</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col>MIRNY AIR ENTERPRISE</col>
          <col>MIRNY AIR ENTERPRISE</col>
          <col>���</col>
          <col>���</col>
          <col>528793</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>I8</col>
          <col>���</col>
          <col>IZA</col>
          <col>23A</col>
          <col>������</col>
          <col/>
          <col>��� ������</col>
          <col>IZHAVIA</col>
          <col>���</col>
          <col>���</col>
          <col>8</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>N4</col>
          <col/>
          <col>NWS</col>
          <col>��</col>
          <col/>
          <col/>
          <col>�������� �����</col>
          <col>NORDWIND</col>
          <col>���</col>
          <col>���</col>
          <col>72</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>5F</col>
          <col/>
          <col/>
          <col>��</col>
          <col>SUD-AEROCARGO</col>
          <col/>
          <col>�� �� SUD AEROCARGO</col>
          <col>SUD AEROCARGO</col>
          <col>���</col>
          <col>���</col>
          <col>142</col>
        </row>
        <row pr_del='0'>
          <col>�5</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col>�� ����</col>
          <col/>
          <col/>
          <col/>
          <col>38666</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>F7</col>
          <col>RSY</col>
          <col>RSY</col>
          <col>��</col>
          <col>�� ����</col>
          <col/>
          <col>��� �� ����</col>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>908030</col>
        </row>
      </rows>
    </data>
  </answer>
</term>


### tid ����䥩� ᮢ������, tid ������ ���������
### ��㧨� �� �����

!! capture=on
$(cache PIKE RU AIRLINES $(get iface_ver) "")

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>AIRLINES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>$(get readonly)</ReadOnly>
      <keep_locally>1</keep_locally>
      <keep_deleted_rows>1</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='$(get max_tid)'>
        <row pr_del='0'>
          <col>01</col>


### tid ����䥩� �� ᮢ������, tid ������ ���ᨬ����
### ��㧨� ����䥩� � �� �����

!! capture=on
$(cache PIKE RU AIRLINES $(+ $(get iface_ver) 1) $(get max_tid))
>> lines=auto
        </fields>
      </iface>
      <rows tid='$(get max_tid)'>
        <row pr_del='0'>
          <col>01</col>


### tid ����䥩� �� ᮢ������, tid ������ �� ���ᨬ����
### ��㧨� ����䥩� � �� �����

!! capture=on
$(cache PIKE RU AIRLINES $(+ $(get iface_ver) 1) $(get not_max_tid))

>> lines=auto
        </fields>
      </iface>
      <rows tid='$(get max_tid)'>
        <row pr_del='0'>
          <col>01</col>


$(sql {UPDATE airlines SET pr_del=1, tid=$(+ $(get max_tid) 1) WHERE pr_del=0})

### tid ����䥩� ᮢ������, tid ������ �� ���ᨬ����
### ��㧨� �������騥 �����

!! capture=on
$(cache PIKE RU AIRLINES $(get iface_ver) $(get max_tid))

>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>AIRLINES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>$(get readonly)</ReadOnly>
      <keep_locally>1</keep_locally>
      <keep_deleted_rows>1</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='$(+ $(get max_tid) 1)'>
        <row pr_del='1'>
          <col>01</col>

%%

### test 4
### PRN_FORMS_LAYOUT
#########################################################################################

$(init_term)

!! capture=on
$(cache PIKE RU PRN_FORMS_LAYOUT $(cache_iface_ver PRN_FORMS_LAYOUT) "")

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>PRN_FORMS_LAYOUT</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>1</ReadOnly>
      <keep_locally>1</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>1</col>
          <col>PRINT_BP</col>
          <col>��ᠤ��� ⠫���</col>
          <col>BP_MODELS</col>
          <col>BP_TYPES</col>
          <col>���᮪ �������</col>
          <col>���᮪ �� ��� ������:</col>
          <col>BP_BLANK_LIST</col>
          <col>airline_bp_set</col>
          <col>trip_bp</col>
          <col>������ ���. ⠫����</col>
          <col>������ ���. ⠫���� ३�</col>
          <col>QST.INSERT_BLANK_BOARDINGPASS_W_SEG</col>
          <col>QST.INSERT_BLANK_BOARDINGPASS_WO_SEG</col>
          <col>MSG.WAIT_PRINTING_BOARDING_PASS</col>
        </row>
        <row pr_del='0'>
          <col>2</col>
          <col>PRINT_BT</col>
          <col>������� ��ન</col>
          <col>BT_MODELS</col>
          <col>TAG_TYPES_PRINTABLE</col>
          <col>���᮪ ��ப</col>
          <col>���᮪ �� ��� ��ન:</col>
          <col>BT_BLANK_LIST</col>
          <col>airline_bt_set</col>
          <col>trip_bt</col>
          <col>������ ���. ��ப</col>
          <col>������ ���. ��ப ३�</col>
          <col/>
          <col/>
          <col/>
        </row>
        <row pr_del='0'>
          <col>3</col>
          <col>PRINT_BR</col>
          <col>������� ���⠭樨</col>
          <col>BR_MODELS</col>
          <col>FORM_TYPES</col>
          <col>���᮪ �������� ���⠭権</col>
          <col>���᮪ �� ��� �������� ���⠭樨:</col>
          <col>BR_BLANK_LIST</col>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
          <col/>
        </row>
        <row pr_del='0'>
          <col>4</col>
          <col>PRINT_BI</col>
          <col>�ਣ��襭��</col>
          <col>BI_MODELS</col>
          <col>BI_TYPES</col>
          <col>���᮪ �ਣ��襭��</col>
          <col>���᮪ �� ��� �ਣ��襭��:</col>
          <col>BI_BLANK_LIST</col>
          <col>airline_bi_set</col>
          <col>trip_bi</col>
          <col>������ �ਣ��襭��</col>
          <col>������ �ਣ��襭�� ३�</col>
          <col>QST.INSERT_BLANK_INVITATION_W_SEG</col>
          <col>QST.INSERT_BLANK_INVITATION_WO_SEG</col>
          <col>MSG.WAIT_PRINTING_INVITATION</col>
        </row>
        <row pr_del='0'>
          <col>5</col>
          <col>PRINT_VO</col>
          <col>������</col>
          <col>VO_MODELS</col>
          <col>VO_TYPES</col>
          <col>���᮪ ����஢</col>
          <col>���᮪ �� ��� �����:</col>
          <col>VO_BLANK_LIST</col>
          <col>airline_vo_set</col>
          <col>trip_vo</col>
          <col>������ ����஢</col>
          <col>������ ����஢ ३�</col>
          <col>QST.INSERT_BLANK_VO_W_SEG</col>
          <col>QST.INSERT_BLANK_VO_WO_SEG</col>
          <col>MSG.WAIT_PRINTING_VOUCHERS_PASS</col>
        </row>
        <row pr_del='0'>
          <col>6</col>
          <col>PRINT_EMDA</col>
          <col>EMDA</col>
          <col>EMDA_MODELS</col>
          <col>EMDA_TYPES</col>
          <col>���᮪ EMDA</col>
          <col>���᮪ �� ��� EMDA:</col>
          <col>EMDA_BLANK_LIST</col>
          <col>airline_emda_set</col>
          <col>trip_emda</col>
          <col>������ EMDA</col>
          <col>������ EMDA ३�</col>
          <col/>
          <col/>
          <col>MSG.WAIT_PRINTING_EMDA</col>
        </row>
      </rows>
    </data>
  </answer>
</term>

%%

### test 5
### STATIONS, SOPP_STATIONS
#########################################################################################

$(init_term)

!! capture=on
$(cache PIKE EN STATIONS $(cache_iface_ver STATIONS) ""
  insert airp:���
         name:G06
         desk:VDE006
         mode_code:�
         using_scales:0
         without_monitor:0
  insert airp:���
         name:R21
         desk:VDR021
         mode_code:�
         using_scales:0
         without_monitor:0
  insert airp:���
         name:VIP-1R
         desk:AER075
         mode_code:�
         using_scales:0
         without_monitor:0)

$(set id_01 $(last_history_row_id STATIONS -2))
$(set id_02 $(last_history_row_id STATIONS -1))
$(set id_03 $(last_history_row_id STATIONS  0))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>���</col>
          <col>VKO</col>
          <col>G06</col>
          <col>VDE006</col>
          <col>�</col>
          <col>Boarding</col>
          <col>0</col>
          <col>0</col>
          <col>$(get id_01)</col>
        </row>
        <row pr_del='0'>
          <col>���</col>
          <col>VKO</col>
          <col>R21</col>
          <col>VDR021</col>
          <col>�</col>
          <col>Check-in</col>
          <col>0</col>
          <col>0</col>
          <col>$(get id_02)</col>
        </row>
        <row pr_del='0'>
          <col>���</col>
          <col>AER</col>
          <col>VIP-1R</col>
          <col>AER075</col>
          <col>�</col>
          <col>Check-in</col>
          <col>0</col>
          <col>0</col>
          <col>$(get id_03)</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU SOPP_STATIONS $(cache_iface_ver SOPP_STATIONS) ""
        $(cache_sql_param airp string VKO))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>SOPP_STATIONS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>1</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>���</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>R21</col>
          <col>�</col>
        </row>
        <row pr_del='0'>
          <col>G06</col>
          <col>�</col>
        </row>
      </rows>
    </data>
  </answer>
</term>

!! capture=on
$(cache PIKE EN SOPP_STATIONS $(cache_iface_ver SOPP_STATIONS) ""
        $(cache_sql_param airp string ZZZZ))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>ALL</col>
          <col/>
        </row>
      </rows>



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

$(defmacro LOAD_TRIP_BAG_NORMS_ETC
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

$(defmacro LOAD_AIRLINE_CACHE
  code
  airline
{
!! capture=on err=ignore
$(cache PIKE RU $(code) "" ""
        $(cache_sql_param airline string $(airline))
)

>> lines=auto
    <data>
      <code>$(code)</code>

})

$(defmacro LOAD_POINT_CACHE
  code
  point_id
  lang=RU
{
!! capture=on err=ignore
$(cache PIKE $(lang) $(code) "" ""
        $(cache_sql_param point_id integer $(point_id)))

>> lines=auto
    <data>
      <code>$(code)</code>

})

### test 1
### ��ࢮ��砫쭠� ����㧪� ��� ��襩
#########################################################################################

$(init_term)

$(LOAD_CACHE AGENCIES)
$(LOAD_AIRLINE_CACHE AIRLINE_BAG_NORMS ��)
$(LOAD_AIRLINE_CACHE AIRLINE_BAG_RATES ��)
$(LOAD_CACHE AIRLINE_BI_SET)
$(LOAD_CACHE AIRLINE_BP_SET)
$(LOAD_AIRLINE_CACHE AIRLINE_BRD_WITH_REG_SET)
$(LOAD_CACHE AIRLINE_BT_SET)
$(LOAD_CACHE AIRLINE_EMDA_SET)
$(LOAD_AIRLINE_CACHE AIRLINE_EXAM_WITH_BRD_SET)
$(LOAD_AIRLINE_CACHE AIRLINE_EXCHANGE_RATES ��)
$(LOAD_AIRLINE_CACHE AIRLINE_KIOSK_CKIN_SETS ��)
$(LOAD_CACHE AIRLINE_MISC_SET)
$(LOAD_CACHE AIRLINE_OFFICES)
$(LOAD_CACHE AIRLINE_PAID_CKIN_SETS)
$(LOAD_CACHE AIRLINE_PERS_WEIGHTS)
$(LOAD_CACHE AIRLINE_PROFILES)
$(LOAD_CACHE AIRLINES)
$(LOAD_CACHE AIRLINE_TRANZIT_SET)
$(LOAD_AIRLINE_CACHE AIRLINE_VALUE_BAG_TAXES ��)
$(LOAD_CACHE AIRLINE_VO_SET)
$(LOAD_AIRLINE_CACHE AIRLINE_WEB_CKIN_SETS ��)
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
$(LOAD_TRIP_BAG_NORMS_ETC TRIP_BAG_NORMS 123 0 �� 580)
$(LOAD_TRIP_BAG_NORMS_ETC TRIP_BAG_NORMS2 123 0 �� 580)
$(LOAD_TRIP_BAG_NORMS_ETC TRIP_BAG_RATES 123 0 �� 580)
$(LOAD_TRIP_BAG_NORMS_ETC TRIP_BAG_RATES2 123 0 �� 580)
$(LOAD_CACHE TRIP_BAG_UNACCOMP)
$(LOAD_CACHE TRIP_BI)
$(LOAD_CACHE TRIP_BP)
$(LOAD_POINT_CACHE TRIP_BRD_WITH_REG 12345)
$(db_sql TRIP_BT "INSERT INTO TRIP_BT (point_id, tag_type) VALUES (12345, '���')")
$(LOAD_POINT_CACHE TRIP_BT 12345)
$(LOAD_CACHE TRIP_EMDA)
$(LOAD_POINT_CACHE TRIP_EXAM_WITH_BRD 12345)
$(LOAD_TRIP_BAG_NORMS_ETC TRIP_EXCHANGE_RATES 123 0 �� 580)
$(LOAD_CACHE TRIP_KIOSK_CKIN)
$(LOAD_CACHE TRIP_LIST_DAYS)
$(LOAD_CACHE TRIP_LITERS)
$(LOAD_CACHE TRIP_PAID_CKIN)
$(LOAD_CACHE TRIP_SUFFIXES)
$(LOAD_CACHE TRIP_TYPES)
$(LOAD_TRIP_BAG_NORMS_ETC TRIP_VALUE_BAG_TAXES 123 0 �� 580)
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

%%

### test 6
### HOTEL_ACMD
#########################################################################################

$(init_term)

!! capture=on
$(cache PIKE EN HOTEL_ACMD $(cache_iface_ver HOTEL_ACMD) ""
  insert airline:
         airp:���
         {hotel_name:����� ��ଠ�}
         single_amount:15
         double_amount:24)

$(set id $(last_history_row_id HOTEL_ACMD))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>���</col>
          <col>AAQ</col>
          <col/>
          <col/>
          <col>����� ��ଠ�</col>
          <col>15</col>
          <col>24</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN HOTEL_ACMD $(cache_iface_ver HOTEL_ACMD) ""
  update id:$(get id)               old_id:$(get id)
         airline:��                 old_airline:
         airp:���                   old_airp:���
         {hotel_name:����� ��ଠ�} {old_hotel_name:����� ��ଠ�}
         single_amount:16           old_single_amount:15
         double_amount:20           old_double_amount:24)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get id)</col>
          <col>���</col>
          <col>AAQ</col>
          <col/>
          <col/>
          <col>����� ��ଠ�</col>
          <col>16</col>
          <col>20</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN HOTEL_ACMD $(cache_iface_ver HOTEL_ACMD) ""
  delete old_id:$(get id)
         old_airline:��
         old_airp:���
         {old_hotel_name:����� ��ଠ�}
         old_single_amount:16
         old_double_amount:20)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>HOTEL_ACMD</code>
      <Forbidden>1</Forbidden>
      <ReadOnly>1</ReadOnly>
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

??
$(db_dump_table hist_hotel_acmd fields="id,airline,airp,hotel_name,single_amount,double_amount" order="hist_time, hist_order" display="on")

>>
--------------------- hist_hotel_acmd DUMP ---------------------
SELECT id, airline, airp, hotel_name, single_amount, double_amount FROM hist_hotel_acmd ORDER BY hist_time, hist_order
[$(get id)] [NULL] [���] [����� ��ଠ�] [15] [24] $()
[$(get id)] [NULL] [���] [����� ��ଠ�] [16] [20] $()
------------------- END hist_hotel_acmd DUMP COUNT=2 -------------------
$()

??
$(db_dump_table history_events fields="table_id,row_ident,open_user,open_desk,close_user,close_desk" order="open_time, hist_order" display="on")

>>
--------------------- history_events DUMP ---------------------
SELECT table_id, row_ident, open_user, open_desk, close_user, close_desk FROM history_events ORDER BY open_time, hist_order
[155] [$(get id)] [������� �.�.] [������] [������� �.�.] [������] $()
[155] [$(get id)] [������� �.�.] [������] [������� �.�.] [������] $()
------------------- END history_events DUMP COUNT=2 -------------------
$()

%%

### test 7
### VALIDATOR_TYPES, FORM_PACKS
#########################################################################################

$(init_term)

!! capture=on err=ignore
$(cache PIKE EN VALIDATOR_TYPES "" "")

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>5�</col>
          <col>5N</col>
          <col>5�</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>7�</col>
          <col>7K</col>
          <col>7�</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>����</col>
          <col>IATA</col>
          <col>�������� IATA</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>YC</col>
          <col>�������� �/� ����</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>NN</col>
          <col>��</col>
          <col>NN</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>4G</col>
          <col>��</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>FV</col>
          <col>��</col>
          <col>FV</col>
        </row>
        <row pr_del='0'>
          <col>�2</col>
          <col>R2</col>
          <col>�2</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>7R</col>
          <col>��</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>RK</col>
          <col>��</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>TOF</col>
          <col>��</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>���</col>
          <col>TCH</col>
          <col>�������� ���</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>�6</col>
          <col>U6</col>
          <col>�6</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UR</col>
          <col>��</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>�������� �/� ��</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>N2</col>
          <col>�������� �/� ��</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col/>
          <col>����</col>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>6R</col>
          <col>��</col>
          <col/>
        </row>
      </rows>


!! capture=on err=ignore
$(cache PIKE RU FORM_PACKS "" "")

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>35</col>
          <col>35</col>
          <col/>
          <col>0</col>
          <col>10</col>
          <col>1</col>
        </row>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>M61</col>
          <col>M61</col>
          <col/>
          <col>1</col>
          <col>10</col>
          <col>1</col>
        </row>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>Z61</col>
          <col>Z61</col>
          <col/>
          <col>1</col>
          <col>10</col>
          <col>1</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN FORM_PACKS $(cache_iface_ver FORM_PACKS) ""
  update curr_no:3512345683          OLD_curr_no:
         type:35                     OLD_type:35
         user_id:$(get_user_id PIKE) OLD_user_id:$(get_user_id PIKE)
  update curr_no:3493857             OLD_curr_no:
         type:Z61                    OLD_type:Z61
         user_id:$(get_user_id PIKE) OLD_user_id:$(get_user_id PIKE))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>35</col>
          <col>35</col>
          <col>3512345683</col>
          <col>0</col>
          <col>10</col>
          <col>1</col>
        </row>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>M61</col>
          <col>M61</col>
          <col/>
          <col>1</col>
          <col>10</col>
          <col>1</col>
        </row>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>Z61</col>
          <col>Z61</col>
          <col>0003493857</col>
          <col>1</col>
          <col>10</col>
          <col>1</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN FORM_PACKS $(cache_iface_ver FORM_PACKS) ""
  update curr_no:                    OLD_curr_no:3512345683
         type:35                     OLD_type:35
         user_id:$(get_user_id PIKE) OLD_user_id:$(get_user_id PIKE)
  update curr_no:66123493857         OLD_curr_no:3493857
         type:Z61                    OLD_type:Z61
         user_id:$(get_user_id PIKE) OLD_user_id:$(get_user_id PIKE))

>> lines=auto
    <data>
      <code>FORM_PACKS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>35</col>
          <col>35</col>
          <col/>
          <col>0</col>
          <col>10</col>
          <col>1</col>
        </row>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>M61</col>
          <col>M61</col>
          <col/>
          <col>1</col>
          <col>10</col>
          <col>1</col>
        </row>
        <row pr_del='0'>
          <col>$(get_user_id PIKE)</col>
          <col>Z61</col>
          <col>Z61</col>
          <col>66123493857</col>
          <col>1</col>
          <col>10</col>
          <col>1</col>
        </row>
      </rows>
    </data>


%%

### test 8
### CHECK ERRORS
#########################################################################################

$(init_term)

!! capture=on
$(cache PIKE EN COUNTRIES $(cache_iface_ver COUNTRIES) ""
  insert code:OO
         code_lat:RU
         code_iso:RUS
         name:�����)

>> lines=auto
      <user_error lexema_id='MSG.CODE_ALREADY_USED_FOR_COUNTRY' code='0'>The value of 'Code (LAT)' RU field is already used in the 'Code (LAT)' field for country ��</user_error>

%%

### test 9
### PLACE_CALC
#########################################################################################

$(init_jxt_pult ������)
$(login VLAD GHJHSD)

$(UPDATE_USER PIKE "������� �.�." 1
{          <airps>
            <item>���</item>
            <item>���</item>
          </airps>}
 opr=VLAD)

$(login PIKE PIKE)

!! capture=on
$(cache PIKE EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  insert airp_dep:���
         airp_dep_view:VKO
         airp_arv:���
         airp_arv_view:AAQ
         craft:��5
         {time:30.12.1899 01:45:00})

$(set id_VKO_AAQ $(last_history_row_id PLACE_CALC))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>���</col>
          <col>VKO</col>
          <col>���</col>
          <col>AAQ</col>
          <col>��5</col>
          <col>TU5</col>
          <col>30.12.1899 01:45:00</col>
          <col>$(get id_VKO_AAQ)</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  insert airp_dep:���
         airp_dep_view:CEK
         airp_arv:���
         airp_arv_view:AAQ
         craft:��5
         {time:30.12.1899 02:05:00})

$(USER_ERROR_RESPONSE MSG.NO_PERM_ENTER_AP_AND_AP 0 {No rights to fill in the row of the airport CEK and AAQ})

$(login VLAD GHJHSD)

!! capture=on
$(cache VLAD EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  insert airp_dep:���
         airp_dep_view:CEK
         airp_arv:���
         airp_arv_view:AAQ
         craft:��5
         {time:30.12.1899 02:05:00})

$(set id_CEK_AAQ $(last_history_row_id PLACE_CALC))

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>���</col>
          <col>VKO</col>
          <col>���</col>
          <col>AAQ</col>
          <col>��5</col>
          <col>TU5</col>
          <col>30.12.1899 01:45:00</col>
          <col>$(get id_VKO_AAQ)</col>
        </row>
        <row pr_del='0'>
          <col>���</col>
          <col>CEK</col>
          <col>���</col>
          <col>AAQ</col>
          <col>��5</col>
          <col>TU5</col>
          <col>30.12.1899 02:05:00</col>
          <col>$(get id_CEK_AAQ)</col>
        </row>
      </rows>

$(login PIKE PIKE)

!! capture=on
$(cache PIKE EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  update old_id:$(get id_VKO_AAQ)        id:$(get id_VKO_AAQ)
         old_airp_dep:���                airp_dep:���
         old_airp_dep_view:VKO           airp_dep_view:AAQ
         old_airp_arv:���                airp_arv:���
         old_airp_arv_view:AAQ           airp_arv_view:VKO
         old_craft:��5                   craft:777
         {old_time:30.12.1899 01:45:00}  {time:30.12.1899 01:50:00})

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>���</col>
          <col>AAQ</col>
          <col>���</col>
          <col>VKO</col>
          <col>777</col>
          <col>777</col>
          <col>30.12.1899 01:50:00</col>
          <col>$(get id_VKO_AAQ)</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  update old_id:$(get id_VKO_AAQ)        id:$(get id_VKO_AAQ)
         old_airp_dep:���                airp_dep:���
         old_airp_dep_view:AAQ           airp_dep_view:DME
         old_airp_arv:���                airp_arv:���
         old_airp_arv_view:VKO           airp_arv_view:AAQ
         old_craft:777                   craft:777
         {time:30.12.1899 01:50:00}      {time:30.12.1899 01:50:00})

$(USER_ERROR_RESPONSE MSG.NO_PERM_ENTER_AP_AND_AP 0 {No rights to fill in the row of the airport DME and AAQ})

!! capture=on
$(cache PIKE EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  update old_id:$(get id_CEK_AAQ)       id:$(get id_CEK_AAQ)
         old_airp_dep:���               airp_dep:���
         old_airp_dep_view:CEK          airp_dep_view:DME
         old_airp_arv:���               airp_arv:���
         old_airp_arv_view:AAQ          airp_arv_view:AAQ
         old_craft:��5                  craft:��5
         {old_time:30.12.1899 02:05:00} {time:30.12.1899 02:05:00})

$(USER_ERROR_RESPONSE MSG.NO_PERM_MODIFY_AP_AND_AP 0 {No rights to change or delete the row with the airport CEK and AAQ})

$(login VLAD GHJHSD)

!! capture=on
$(cache VLAD EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  update old_id:$(get id_CEK_AAQ)       id:$(get id_CEK_AAQ)
         old_airp_dep:���               airp_dep:���
         old_airp_dep_view:CEK          airp_dep_view:DME
         old_airp_arv:���               airp_arv:���
         old_airp_arv_view:AAQ          airp_arv_view:AAQ
         old_craft:��5                  craft:��5
         {old_time:30.12.1899 02:05:00} {time:30.12.1899 02:05:00})

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>���</col>
          <col>AAQ</col>
          <col>���</col>
          <col>VKO</col>
          <col>777</col>
          <col>777</col>
          <col>30.12.1899 01:50:00</col>
          <col>$(get id_VKO_AAQ)</col>
        </row>
        <row pr_del='0'>
          <col>���</col>
          <col>DME</col>
          <col>���</col>
          <col>AAQ</col>
          <col>��5</col>
          <col>TU5</col>
          <col>30.12.1899 02:05:00</col>
          <col>$(get id_CEK_AAQ)</col>
        </row>
      </rows>

$(login PIKE PIKE)

!! capture=on
$(cache PIKE EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  delete old_id:$(get id_VKO_AAQ)
         old_airp_dep:���
         old_airp_dep_view:AAQ
         old_airp_arv:���
         old_airp_arv_view:VKO
         old_craft:777
         {old_time:30.12.1899 01:50:00})

>> lines=auto
    <data>
      <code>PLACE_CALC</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'/>
    </data>

!! capture=on
$(cache PIKE EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  delete old_id:$(get id_CEK_AAQ)
         old_airp_dep:���
         old_airp_dep_view:DME
         old_airp_arv:���
         old_airp_arv_view:AAQ
         old_craft:��5
         {old_time:30.12.1899 02:05:00})

$(USER_ERROR_RESPONSE MSG.NO_PERM_MODIFY_AP_AND_AP 0 {No rights to change or delete the row with the airport DME and AAQ})

$(login VLAD GHJHSD)

!! capture=on
$(cache VLAD EN PLACE_CALC $(cache_iface_ver PLACE_CALC) ""
  delete old_id:$(get id_CEK_AAQ)
         old_airp_dep:���
         old_airp_dep_view:DME
         old_airp_arv:���
         old_airp_arv_view:AAQ
         old_craft:��5
         {old_time:30.12.1899 02:05:00})

>> lines=auto
    <data>
      <code>PLACE_CALC</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='-1'/>
    </data>

%%

### test 10
### COMP_REM_TYPES, CKIN_REM_TYPES
#########################################################################################

$(init_term)

!! capture=on
$(cache PIKE RU COMP_REM_TYPES $(cache_iface_ver COMP_REM_TYPES) ""
  insert code:���0 code_lat:CMP0 {name:����ઠ ���������� 0} {name_lat:Comp rem 0} priority:0
  insert code:���2 code_lat:CMP2 {name:����ઠ ���������� 2} {name_lat:Comp rem 2} priority:2
  insert code:���5 code_lat:CMP5 {name:����ઠ ���������� 5} {name_lat:Comp rem 5} priority:5
  insert code:���6 code_lat:CMP6 {name:����ઠ ���������� 6} {name_lat:Comp rem 6} priority:6
  insert code:���7 code_lat:CMP7 {name:����ઠ ���������� 7} {name_lat:Comp rem 7} priority:7
  insert code:���8 code_lat:CMP8 {name:����ઠ ���������� 8} {name_lat:Comp rem 8} priority:8)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

$(set id_comp_rem1 $(last_history_row_id comp_rem_types -05))
$(set id_comp_rem2 $(last_history_row_id comp_rem_types -04))
$(set id_comp_rem5 $(last_history_row_id comp_rem_types -03))
$(set id_comp_rem6 $(last_history_row_id comp_rem_types -02))
$(set id_comp_rem7 $(last_history_row_id comp_rem_types -01))
$(set id_comp_rem8 $(last_history_row_id comp_rem_types -00))

!! capture=on
$(cache PIKE RU CKIN_REM_TYPES $(cache_iface_ver CKIN_REM_TYPES) ""
  insert code:���9 code_lat:CKN9 {name:����ઠ ॣ����樨 9} {name_lat:Ckin rem 9} grp_id:9 is_iata:1
  insert code:���4 code_lat:CKN4 {name:����ઠ ॣ����樨 4} {name_lat:Ckin rem 4} grp_id:4 is_iata:1
  insert code:���5 code_lat:CKN5 {name:����ઠ ॣ����樨 5} {name_lat:Ckin rem 5} grp_id:5 is_iata:0
  insert code:���6 code_lat:CKN6 {name:����ઠ ॣ����樨 6} {name_lat:Ckin rem 6} grp_id:6 is_iata:1
  insert code:���7 code_lat:CKN7 {name:����ઠ ॣ����樨 7} {name_lat:Ckin rem 7} grp_id:7 is_iata:0
  insert code:���8 code_lat:CKN8 {name:����ઠ ॣ����樨 8} {name_lat:Ckin rem 8} grp_id:8 is_iata:1)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

$(set id_ckin_rem3 $(last_history_row_id ckin_rem_types -05))
$(set id_ckin_rem4 $(last_history_row_id ckin_rem_types -04))
$(set id_ckin_rem5 $(last_history_row_id ckin_rem_types -03))
$(set id_ckin_rem6 $(last_history_row_id ckin_rem_types -02))
$(set id_ckin_rem7 $(last_history_row_id ckin_rem_types -01))
$(set id_ckin_rem8 $(last_history_row_id ckin_rem_types -00))

### �����塞 � 㤠�塞 ��ப� � comp_rem_types, ckin_rem_types � �����㦠�� ���������

!! capture=on
$(cache PIKE RU COMP_REM_TYPES $(cache_iface_ver COMP_REM_TYPES) $(cache_data_ver comp_rem_types)
  update old_code:���0 old_code_lat:CMP0 {old_name:����ઠ ���������� 0} {old_name_lat:Comp rem 0} old_priority:0 old_id:$(get id_comp_rem1)
             code:���1     code_lat:CMP1     {name:����ઠ ���������� 1}     {name_lat:Comp rem 1}     priority:1     id:$(get id_comp_rem1)
  delete old_code:���2 old_code_lat:CMP2 {old_name:����ઠ ���������� 2} {old_name_lat:Comp rem 2} old_priority:2 old_id:$(get id_comp_rem2)
  delete old_code:���7 old_code_lat:CMP7 {old_name:����ઠ ���������� 7} {old_name_lat:Comp rem 7} old_priority:7 old_id:$(get id_comp_rem7)
  delete old_code:���8 old_code_lat:CMP8 {old_name:����ઠ ���������� 8} {old_name_lat:Comp rem 8} old_priority:8 old_id:$(get id_comp_rem8))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>COMP_REM_TYPES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>1</keep_locally>
      <keep_deleted_rows>1</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='$(cache_data_ver comp_rem_types)'>
        <row pr_del='0'>
          <col>���1</col>
          <col>CMP1</col>
          <col>����ઠ ���������� 1</col>
          <col>Comp rem 1</col>
          <col>1</col>
          <col>$(get id_comp_rem1)</col>
        </row>
        <row pr_del='1'>
          <col>���2</col>
          <col>CMP2</col>
          <col>����ઠ ���������� 2</col>
          <col>Comp rem 2</col>
          <col>2</col>
          <col>$(get id_comp_rem2)</col>
        </row>
        <row pr_del='1'>
          <col>���7</col>
          <col>CMP7</col>
          <col>����ઠ ���������� 7</col>
          <col>Comp rem 7</col>
          <col>7</col>
          <col>$(get id_comp_rem7)</col>
        </row>
        <row pr_del='1'>
          <col>���8</col>
          <col>CMP8</col>
          <col>����ઠ ���������� 8</col>
          <col>Comp rem 8</col>
          <col>8</col>
          <col>$(get id_comp_rem8)</col>
        </row>
      </rows>
    </data>
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)
  </answer>
</term>

!! capture=on
$(cache PIKE RU CKIN_REM_TYPES $(cache_iface_ver CKIN_REM_TYPES) $(cache_data_ver ckin_rem_types)
  update old_code:���9 old_code_lat:CKN9 {old_name:����ઠ ॣ����樨 9} {old_name_lat:Ckin rem 9} old_grp_id:9 old_is_iata:1 old_id:$(get id_ckin_rem3)
             code:���3     code_lat:CKN3     {name:����ઠ ॣ����樨 3}     {name_lat:Ckin rem 3}     grp_id:3     is_iata:0     id:$(get id_ckin_rem3)
  delete old_code:���4 old_code_lat:CKN4 {old_name:����ઠ ॣ����樨 4} {old_name_lat:Ckin rem 4} old_grp_id:4 old_is_iata:1 old_id:$(get id_ckin_rem4)
  delete old_code:���6 old_code_lat:CKN6 {old_name:����ઠ ॣ����樨 6} {old_name_lat:Ckin rem 6} old_grp_id:6 old_is_iata:1 old_id:$(get id_ckin_rem6)
  delete old_code:���8 old_code_lat:CKN8 {old_name:����ઠ ॣ����樨 8} {old_name_lat:Ckin rem 8} old_grp_id:8 old_is_iata:1 old_id:$(get id_ckin_rem8))

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <interface id='cache'/>
    <data>
      <code>CKIN_REM_TYPES</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>1</keep_locally>
      <keep_deleted_rows>1</keep_deleted_rows>
      <user_depend>0</user_depend>
      <rows tid='$(cache_data_ver ckin_rem_types)'>
        <row pr_del='0'>
          <col>���3</col>
          <col>CKN3</col>
          <col>3</col>
          <col>���毨⠭��</col>
          <col>����ઠ ॣ����樨 3</col>
          <col>Ckin rem 3</col>
          <col>0</col>
          <col>$(get id_ckin_rem3)</col>
        </row>
        <row pr_del='1'>
          <col>���4</col>
          <col>CKN4</col>
          <col>4</col>
          <col>���. ������</col>
          <col>����ઠ ॣ����樨 4</col>
          <col>Ckin rem 4</col>
          <col>1</col>
          <col>$(get id_ckin_rem4)</col>
        </row>
        <row pr_del='1'>
          <col>���6</col>
          <col>CKN6</col>
          <col>6</col>
          <col>����� ���ᠦ��</col>
          <col>����ઠ ॣ����樨 6</col>
          <col>Ckin rem 6</col>
          <col>1</col>
          <col>$(get id_ckin_rem6)</col>
        </row>
        <row pr_del='1'>
          <col>���8</col>
          <col>CKN8</col>
          <col>8</col>
          <col>��᫮த</col>
          <col>����ઠ ॣ����樨 8</col>
          <col>Ckin rem 8</col>
          <col>1</col>
          <col>$(get id_ckin_rem8)</col>
        </row>
      </rows>
    </data>
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)
  </answer>
</term>

### ������塞 ��ப� � comp_rem_types, ckin_rem_types, ����� �� ����� � �ਧ����� 㤠����� � �����㦠�� ���������

!! capture=on
$(cache PIKE RU COMP_REM_TYPES $(cache_iface_ver COMP_REM_TYPES) $(cache_data_ver comp_rem_types)
  insert code:���2 code_lat:CMP0 {name:����ઠ ���������� 0} {name_lat:Comp rem 0} priority:0)

>> lines=auto
      <rows tid='$(cache_data_ver comp_rem_types)'>
        <row pr_del='0'>
          <col>���2</col>
          <col>CMP0</col>
          <col>����ઠ ���������� 0</col>
          <col>Comp rem 0</col>
          <col>0</col>
          <col>$(get id_comp_rem2)</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE RU CKIN_REM_TYPES $(cache_iface_ver CKIN_REM_TYPES) $(cache_data_ver ckin_rem_types)
  insert code:���4 code_lat:CKN9 {name:����ઠ ॣ����樨 9} {name_lat:Ckin rem 9} grp_id:9 is_iata:0)

>> lines=auto
      <rows tid='$(cache_data_ver ckin_rem_types)'>
        <row pr_del='0'>
          <col>���4</col>
          <col>CKN9</col>
          <col>9</col>
          <col>������</col>
          <col>����ઠ ॣ����樨 9</col>
          <col>Ckin rem 9</col>
          <col>0</col>
          <col>$(get id_ckin_rem4)</col>
        </row>
      </rows>

### � ⥯��� ���� �����㦠�� ��������� comp_rem_types, ckin_rem_types

!! capture=on
$(cache PIKE EN COMP_REM_TYPES $(cache_iface_ver COMP_REM_TYPES) "")

>> lines=auto
      <rows tid='...'>
        <row pr_del='0'>
          <col>���1</col>
          <col>CMP1</col>
          <col>����ઠ ���������� 1</col>
          <col>Comp rem 1</col>
          <col>1</col>
          <col>$(get id_comp_rem1)</col>
        </row>
        <row pr_del='0'>
          <col>���2</col>
          <col>CMP0</col>
          <col>����ઠ ���������� 0</col>
          <col>Comp rem 0</col>
          <col>0</col>
          <col>$(get id_comp_rem2)</col>
        </row>
        <row pr_del='0'>
          <col>���5</col>
          <col>CMP5</col>
          <col>����ઠ ���������� 5</col>
          <col>Comp rem 5</col>
          <col>5</col>
          <col>$(get id_comp_rem5)</col>
        </row>
        <row pr_del='0'>
          <col>���6</col>
          <col>CMP6</col>
          <col>����ઠ ���������� 6</col>
          <col>Comp rem 6</col>
          <col>6</col>
          <col>$(get id_comp_rem6)</col>
        </row>
        <row pr_del='1'>
          <col>���7</col>
          <col>CMP7</col>
          <col>����ઠ ���������� 7</col>
          <col>Comp rem 7</col>
          <col>7</col>
          <col>$(get id_comp_rem7)</col>
        </row>
        <row pr_del='1'>
          <col>���8</col>
          <col>CMP8</col>
          <col>����ઠ ���������� 8</col>
          <col>Comp rem 8</col>
          <col>8</col>
          <col>$(get id_comp_rem8)</col>
        </row>
      </rows>

!! capture=on
$(cache PIKE EN CKIN_REM_TYPES $(cache_iface_ver CKIN_REM_TYPES) "")

>> lines=auto
      <rows tid='...'>
        <row pr_del='0'>
          <col>���3</col>
          <col>CKN3</col>
          <col>3</col>
          <col>Special meal</col>
          <col>����ઠ ॣ����樨 3</col>
          <col>Ckin rem 3</col>
          <col>0</col>
          <col>$(get id_ckin_rem3)</col>
        </row>
        <row pr_del='0'>
          <col>���4</col>
          <col>CKN9</col>
          <col>9</col>
          <col>More</col>
          <col>����ઠ ॣ����樨 9</col>
          <col>Ckin rem 9</col>
          <col>0</col>
          <col>$(get id_ckin_rem4)</col>
        </row>
        <row pr_del='0'>
          <col>���5</col>
          <col>CKN5</col>
          <col>5</col>
          <col>Seats in the compartment</col>
          <col>����ઠ ॣ����樨 5</col>
          <col>Ckin rem 5</col>
          <col>0</col>
          <col>$(get id_ckin_rem5)</col>
        </row>
        <row pr_del='1'>
          <col>���6</col>
          <col>CKN6</col>
          <col>6</col>
          <col>Passenger status</col>
          <col>����ઠ ॣ����樨 6</col>
          <col>Ckin rem 6</col>
          <col>1</col>
          <col>$(get id_ckin_rem6)</col>
        </row>
        <row pr_del='0'>
          <col>���7</col>
          <col>CKN7</col>
          <col>7</col>
          <col>Bonus program</col>
          <col>����ઠ ॣ����樨 7</col>
          <col>Ckin rem 7</col>
          <col>0</col>
          <col>$(get id_ckin_rem7)</col>
        </row>
        <row pr_del='1'>
          <col>���8</col>
          <col>CKN8</col>
          <col>8</col>
          <col>Oxygen</col>
          <col>����ઠ ॣ����樨 8</col>
          <col>Ckin rem 8</col>
          <col>1</col>
          <col>$(get id_ckin_rem8)</col>
        </row>
      </rows>

%%
### test 11
### CODESHARE_SETS
#########################################################################################

$(init_term)

$(login PIKE PIKE)


$(set twoDaysAgo   "$(date_format %d.%m.%Y  -2) 00:00:00")
$(set range1_from  "$(date_format %d.%m.%Y  +1) 00:00:00")
$(set range1_to    "$(date_format %d.%m.%Y  +6) 23:59:59")
$(set range2_from  "$(date_format %d.%m.%Y  +7) 00:00:00")
$(set range2_to    "$(date_format %d.%m.%Y +13) 23:59:59")
$(set range3_from  "$(date_format %d.%m.%Y +14) 00:00:00")
$(set range3_to    "$(date_format %d.%m.%Y +20) 23:59:59")
$(set range3_to2   "$(date_format %d.%m.%Y +27) 23:59:59")

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
  insert airline_oper:�� flt_no_oper:110 airline_mark:�� flt_no_mark:210 airp_dep:��� pr_mark_norms:1 pr_mark_bp:1 pr_mark_rpt:1 first_date:$(get twoDaysAgo) last_date:$(get range3_to) pr_denial:0
)

$(USER_ERROR_RESPONSE MSG.TABLE.FIRST_DATE_BEFORE_TODAY)


!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
  insert airline_oper:�� flt_no_oper:110 airline_mark:�� flt_no_mark:210 airp_dep:��� pr_mark_norms:1 pr_mark_bp:1 pr_mark_rpt:1 first_date:$(get range1_from) last_date:$(get range3_to) pr_denial:0
)
>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) "")
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer handle=...
    <interface id='cache'/>
    <data>
      <code>CODESHARE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='$(cache_data_ver CODESHARE_SETS)'>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col/>
          <col>$(get range1_from)</col>
          <col>$(get range3_to)</col>
          <col>0</col>
        </row>
      </rows>
    </data>
  </answer>
</term>

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
  insert airline_oper:�� flt_no_oper:110 airline_mark:�� flt_no_mark:210 airp_dep:��� pr_mark_norms:1 pr_mark_bp:1 pr_mark_rpt:1 first_date:$(get range2_from) last_date:$(get range2_to) pr_denial:0
)
>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) "")
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer handle=...
    <interface id='cache'/>
    <data>
      <code>CODESHARE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='$(cache_data_ver CODESHARE_SETS)'>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col/>
          <col>$(get range1_from)</col>
          <col>$(get range1_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col/>
          <col>$(get range2_from)</col>
          <col>$(get range2_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col/>
          <col>$(get range3_from)</col>
          <col>$(get range3_to)</col>
          <col>0</col>
        </row>
      </rows>
    </data>
  </answer>
</term>

$(set range1_id $(last_history_row_id codeshare_sets -02))
$(set range2_id $(last_history_row_id codeshare_sets -01))
$(set range3_id $(last_history_row_id codeshare_sets -00))

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
insert airline_oper:�� flt_no_oper:110 airline_mark:�� flt_no_mark:210 airp_dep:��� pr_mark_norms:1 pr_mark_bp:1 pr_mark_rpt:1 first_date:$(get range1_from) last_date:$(get range1_to) pr_denial:0 days:"1357"
insert airline_oper:�� flt_no_oper:110 airline_mark:�� flt_no_mark:210 airp_dep:��� pr_mark_norms:1 pr_mark_bp:1 pr_mark_rpt:1 first_date:$(get range2_from) last_date:$(get range2_to) pr_denial:0 days:"246"
insert airline_oper:�� flt_no_oper:110 airline_mark:�� flt_no_mark:210 airp_dep:��� pr_mark_norms:1 pr_mark_bp:1 pr_mark_rpt:1 first_date:$(get range3_from) last_date:$(get range3_to) pr_denial:0 days:"147"
)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) "")
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer handle=...
    <interface id='cache'/>
    <data>
      <code>CODESHARE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid='$(cache_data_ver CODESHARE_SETS)'>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>.2.4.6.</col>
          <col>$(get range1_from)</col>
          <col>$(get range1_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>1.3.5.7</col>
          <col>$(get range1_from)</col>
          <col>$(get range1_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>1.3.5.7</col>
          <col>$(get range2_from)</col>
          <col>$(get range2_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>.2.4.6.</col>
          <col>$(get range2_from)</col>
          <col>$(get range2_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>.23.56.</col>
          <col>$(get range3_from)</col>
          <col>$(get range3_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>1..4..7</col>
          <col>$(get range3_from)</col>
          <col>$(get range3_to)</col>
          <col>0</col>
        </row>
      </rows>
    </data>
  </answer>
</term>


$(set range4_id $(last_history_row_id codeshare_sets -04))
$(set range5_id $(last_history_row_id codeshare_sets -02))
$(set range6_id $(last_history_row_id codeshare_sets -00))

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
delete old_airline_oper:�� old_flt_no_oper:110 old_airline_mark:�� old_flt_no_mark:210 old_airp_dep:��� old_id:$(get range1_id)
delete old_airline_oper:�� old_flt_no_oper:110 old_airline_mark:�� old_flt_no_mark:210 old_airp_dep:��� old_id:$(get range2_id)
delete old_airline_oper:�� old_flt_no_oper:110 old_airline_mark:�� old_flt_no_mark:210 old_airp_dep:��� old_id:$(get range3_id)
)

>> lines=auto
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) "")
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer handle=...
    <interface id='cache'/>
    <data>
      <code>CODESHARE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid=...
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>1.3.5.7</col>
          <col>$(get range1_from)</col>
          <col>$(get range1_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>.2.4.6.</col>
          <col>$(get range2_from)</col>
          <col>$(get range2_to)</col>
          <col>0</col>
        </row>
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>1..4..7</col>
          <col>$(get range3_from)</col>
          <col>$(get range3_to)</col>
          <col>0</col>
        </row>
      </rows>
    </data>
  </answer>
</term>


!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
update old_airline_oper:�� old_flt_no_oper:110 old_airline_mark:�� old_flt_no_mark:210 old_airp_dep:��� old_id:$(get range6_id) last_date:$(get range1_from)
)

$(USER_ERROR_RESPONSE MSG.TABLE.INVALID_RANGE)

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
delete old_airline_oper:�� old_flt_no_oper:110 old_airline_mark:�� old_flt_no_mark:210 old_airp_dep:��� old_id:$(get range4_id)
delete old_airline_oper:�� old_flt_no_oper:110 old_airline_mark:�� old_flt_no_mark:210 old_airp_dep:��� old_id:$(get range5_id)
update old_airline_oper:�� old_flt_no_oper:110 old_airline_mark:�� old_flt_no_mark:210 old_airp_dep:��� old_id:$(get range6_id) last_date:$(get range3_to2)
)
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer handle=...
    <interface id='cache'/>
    <data>
      <code>CODESHARE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid=...
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>1..4..7</col>
          <col>$(get range3_from)</col>
          <col>$(get range3_to2)</col>
          <col>0</col>
        </row>
      </rows>
    </data>
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)
  </answer>
</term>

!! capture=on
$(cache PIKE RU CODESHARE_SETS $(cache_iface_ver CODESHARE_SETS) ""
insert airline_oper:�� flt_no_oper:110 airline_mark:�� flt_no_mark:210 airp_dep:��� pr_mark_norms:1 pr_mark_bp:1 pr_mark_rpt:1 first_date:$(get range3_from) last_date:$(get range3_to2) pr_denial:0 days:"1..4..7"
)
>> lines=auto
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer handle=...
    <interface id='cache'/>
    <data>
      <code>CODESHARE_SETS</code>
      <Forbidden>0</Forbidden>
      <ReadOnly>0</ReadOnly>
      <keep_locally>0</keep_locally>
      <keep_deleted_rows>0</keep_deleted_rows>
      <user_depend>1</user_depend>
      <rows tid=...
        <row pr_del='0'>
          <col>...
          <col>��</col>
          <col>��</col>
          <col>110</col>
          <col/>
          <col/>
          <col>���</col>
          <col>���</col>
          <col>��</col>
          <col>��</col>
          <col>210</col>
          <col/>
          <col/>
          <col>1</col>
          <col>1</col>
          <col>1</col>
          <col>...4..7</col>
          <col>$(get range3_from)</col>
          <col>$(get range3_to2)</col>
          <col>0</col>
        </row>
      </rows>
    </data>
$(MESSAGE_TAG MSG.CHANGED_DATA_COMMIT)
  </answer>
</term>

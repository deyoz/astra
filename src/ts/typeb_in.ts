include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)

# meta: suite typeb

### test 1
### PNL ¨§ ¤Άγε η αβ¥©. ‚β®ΰ ο η αβμ - ¤γ΅«¨ª β, α®¤¥ΰ¦ ι¨© PDM
#########################################################################################

$(init_term)

$(set_user_time_type LocalAirp PIKE)

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
1“/‚€‘
ENDPART1

<<
QU MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H
MOWKK2H
MOWKK3H
MOWKK4H
MOWKK5H
MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H
MOWKK7H
MOWKK8H
.TJMRM1T $(dd)1202
PDM
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
-$(get airp_arv)000Y
1’“/”…„
ENDPNL

$(set point_dep $(last_point_id_spp))

!! capture=on err=ignore
<?xml version='1.0' encoding='UTF-8'?>
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetTlgIn>
      <point_id>$(get point_dep)</point_id>
    </GetTlgIn>
  </query>
</term>

$(set pnl_parts
{      <tlg>
        <err_lst/>
        <id>...</id>
        <num>1</num>
        <type>PNL</type>
        <addr>MOWKK1H
</addr>
        <heading>.TJMRM1T $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
</heading>
        <body>-$(get airp_arv)000C
1“/‚€‘
</body>
        <ending>ENDPART1
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_history>0</is_history>
      </tlg>
      <tlg>
        <err_lst/>
        <id>...</id>
        <num>2</num>
        <type>PNL</type>
        <addr>QU MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H
MOWKK2H
MOWKK3H
MOWKK4H
MOWKK5H
MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H
MOWKK7H
MOWKK8H
</addr>
        <heading>.TJMRM1T $(dd)1202
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
</heading>
        <body>-$(get airp_arv)000Y
1’“/”…„
</body>
        <ending>ENDPNL
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_final_part>1</is_final_part>
        <is_history>0</is_history>
      </tlg>}
)

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
    <tlgs>
$(get pnl_parts)
    </tlgs>
  </answer>
</term>

$(set pax_id_01 $(get_pax_id $(get point_dep) “ ‚€‘))
$(set pax_id_02 $(get_pax_id $(get point_dep) ’“ ”…„))

<<
QU MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H
MOWKK2H
MOWKK3H
MOWKK4H
MOWKK5H
MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H
MOWKK7H
MOWKK8H
.TJMRM1T $(dd)1300
PDM
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
-$(get airp_arv)000Y
DEL
1’“/”…„
ADD
1’“/”…„
ENDADL

<<
MOWKK1H
.TJMRM1T $(dd)1300
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
-$(get airp_arv)000C
ADD
1“/‚€‘‹‰
DEL
1“/‚€‘
ENDPART1

$(set pax_id_01 $(get_pax_id $(get point_dep) “ ‚€‘‹‰))
$(set pax_id_02 $(get_pax_id $(get point_dep) ’“ ”…„))

!! capture=on err=ignore
<?xml version='1.0' encoding='UTF-8'?>
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetTlgIn>
      <point_id>$(get point_dep)</point_id>
    </GetTlgIn>
  </query>
</term>

$(set adl_parts
{      <tlg>
        <err_lst/>
        <id>...</id>
        <num>1</num>
        <type>ADL</type>
        <addr>MOWKK1H
</addr>
        <heading>.TJMRM1T $(dd)1300
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
</heading>
        <body>-$(get airp_arv)000C
ADD
1“/‚€‘‹‰
DEL
1“/‚€‘
</body>
        <ending>ENDPART1
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_history>0</is_history>
      </tlg>
      <tlg>
        <err_lst/>
        <id>...</id>
        <num>2</num>
        <type>ADL</type>
        <addr>QU MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H MOWKK1H
MOWKK2H
MOWKK3H
MOWKK4H
MOWKK5H
MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H MOWKK6H
MOWKK7H
MOWKK8H
</addr>
        <heading>.TJMRM1T $(dd)1300
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
</heading>
        <body>-$(get airp_arv)000Y
DEL
1’“/”…„
ADD
1’“/”…„
</body>
        <ending>ENDADL
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_final_part>1</is_final_part>
        <is_history>0</is_history>
      </tlg>}
)


>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
    <tlgs>
$(get pnl_parts)
$(get adl_parts)
    </tlgs>
  </answer>
</term>

%%

### test 2
### PNL ¨§ ¤Άγε η αβ¥©.  ¦¤ ο ¨§ η αβ¥© ―ΰ¨ε®¤¨β ¤Ά ¦¤λ
#########################################################################################

$(init_term)

$(set_user_time_type LocalAirp PIKE)

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
.TJMRMUT $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
-$(get airp_arv)000C
1“/‚€‘
ENDPART1

<<
MOWKK1H
.TJMRMUT $(dd)1200
PDM
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
-$(get airp_arv)000Y
1’“/”…„
ENDPNL

<<
MOWKK1H
.TJMRMUT $(dd)1200
PDM
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
-$(get airp_arv)000C
1“/‚€‘
ENDPART1

<<
MOWKK1H
.TJMRMUT $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
-$(get airp_arv)000Y
1’“/”…„
ENDPNL

$(set point_dep $(last_point_id_spp))
$(set pax_id_01 $(get_pax_id $(get point_dep) “ ‚€‘))
$(set pax_id_02 $(get_pax_id $(get point_dep) ’“ ”…„))

!! capture=on err=ignore
<?xml version='1.0' encoding='UTF-8'?>
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetTlgIn>
      <point_id>$(get point_dep)</point_id>
    </GetTlgIn>
  </query>
</term>

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
    <tlgs>
      <tlg>
        <err_lst/>
        <id>...</id>
        <num>1</num>
        <type>PNL</type>
        <addr>MOWKK1H
</addr>
        <heading>.TJMRMUT $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
</heading>
        <body>-$(get airp_arv)000C
1“/‚€‘
</body>
        <ending>ENDPART1
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_history>0</is_history>
      </tlg>
      <tlg>
        <err_lst/>
        <id>...</id>
        <num>2</num>
        <type>PNL</type>
        <addr>MOWKK1H
</addr>
        <heading>.TJMRMUT $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
</heading>
        <body>-$(get airp_arv)000Y
1’“/”…„
</body>
        <ending>ENDPNL
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_final_part>1</is_final_part>
        <is_history>0</is_history>
      </tlg>
      <tlg>
        <err_lst>
          <item>
            <no>1</no>
            <pos>0</pos>
            <len>0</len>
            <text>Telegram duplicated</text>
          </item>
        </err_lst>
        <id>...</id>
        <num>1</num>
        <type>PNL</type>
        <addr>MOWKK1H
</addr>
        <heading>.TJMRMUT $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
</heading>
        <body>-$(get airp_arv)000C
1“/‚€‘
</body>
        <ending>ENDPART1
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_history>0</is_history>
      </tlg>
      <tlg>
        <err_lst>
          <item>
            <no>1</no>
            <pos>0</pos>
            <len>0</len>
            <text>Telegram duplicated</text>
          </item>
        </err_lst>
        <id>...</id>
        <num>2</num>
        <type>PNL</type>
        <addr>MOWKK1H
</addr>
        <heading>.TJMRMUT $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
</heading>
        <body>-$(get airp_arv)000Y
1’“/”…„
</body>
        <ending>ENDPNL
</ending>
        <time_receive>$(date_format %d.%m.%Y) xx:xx:xx</time_receive>
        <is_final_part>1</is_final_part>
        <is_history>0</is_history>
      </tlg>
    </tlgs>
  </answer>
</term>

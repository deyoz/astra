include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/spp/write_dests_macro.ts)
include(ts/pnl/btm_ptm.ts)

# meta: suite typeb

$(defmacro LOAD_TLG
  tlg_text
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <LoadTlg>
      <tlg_text>$(tlg_text)</tlg_text>
    </LoadTlg>
  </query>
</term>}

})

### test 1
### PNL из двух частей. Вторая часть - дубликат, содержащий PDM
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
1ПУПКИН/ВАСЯ
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
1ТУПКИН/ФЕДЯ
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
1ПУПКИН/ВАСЯ
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
1ТУПКИН/ФЕДЯ
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

$(set pax_id_01 $(get_pax_id $(get point_dep) ПУПКИН ВАСЯ))
$(set pax_id_02 $(get_pax_id $(get point_dep) ТУПКИН ФЕДЯ))

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
1ТУПКИН/ФЕДЯ
ADD
1ТУПКИН/ФЕДОР
ENDADL

<<
MOWKK1H
.TJMRM1T $(dd)1300
ADL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
-$(get airp_arv)000C
ADD
1ПУПКИН/ВАСИЛИЙ
DEL
1ПУПКИН/ВАСЯ
ENDPART1

$(set pax_id_01 $(get_pax_id $(get point_dep) ПУПКИН ВАСИЛИЙ))
$(set pax_id_02 $(get_pax_id $(get point_dep) ТУПКИН ФЕДОР))

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
1ПУПКИН/ВАСИЛИЙ
DEL
1ПУПКИН/ВАСЯ
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
1ТУПКИН/ФЕДЯ
ADD
1ТУПКИН/ФЕДОР
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
### PNL из двух частей. Каждая из частей приходит дважды
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
1ПУПКИН/ВАСЯ
ENDPART1

<<
MOWKK1H
.TJMRMUT $(dd)1200
PDM
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
-$(get airp_arv)000Y
1ТУПКИН/ФЕДЯ
ENDPNL

<<
MOWKK1H
.TJMRMUT $(dd)1200
PDM
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART1
-$(get airp_arv)000C
1ПУПКИН/ВАСЯ
ENDPART1

<<
MOWKK1H
.TJMRMUT $(dd)1200
PNL
$(get airline)$(get flt_no)/$(ddmon +1 en) $(get airp_dep) PART2
-$(get airp_arv)000Y
1ТУПКИН/ФЕДЯ
ENDPNL

$(set point_dep $(last_point_id_spp))
$(set pax_id_01 $(get_pax_id $(get point_dep) ПУПКИН ВАСЯ))
$(set pax_id_02 $(get_pax_id $(get point_dep) ТУПКИН ФЕДЯ))

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
1ПУПКИН/ВАСЯ
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
1ТУПКИН/ФЕДЯ
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
1ПУПКИН/ВАСЯ
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
1ТУПКИН/ФЕДЯ
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

%%

### test 3
### SSM с трехзначным русским кодом
#########################################################################################

$(init_term)

$(LOAD_TLG capture=on
{MOWKK1H
.THSPROD 281352
SSM
UTC
28JAN00001E001/TRANSHOST/28JAN
NEW
ЛЩ9661
29JAN21 29JAN21 5
J МИ8 Y
АСР0430 МХЛ0630}
)

>> lines=auto
$(MESSAGE_TAG MSG.TLG.LOADED)

#<<
#MOWKK1H
#.THSPROD 281352
#SSM
#UTC
#28JAN00001E001/TRANSHOST/28JAN
#NEW
#ЛУК9661
#29JAN21 29JAN21 5
#J МИ8 Y
#АСР0430 МХЛ0630

%%

### test 4
### поиск неразобранных не type-b телеграмм
#########################################################################################

$(init_term)

<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P0001\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+MU+$(yymmdd):$(hhmi)+1569312526531++IAPI"
UNG+CUSRES+NIAC+MU+$(yymmdd):$(hhmi)+15693125265312+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA"
BGM+132"
RFF+TN:1909240821556284716"
RFF+AF:MU589"
DTM+189:$(yymmdd)1420:201"
DTM+232:$(yymmdd)0930:201"
LOC+125+SFO"
LOC+87+PVG"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:1234"
ERC+1Z"
UNT+13+11085B94E1F8FA"
UNE+1+15693125265312"
UNZ+1+1569312526531"

>>
UNB+SIRE:4+MU+NIAC+xxxxxx:xxxx+1569312526531++IAPI"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN"
BGM+312"
ERP+1"
ERC+118"
UNT+5+11085B94E1F8FA"
UNZ+1+1569312526531"

!! capture=on err=ignore
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <GetTlgIn2>
      <err_cls>1</err_cls>
      <tlg_num>$(last_tlg_num -1)</tlg_num>
      <pr_time_receive>1</pr_time_receive>
      <TimeReceiveFrom>$(date_format %d.%m.%Y -1) 00:00:00</TimeReceiveFrom>
      <TimeReceiveTo>$(date_format %d.%m.%Y +1) 00:00:00</TimeReceiveTo>
    </GetTlgIn2>
  </query>
</term>

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer...
    <tlgs>
      <tlg>
        <err_lst/>
        <id>$(last_tlg_num -1)</id>
        <num>1</num>
        <type/>
        <addr/>
        <heading/>
        <body>UNB+SIRE:4+NIAC+MU+xxxxxx:xxxx+1569312526531++IAPI'
UNG+CUSRES+NIAC+MU+xxxxxx:xxxx+15693125265312+UN+D:05B'
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA'
BGM+132'
RFF+TN:1909240821556284716'
RFF+AF:MU589'
DTM+189:xxxxxx1420:201'
DTM+232:xxxxxx0930:201'
LOC+125+SFO'
LOC+87+PVG'
ERP+2'
RFF+AVF:NY7HZZ'
RFF+ABO:1234'
ERC+1Z'
UNT+13+11085B94E1F8FA'
UNE+1+15693125265312'
UNZ+1+1569312526531'
</body>
        <ending/>
        <time_receive>xx.xx.xxxx xx:xx:xx</time_receive>
        <is_history>0</is_history>
      </tlg>
    </tlgs>
  </answer>
</term>

%%

### test 5
### очистка
#########################################################################################

$(clean_old_records)

%%

### test 6
### входные BTM/PTM
#########################################################################################

$(init_term)

$(set time_create1 $(dd)$(hhmi))       ### время создания первоначальных PTM/BTM
$(set time_create2 $(dd)$(hhmi -1m))   ### более ранние PTM/BTM - оставляем данные первоначальных PTM/BTM
$(set time_create3 $(dd)$(hhmi +1m))   ### более поздние PTM/BTM - удаляем данные первоначальных PTM/BTM, добавляем данные текущих

$(PTM_UT_576_KRR $(get time_create1))
$(set ptm_id1 $(last_typeb_in_id))
$(BTM_UT_576_KRR $(get time_create1))
$(set btm_id1 $(last_typeb_in_id))

$(PTM_UT_576_KRR $(get time_create2))
$(set ptm_id2 $(last_typeb_in_id))
$(BTM_UT_576_KRR $(get time_create2))
$(set btm_id2 $(last_typeb_in_id))

??
$(dump_table tlg_transfer fields="tlg_id" order="tlg_id" display="on")

>>
--------------------- tlg_transfer DUMP ---------------------
SELECT tlg_id FROM tlg_transfer ORDER BY tlg_id
$(echo "[$(get ptm_id1)] $(lf)" 6)\
$(echo "[$(get btm_id1)] $(lf)" 6)\
------------------- END tlg_transfer DUMP COUNT=12 -------------------
$()


$(PTM_UT_576_KRR $(get time_create3))
$(set ptm_id3 $(last_typeb_in_id))
$(BTM_UT_576_KRR $(get time_create3))
$(set btm_id3 $(last_typeb_in_id))

??
$(dump_table tlg_transfer fields="tlg_id" order="tlg_id" display="on")

>>
--------------------- tlg_transfer DUMP ---------------------
SELECT tlg_id FROM tlg_transfer ORDER BY tlg_id
$(echo "[$(get ptm_id3)] $(lf)" 6)\
$(echo "[$(get btm_id3)] $(lf)" 6)\
------------------- END tlg_transfer DUMP COUNT=12 -------------------
$()


$(set today $(date_format %d.%m.%Y +0))
$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point UT 576 100 44444 ""                   KRR "$(get today) 07:00")
  $(new_spp_point_last             "$(get today) 15:00" VKO ) })

$(set point_dep $(get_point_dep_for_flight UT 576 "" $(yymmdd) KRR))

!! capture=on err=ignore
<?xml version='1.0' encoding='UTF-8'?>
<term>
  <query handle='0' id='Telegram' ver='1' opr='PIKE' screen='TLG.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <GetTlgIn>
      <point_id>$(get point_dep)</point_id>
    </GetTlgIn>
  </query>
</term>

>> mode=regex
.*
      <tlg>
        <err_lst/>
        <id>$(get ptm_id1)</id>
        <num>1</num>
        <type>PTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get ptm_id1)</id>
        <num>2</num>
        <type>PTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get btm_id1)</id>
        <num>1</num>
        <type>BTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get btm_id1)</id>
        <num>2</num>
        <type>BTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get ptm_id2)</id>
        <num>1</num>
        <type>PTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get ptm_id2)</id>
        <num>2</num>
        <type>PTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get btm_id2)</id>
        <num>1</num>
        <type>BTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get btm_id2)</id>
        <num>2</num>
        <type>BTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get ptm_id3)</id>
        <num>1</num>
        <type>PTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get ptm_id3)</id>
        <num>2</num>
        <type>PTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get btm_id3)</id>
        <num>1</num>
        <type>BTM</type>
.*
      <tlg>
        <err_lst/>
        <id>$(get btm_id3)</id>
        <num>2</num>
        <type>BTM</type>
.*


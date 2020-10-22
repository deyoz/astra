include(ts/macro.ts)

# meta: suite libra


###
#   Описание: get_schedule
#
###
#########################################################################################

$(desc_test 1)

$(init_term)

#
# <?xml version='1.0' encoding='CP866'?>
# <term>
# <query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
#  <request>
#    <libra_id>2</libra_id>
#    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='get_schedule' date_begin='19.04.2020' date_end='21.04.2020'&gt;&lt;/root&gt;</data>
#  </request>
# </query>
# </term>
#


%%
###
#   Описание: astra_call/load_tlg
#
###
#########################################################################################

$(desc_test 2)

$(init_term)

$(sql "insert into TYPEB_ORIGINATORS(ID, ADDR, DESCR, FIRST_DATE, LAST_DATE, TID, PR_DEL) values (ID__SEQ.nextval, 'MOWKB1H', 'DEFAULT', sysdate - 10, sysdate + 10, TID__SEQ.nextval, 0)")

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;load_tlg&lt;/func&gt;&lt;args&gt;&lt;text&gt;This is telegram text&lt;/text&gt;&lt;/args&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}


%%
###
#   Описание: astra_call/bad
#
###
#########################################################################################

$(desc_test 3)

$(init_term)

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;bad&lt;/func&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}


%%
###
#   Описание: astra_call/get_user_info
#
###
#########################################################################################

$(desc_test 4)

$(init_term)

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;get_user_info&lt;/func&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}


%%
###
#   Описание: astra_call/load_doc
#
###
#########################################################################################

$(desc_test 5)

$(init_term)

{<?xml version='1.0' encoding='CP866'?>
<term>
<query handle='0' id='libra' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
  <request>
    <libra_id>2</libra_id>
    <data>&lt;?xml version='1.0' encoding='utf-8'?&gt;&lt;root name='astra_call'&gt;&lt;func&gt;load_doc&lt;/func&gt;&lt;args&gt;&lt;point_id&gt;4853359&lt;/point_id&gt;&lt;type&gt;LOADSHEET&lt;/type&gt;&lt;content&gt;L O A D S H E E T&lt;/content&gt;&lt;/args&gt;&lt;/root&gt;</data>
  </request>
</query>
</term>}

??
$(dump_table WB_MSG fields="point_id, msg_type, source" display="on")

>> lines=auto
[4853359] [LOADSHEET] [LIBRA] $()


%%
###
#   Описание: DEFERRED astra_call/load_tlg
#
###
#########################################################################################

$(desc_test 6)

$(init)

$(sql "insert into TYPEB_ORIGINATORS(ID, ADDR, DESCR, FIRST_DATE, LAST_DATE, TID, PR_DEL) values (ID__SEQ.nextval, 'MOWKB1H', 'DEFAULT', sysdate - 10, sysdate + 10, TID__SEQ.nextval, 0)")

$(sql {begin LIBRA.DEFERR_ASTRA_CALL2(to_clob('<?xml version=\"1.0\" encoding=\"utf-8\"?><root name=\"astra_call\"><func>load_tlg</func><args><text>This is telegram text</text></args></root>')); end;})

$(run_daemon astra_calls_handler)

??
$(dump_table DEFERRED_ASTRA_CALLS display="on" fields="time_handle")

>> lines=auto
[xx.xx.xx xx:xx:xx,xxxxxx] $()


$(run_daemon astra_calls_cleaner)

??
$(dump_table DEFERRED_ASTRA_CALLS display="on" fields="time_handle")

>> lines=auto
[xx.xx.xx xx:xx:xx,xxxxxx] $()


$(sql {update DEFERRED_ASTRA_CALLS set TIME_HANDLE = TIME_HANDLE - 1})

$(run_daemon astra_calls_cleaner)

??
$(dump_table DEFERRED_ASTRA_CALLS display="on" fields="time_handle")

>> mode=regex
.*ND DEFERRED_ASTRA_CALLS DUMP COUNT=0.*


%%
###
#   Описание: get_schedule
#
###
#########################################################################################

$(desc_test 7)

$(init_term)

$(PREPARE_SEASON_SCD ЮТ СОЧ ПРХ 298)
$(make_spp)
$(deny_ets_interactive ЮТ 298 СОЧ)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(sql "begin LIBRA.WRITE_AHM_FLIGHT_LOG_MSG($(get point_dep), 'ОПЕРАТОР AHM', 'МОВРОМ', 'СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ РЕДАКТОРА AHM'); end;")
$(sql "begin LIBRA.WRITE_BAL_FLIGHT_LOG_MSG($(get point_dep), 'ЦЕНТРОВЩИК', 'МОВРОМ', 'СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ КАЛЬКУЛЯТОРА ЦЕНТРОВКИ'); end;")

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ РЕДАКТОРА AHM</msg>

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>СООБЩЕНИЕ В ЖУРНАЛ РЕЙСА ОТ КАЛЬКУЛЯТОРА ЦЕНТРОВКИ</msg>

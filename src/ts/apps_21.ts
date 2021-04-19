include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/pax/boarding_macro.ts)
include(ts/spp/read_trips_macro.ts)

# meta: suite apps

###
#   ���� �1
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-����� �� ��ᠤ�� ������ ���ᠦ�� �室�� �� �६� ॣ����樨
#   APPS-����� �� �⬥�� ������ ���ᠦ�� �室�� �� �⬥�� ॣ����樨
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)


>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# ॣ������ 童�� �����

$(CHECKIN_CREW $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN �� RUS 2124134 RUS 11.05.1978 15.05.2025 M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///8501/B/10////////

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# �⬥�� ॣ����樨 童�� ����� (CICX 㩤�� ⮫쪮 �᫨ �� �⢥� �� CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN ��)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/10/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N/.*


# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

%%
#########################################################################################

###
#   ���� �2
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-������ �� ��ᠤ�� ���ᠦ�஢ �室�� � ������ ��室� PNL.
#   �� ॣ����樨 ������ 㦥 �� �室��
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep))

# �室�� CIRQ-������
$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)
# ��㫨�㥬 ��室 ��᪮�쪨� apps-�⢥⮢:

# �� ���ᠦ��� KURGINSKAYA/ANNA GRIGOREVNA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id13)/PRS/27/001/CZ/P/RU/RU/0319189298//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F//8501/B/1////////


# �� ���ᠦ��� SELIVANOV RUSLAN

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id4)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/2////////


# �� ���ᠦ��� TUMALI VALERII

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/3////////


# �� ���ᠦ��� ALIMOV TALGAT

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/4////////

# �� ���ᠦ��� KHASSENOVA ZULFIYA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id3)/PRS/27/001/CZ/P/KZ/KZ/N07298275//P//20210329////KHASSENOVA/ZULFIYA/19741106/F//8501/B/5////////

# �� ���ᠦ��� FUKS LIUDMILA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id38)/PRS/27/001/CZ/////////////////8502/D////////


$(set pax_id_TUMALI $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 $(date_format %d.%m.%Y +1y) M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/UKR/UKR/FA144642//P/$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144642//P/$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////


# ॣ������ ���ᠦ�� SELIVANOV RUSLAN � ��������� ���㬥�⮬
# apps ������ ����

$(set pax_id_SELIVANOV $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX $(get pax_id_SELIVANOV) $(get point_dep) $(get point_arv) �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 �� UA 12342131 UA 23.09.1983 $(date_format %d.%m.%Y +1y) M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/2/P/RUS/RUS/9205589611//P/$(yyyymmdd +1y)////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/12342131//P/$(yyyymmdd +1y)////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


!! capture=on
$(GET_EVENTS $(get point_dep))

>> mode=regex
.*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�. ��१����� ����������.</msg>.*
.*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� SELIVANOV. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�.</msg>.*

# ��室�� ADL � 㤠������ ���� ���ᠦ�஢
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/4/P/KAZ/KAZ/N11024936//P/$(yyyymmdd +1y)////ALIMOV/TALGAT/19960511/M///N/N/.*

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/5/P/KAZ/KAZ/N07298275//P/$(yyyymmdd +1y)////KHASSENOVA/ZULFIYA/19741106/F///N/N/.*

# ��室�� ADL � ���������� ������ �� ������ ���ᠦ���
$(INB_ADL_UT_CHG1PAX AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))


# �室�� ��� CICX/CIRQ, �� ������ �� �᫮��� ����祭�� �⢥�
# ࠭�� �� �⮬� ���ᠦ���!!!

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/RUS/RUS/0319189298//P/$(yyyymmdd +1y)////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/0319189297//P/$(yyyymmdd +1y)////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


# �����⨥ ३�. �� ����ன�� APPS �� ������ �室��� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)


%%
#########################################################################################

###
#   ���� �3
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-������ �� ��ᠤ�� ���ᠦ�஢ �室�� �� �ਢ離� ३�
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 inbound=true outbound=false)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(auto_set_craft $(get point_dep))

$(run_trip_task send_all_apps $(get point_dep))

# �室�� CIRQ-������
$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)


%%
#########################################################################################
###
#   ���� �4
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �� �ந�室�� APPS-����� �� ��ᠤ�� ������ ���ᠦ��  �� �६� ॣ����樨
#   �� �ந�室�� APPS-����� �� �⬥�� ������ ���ᠦ��  �� �⬥�� ॣ����樨
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=false outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

# ॣ������ 童�� �����

$(CHECKIN_CREW $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN �� RUS 2124134 RUS 11.05.1978 15.05.2025 M)

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# �⬥�� ॣ����樨 童�� ����� (CICX 㩤�� ⮫쪮 �᫨ �� �⢥� �� CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN ��)

# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

%%
#########################################################################################

###
#   ���� �5
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �� ��室�� APPS-������ �� ��ᠤ�� ���ᠦ�஢ � ������ ��室� PNL.
#   �� ॣ����樨 ������ 㦥 �� �室��
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
$()

$(run_trip_task send_apps $(get point_dep) "uncheck")

# �室�� CIRQ-������
# $(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)


# ��㫨�㥬 ��室 ��᪮�쪨� apps-�⢥⮢:

# �� ���ᠦ��� KURGINSKAYA/ANNA GRIGOREVNA

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id13)/PRS/27/001/CZ/P/RU/RU/0319189298//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F//8501/B/1////////


# �� ���ᠦ��� SELIVANOV RUSLAN

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id4)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/2////////


# �� ���ᠦ��� TUMALI VALERII

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/3////////


# �� ���ᠦ��� ALIMOV TALGAT

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/4////////


# �� ���ᠦ��� FUKS LIUDMILA

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id38)/PRS/27/001/CZ/////////////////8502/D////////


# ॣ������ ���ᠦ�� TUMALI VALERII � ���㬥�⮬ ��� � PNL
# apps �� ������ �� ���� ����, �� ������

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PCX/20/1/3/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*
# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////


# ॣ������ ���ᠦ�� SELIVANOV RUSLAN � ��������� ���㬥�⮬
# apps ������ ����

$(set pax_id $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 �� UA 12342131 UA 23.09.1983 20.12.2025 M)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PCX/20/1/2/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PRQ/22/1/P/UKR/UKR/12342131//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


# !! capture=on
# $(GET_EVENTS $(get point_dep))

# >> mode=regex
# .*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�. ��१����� ����������.</msg>.*
# .*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� SELIVANOV. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�.</msg>.*


# ��室�� ADL � 㤠������ ���� ���ᠦ�஢
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
CHECK_ALARM
$()


# ������ ���� �⬥��, �� �� �室�� - ������ ??
$(run_trip_task send_apps $(get point_dep) "uncheck")


# ��室�� ADL � ���������� ������ �� ������ ���ᠦ���
$(INB_ADL_UT_CHG1PAX AER PRG 298 $(ddmon +0 en))

??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
CHECK_ALARM
$()

$(run_trip_task send_apps $(get point_dep) "uncheck")

# �室�� ��� CICX/CIRQ, �� ������ �� �᫮��� ����祭�� �⢥�
# ࠭�� �� �⮬� ���ᠦ���!!!

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PCX/20/1/1/P/RUS/RUS/0319189298//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/110000/PRQ/22/1/P/RUS/RUS/0319189297//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


# �����⨥ ३�. �� ����ன�� APPS �� ������ �室��� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)


%%
#########################################################################################

###
#   ���� �6
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �� �室�� APPS-������ �� ��ᠤ�� ���ᠦ�஢ �� �ਢ離� ३�
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 inbound=false outbound=false)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(auto_set_craft $(get point_dep))

$(run_trip_task send_all_apps $(get point_dep) "uncheck")

# �室�� CIRQ-������
# $(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

%%
#########################################################################################
###
#   ���� �7
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �⢥� CIRS �����頥� �訡��. �������� apps ������ �� APPS_PAX_DATA. ��������� Alarm APPSCONFlICT
#   �� ����� CICX APPS �� � pax_id ���, 䫠� is_cancel=true. ����饭�� �� �ନ�����(�� ���� ���� ��� �㭪樨 typeOfAction)
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#################################  1 case
###### �஢�ਬ 㤠����� �� Apps Alarms �� ���� CICX � �� (���������饬 ����)

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/ERR/3/CZ/6999/AP ERROR: PL-SQL FAILED/

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. ������ ���� APPS_CONFLICT
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
APPS_CONFLICT
$()

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: �訡�� AP: �� 㤠���� �믮����� PL-SQL �����</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

# �� ������ CICX APPS
# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# �஢��塞 ���� �� � ���� ����� ��� ���ᦨ�. �� ������ ���� ��祣� ��᫥ 㤠�����
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

#################################
#################################

%%
#########################################################################################
###
#   ���� �8
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �⢥� CIRS �����頥� ����� D. ��������� Alarm APPS_NEGATIVE_DIRECTIVE
#   �� ����� CANCEL_PAX is_cancel=true, is_exists=true. APPS ����饭�� �� �ନ�����(�� ��ன ���� ��� �㭪樨 typeOfAction)
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#################################  2 case
###### �஢�ਬ 㤠����� �� Apps Alarms �� �������饬 ���� �� ���� CICX � �����⮬ �����)

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8502/D/1/////////

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. ����� D(P) ����� ���⠢���� ���� APPS_NEGATIVE_DIRECTIVE
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
APPS_NEGATIVE_DIRECTIVE
$()

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

# �� ������ CICX APPS
#>> lines=auto mode=regex
#.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# �஢��塞 ���� �� � ���� ����� ��� ���ᦨ�. �� ������ ���� ��祣� ��᫥ 㤠����� �� ��஬ case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

#################################
#################################

%%
#########################################################################################
###
#   ���� �9
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �⢥� CIRS ��ଠ�쭮 �����頥� ����� B. ��������� �����
#   �� ����� CICX � pax_id ���, 䫠� is_cancel=true, ����� ࠢ�� B. ����饭�� �ନ�����(�᫨ ����� B)
#   �� ��ன ���� ��� �㭪樨 typeOfAction
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#################################  2 case
###### �஢�ਬ 㤠����� �� Apps Alarms �� �������饬 ���� �� ���� CICX � �����⮬ �����)

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. �� ������ ����
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

# �஢��塞 ���� �� � ���� ����� ��� ���ᦨ�. �� ������ ���� ��祣� ��᫥ 㤠����� �� ��஬ case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

#################################

%%
#########################################################################################
###
#   ���� �10
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ��᫥ PNL CIRQ ����砥� ��� �⢥� � ����ᠬ� B � P
#   � ��砥 B - Update(CICX,CIRQ) � ��砥 P(U,E,T..)- New(CIRQ)
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))
$(set move_id $(get_move_id $(get point_dep)))

$(run_trip_task send_apps $(get point_dep))

$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))

# ���塞 ��ᯮ�� �⮡� is_the_same �� false.���� � �⮬ ��砥 �� �⠪ false, ��⮬� �� ���� pre_checkin ��᫥ PNL
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144500 UA 16.04.1968 25.06.2025 M)

#�⬥�� ���� � 䫠��� precheckin P ��⮬� �� �।��騩 CIRQ �� ��ࠢ��� �� ��ࠡ�⪥ PNL
$(CICX_21 P UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          1 P UKR UKR FA144642 P $(yyyymmdd +1y) TUMALI VALERII 19680416 M N N)
$(set cicx_msg_id $(capture 1))

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144500 P 20250625 TUMALI VALERII 19680416 M N N)
$(set cirq_msg_id $(capture 1))

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CICC:$(get cicx_msg_id)/PCC/26/001/CZ/P/UKR/UKR/FA144642//P//$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M//8505/C///////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get cirq_msg_id)/PRS/27/001/CZ/P/UKR/UKR/FA144500//P//$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M//8507/U/1/////////

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set pax_id $(get_single_pax_id $(get point_dep) TUMALI VALERII))
$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

!!
$(UPDATE_PAX_PASSPORT $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144777 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N)


%%
#########################################################################################
###
#   ���� �11
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ��᫥ PNL CIRQ ����砥� ��� �⢥� � ����묨 ����ᠬ�
#   �� 7 ���� ��� �㭪樨 typeOfAction �஢�ઠ outOfSync
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))
$(set move_id $(get_move_id $(get point_dep)))

$(run_trip_task send_apps $(get point_dep))

$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

#<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
#CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M////1/////////

$(set pax_id_MYPASSENGER $(get_pax_id $(get point_dep) TUMALI VALERII))

# ���塞 ��ᯮ�� �⮡� is_the_same �� false.���� � �⮬ ��砥 �� �⠪ false, ��⮬� �� ���� pre_checkin ��᫥ PNL
!!
$(CHECKIN_PAX $(get pax_id_MYPASSENGER) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144500 UA 16.04.1968 25.06.2025 M)

#�஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. �� ������ ���� ��祣� ��᫥ 㤠����� �� ��஬ case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
APPS_CONFLICT
$()

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id16)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set pax_id $(get_single_pax_id $(get point_dep) TUMALI VALERII))
$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id_MYPASSENGER) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

## �஢��塞 ���� �� � ���� ����� ��� ���ᦨ�. �� ������ ���� ��祣� ��᫥ 㤠����� �� ��஬ case
??
$(check_pax_alarms $(get pax_id_MYPASSENGER))
>>
$()

%%
#########################################################################################

###
#   ���� �12
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �� ��室� ADL CICX ���뫠�� ⮫쪮 � ��砥 , �᫨ ���ᠦ�� �� ��ॣ����஢��
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

$(run_trip_task send_apps $(get point_dep))

$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

$(set pax_id_ALIMOV $(get_pax_id $(get point_dep) ALIMOV TALGAT))


<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/1////////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id3)/PRS/27/001/CZ/P/KZ/KZ/N07298275//P//20210329////KHASSENOVA/ZULFIYA/19741106/F//8501/B/5////////

# ��������㥬 ������ �� ���ᠦ�஢

!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep) $(get point_arv) �� 298 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M)

$(CICX_21 P UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
        1 P KAZ KAZ N11024936 P $(yyyymmdd +1y) ALIMOV TALGAT 19960511 M N N)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000 P KAZ KAZ N11024936
        P 20261004 ALIMOV TALGAT 19960511 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CICC:$(get cicx_msg_id)/PCC/26/001/CZ/P/KAZ/KAZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8505/C///////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get cirq_msg_id)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/88////////


# ��室�� ADL � 㤠������ ���� ���ᠦ�஢ , ���� �� ������ ��ॣ����஢��(ALIMOV), � ��㣮� ���
# CICX ������ ⮫쪮 ��� ����ॣ����஢������ ���ᠦ��
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/5/P/KAZ/KAZ/N07298275//P/$(yyyymmdd +1y)////KHASSENOVA/ZULFIYA/19741106/F///N/N/.*

$(OK_TO_BOARD $(get point_dep) $(get pax_id_ALIMOV))

%%
#########################################################################################

###
#   ���� �13
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   � ॠ�쭮� ����� �� APPS-������ � �ਧ����� pre-checkin
#   (�.�. ������, ���뫠��� �� ��ࠡ�⪥ PNL/ADL)
#   SITA APPS �ᥣ�� �⢥砥� �訡���
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

$(run_trip_task send_apps $(get point_dep))

# �室�� CIRQ-������
$(CIRQ_61_UT_REQS_APPS_VERSION_21 UT 298 AER PRG)

# ��㫨�㥬 �⢥� �� ���ᠦ��� TUMALI VALERII

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id16)/ERR/3//5569/Participating countries cannot process these transactions/

# �஢��塞 �� ��᫥ �訡�� ���ᠦ�� 㤠�����

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

# CICX �� ��ࠢ�����
#>> lines=auto mode=regex
#.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/UKR/UKR/FA144642//P/$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////

$(NO_BOARD $(get point_dep) $(get pax_id))

%%
##########################################################################################################
###
#   ���� �14
#
#   ���ᠭ��: ���ᠦ�஢: 3,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-������ �� ��ᠤ�� ���ᠦ�஢ �室�� � ������ ��室� PNL c �࠭���� ���ᠦ�஬,
#   ����� ���� ��᫥ ३� PNL(�窠 O) � ���� �� ���뫠���� ��� ���ᦨ� � �窮�(I)
#   �� ॣ����樨 ������ 㦥 �� �室��
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(PREPARE_SEASON_SCD �� ��� ��� 190)
$(PREPARE_SEASON_SCD �� ��� ��� 450)

$(make_spp)

$(INB_PNL_UT_TRANSFER3 AMS LHR 450 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER2 PRG AMS 190 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER1 AER PRG 298 $(ddmon +0 en))


$(set point_dep_UT_298 $(last_point_id_spp 0))
$(set point_dep_UT_190 $(last_point_id_spp 1))
$(set point_dep_UT_450 $(last_point_id_spp 2))

$(deny_ets_interactive �� 298 ���)
$(deny_ets_interactive �� 190 ���)
$(deny_ets_interactive �� 450 ���)


# $(dump_table CRS_PAX fields="pax_id, surname, name, pnr_id, pr_del ")
# $(dump_table CRS_PNR fields="pnr_id, airp_arv, system, point_id ")
# $(dump_table TLG_BINDING)
# $(dump_table POINTS fields="point_id, airline, flt_no, airp, scd_out, suffix")
# $(dump_table APPS_PAX_DATA fields="pax_id, apps_pax_id, given_names, cirq_msg_id, cicx_msg_id, version, point_id")

$(run_trip_task send_apps $(get point_dep_UT_298))

# �室�� CIRQ-������
# $(CIRQ_3_UT_REQS_APPS_VERSION_21_TRANSFER UT 298 AER PRG)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/KAZ/KAZ/N11024936//P/20261004////ALIMOV/TALGAT/19960511/M///N/N////.*

$(set msg_id2 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/0306355301//P/20291005////KOBYLINSKIY/ALEKSEY/19861231/M///N/N////.*

$(set msg_id10 $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/Y////.*

$(set msg_id11 $(capture 1))

#$(dump_table APPS_PAX_DATA fields="pax_id, apps_pax_id, cirq_msg_id, cicx_msg_id, family_name")

$(run_trip_task send_apps $(get point_dep_UT_190))
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT190/PRG/AMS/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/Y////.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT190/PRG/AMS/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/Y////.*

$(run_trip_task send_apps $(get point_dep_UT_450))
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT450/AMS/LHR/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/0[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/N////.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT450/AMS/LHR/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/0[0-9]?0000/PRQ/22/1/P/////P/////OZ/OFER//U///N/N////.*

# >> lines=auto mode=regex
#.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/1[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/Y////.*

$(set msg_id13 $(capture 1))

# ��㫨�㥬 ��室 ��᪮�쪨� apps-�⢥⮢:

# �� ���ᠦ��� ALIMOV TALGAT

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id2)/PRS/27/001/CZ/P/KZ/KZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8501/B/4////////

# �� ���ᠦ��� KOBYLINSKIY ALEKSEY

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id10)/PRS/27/001/CZ/P/RUS/RUS/0306355301//P//20291005////KOBYLINSKIY/ALEKSEY/19861231/M//8501/B/5////////

# �� ���ᠦ��� OZ OFER

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id11)/PRS/27/001/CZ/////////////////8502/D////////



%%
########################################################################################
#   ���� �15
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-������ �� ��ᠤ�� ������ �࠭��୮�� ���ᠦ�� �室�� �� �६� �������� ॣ����樨
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(PREPARE_SEASON_SCD �� ��� ��� 190)
$(PREPARE_SEASON_SCD �� ��� ��� 450)

$(make_spp)

$(deny_ets_interactive �� 298 ���)
$(deny_ets_interactive �� 190 ���)
$(deny_ets_interactive �� 450 ���)

$(INB_PNL_UT_TRANSFER3 AMS LHR 450 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER2 PRG AMS 190 $(ddmon +0 en))
$(INB_PNL_UT_TRANSFER1 AER PRG 298 $(ddmon +0 en))

$(set point_dep_UT_298 $(last_point_id_spp 0))
$(set point_dep_UT_190 $(last_point_id_spp 1))
$(set point_dep_UT_450 $(last_point_id_spp 2))

$(set point_arv_UT_298 $(get_next_trip_point_id $(get point_dep_UT_298)))
$(set point_arv_UT_190 $(get_next_trip_point_id $(get point_dep_UT_190)))
$(set point_arv_UT_450 $(get_next_trip_point_id $(get point_dep_UT_450)))

# $(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep_UT_298))
$(auto_set_craft $(get point_dep_UT_190))
$(auto_set_craft $(get point_dep_UT_450))

# $(set move_id $(get_move_id $(get point_dep)))

$(set pax_id1 $(get_pax_id $(get point_dep_UT_298) OZ OFER))
$(set pax_id2 $(get_pax_id $(get point_dep_UT_190) OZ OFER))
$(set pax_id3 $(get_pax_id $(get point_dep_UT_450) OZ OFER))

!!
$(CHECKIN_PAX_TRANSFER $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� OZ OFER 2985523437721
                       $(get pax_id2) $(get point_dep_UT_190) $(get point_arv_UT_190) �� 190 ��� ��� OZ OFER
                       $(get pax_id3) $(get point_dep_UT_450) $(get point_arv_UT_450) �� 450 ��� ��� OZ OFER
                       �� UA 32427293 UA 16.04.1968 25.06.2025 M)


# ��ࠢ�� apps ��� ३� 298 � ��� , 䫠� �࠭��� Y � DEST.
$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M N Y)

# ��ࠢ�� apps ��� ३� 190 � ��� , 䫠� �࠭��� Y � DEST � ORIG.
$(CIRQ_21 "" UT 190 PRG AMS $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y Y "CHK/2/AER/UT298/")

# ��ࠢ�� apps ��� ३� 190 � ��������� , 䫠� �࠭��� Y � DEST � ORIG.
$(CIRQ_21 "" UT 190 PRG AMS $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y Y "CHK/2/AER/UT298/")

# ��ࠢ�� apps ��� ३� 450 � ��������� , 䫠� �࠭��� Y ORIG.
$(CIRQ_21 "" UT 450 AMS LHR $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 090000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y N "CHK/2/AER/UT298/")

# ��ࠢ�� apps ��� ३� 450 � ������ , 䫠� �࠭��� Y � ORIG.
$(CIRQ_21 "" UT 450 AMS LHR $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 090000
P UKR UKR 32427293 P 20250625 OZ OFER 19680416 M Y N "CHK/2/AER/UT298/")

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

# !! capture=on
# $(GET_EVENTS $(get point_dep_UT_298))

# >> lines=auto
#         <msg>����� �� ��ᠤ�㤫� ���ᠦ�� OZ. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

# $(set grp_id1 $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))
# $(set tid1 $(get_single_tid $(get point_dep_UT_298) OZ OFER))


# !!
# $(CANCEL_PAX $(get pax_id1) $(get grp_id1) $(get tid1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� OZ OFER 2986145115578 ��)


# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/1[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*



%%
#########################################################################################

###
#   ���� �16
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: ���(���� �������⥫쭮 ������ � ��)
#            ����� apps: 21
#
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_eds �� UTET UTDC)

$(init_apps �� �� APPS_21 closeout=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)


$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(ET_DISP_61_UT_REQS)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

# $(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(SAVE_ET_DISP $(get point_dep) 2986145115578 TUMALI VALERII �� UTDC UTET G4LK6W AER PRG)

!! err=ignore
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

>>
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) �� 2986145115578 1 CK xxxxxx ��� ��� 298)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2986145115578 1 CK)


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)


>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*


<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!! err=ignore
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

>>
$(TKCREQ_ET_COS UTDC UTET $(last_edifact_ref) �� 2986145115578 1 I xxxxxx ��� ��� 298)
<<
$(TKCRES_ET_COS UTET UTDC $(last_edifact_ref) 2986145115578 1 I)


>> lines=auto
    <kick req_ctxt_id...

!!
$(lastRedisplay)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*


%%
########################################################################################
###
#   ���� �17
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ���� �� ��ࠢ����� apps ������� ��� ������� ३�
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false )

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))
$(set pax_id_ALIMOV $(get_pax_id $(get point_dep) ALIMOV TALGAT))
$(set pax_id_ANNA $(get_pax_id $(get point_dep) KURGINSKAYA "ANNA GRIGOREVNA"))
$(set pax_id_SELIVANOV $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

##----------------------------------------------------------------------------------
# ���� ���ᠦ�� ��ॣ��, ���⮬� �� ���� �� �⬥�� ३� ��ࠢ���� CICX
!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

##----------------------------------------------------------------------------------
# ��ன ���ᠦ�� �� ��ॣ��, ��⮬� �� � �⢥� CIRS ����� D � ��� �訡�� 8502
# ���⮬� �� ���� �� �㦭� ᫠�� �⬥��
!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep) $(get point_arv) �� 298 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P KAZ KAZ N11024936 P 20261004 ALIMOV TALGAT 19960511 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/KAZ/KAZ/N11024936//P//20261004////ALIMOV/TALGAT/19960511/M//8502/D/2/////////

##----------------------------------------------------------------------------------
# ��⨩ ���ᠦ�� ��ॣ�� � �� ���� ��ࠢ����� �⬥��, ��ࠡ�⠥�
# ���⮬� �� ���� ᫠�� �⬥��

!!
$(CHECKIN_PAX $(get pax_id_ANNA) $(get point_dep) $(get point_arv) �� 298 ��� ��� KURGINSKAYA "ANNA GRIGOREVNA" 2982425690987 �� UA 12342131 UA 23.09.1983 20.12.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR 12342131 P 20251220 KURGINSKAYA "ANNA GRIGOREVNA" 19830923 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/0319189298//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19830923/F//8501/B/3////////

$(set grp_id $(get_single_grp_id $(get point_dep) KURGINSKAYA "ANNA GRIGOREVNA"))
$(set tid $(get_single_tid $(get point_dep) KURGINSKAYA "ANNA GRIGOREVNA"))

!!
$(CANCEL_PAX $(get pax_id_ANNA) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv)
    �� 298 ��� ��� KURGINSKAYA "ANNA GRIGOREVNA" 2982425690987 ��)

$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
       3 P UKR UKR 12342131 P 20251220 KURGINSKAYA "ANNA GRIGOREVNA" 19830923 M N N)


<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CICC:$(capture 1)/PCC/26/001/CZ/P/UKR/UKR/12342131//P//20251220////KURGINSKAYA/ANNA GRIGOREVNA/19830923/M//8506/D///////

#-----------------------------------------------------------------------------------------

# 4 ���ᠦ�� ��ॣ�� �� ���� ��襫 �⢥�, ��⮬ �����⨬ ��� ��, �� ���� ��室�� �⢥� 2 ࠧ
# � ⠡��� APPS_PAX_DATA ������ ���� ��� ����� �� 1�� ���ᠦ���
# �� ��� ������ ��ࠢ����� �⬥��?

!!
$(CHECKIN_PAX $(get pax_id_SELIVANOV) $(get point_dep) $(get point_arv) �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 �� RU 9205589611 RU 23.09.1983 20.12.2025 M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/3////////

$(set grp_id $(get_single_grp_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH"))
$(set tid    $(get_single_tid    $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH"))


# �������� ���㬥��
!!
$(UPDATE_PAX_PASSPORT $(get pax_id_SELIVANOV) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 �� UA FA144777 UA 16.04.1968 25.06.2025 M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144777//P/20250625////SELIVANOV/RUSLAN NAILYEVICH/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144777//P//20250625////SELIVANOV/RUSLAN NAILYEVICH/19680416/M//8501/B/4////////

#---------------------------------------------------------------------------------------

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

# ��ࠢ����� �⬥�� ����ᠦ����� ���ᠦ��
$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
       1 P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
        4 P UKR UKR FA144777 P 20250625 SELIVANOV "RUSLAN NAILYEVICH" 19680416 M N N)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

$(OK_TO_BOARD $(get point_dep) $(get pax_id))
$(OK_TO_BOARD $(get point_dep) $(get pax_id_SELIVANOV))

%%
#########################################################################################

###
#   ���� �18
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-������ �� �室�� ��⮬� �� pre_checkin 䫠� ���⠢��� � false
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=false pre_checkin=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep))


%%
########################################################################
###
#   ���� �19
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-����� �� ��ᠤ�� ������ ���ᠦ�� �࠭���� ३�
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD_TRANSIT �� ��� ��� ��� 298) #�� ��㪮�� �ࠣ�
$(make_spp)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get_next_trip_point_id $(get point_dep))))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

#$(dump_table POINTS fields="point_id, airline, flt_no, airp, scd_out, suffix")

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

### �� ���䨪�樨 SITA ���ॡ����� ���⠢���� �࠭���� 䫠� DEST � �࠭����� ३��

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 1[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N Y)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)


$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 1[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
          1 P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N Y)

# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

%%
#########################################################################################
###
#   ���� �20
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ���⠢����� Alarm APPS_NOT_SCD_IN_TIME �᫨ �� ३� �� �������� �६� �ਫ��
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)

$(PREPARE_SEASON_SCD_WITHOUT_ARRIVE_TIME �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(run_trip_task send_apps $(get point_dep))
$(run_trip_task check_alarm_apps_scdtime $(get point_dep))

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. ������ ���� APPS_NOT_SCD_IN_TIME
??
$(check_trip_alarms $(get point_dep))
>>
APPS_NOT_SCD_IN_TIME
$()

# �������� ����, ⮫쪮 ��� ���筮�� ����� �����
$(UPDATE_SPP_FLIGHT $(get point_dep) $(get point_arv) �� ��� ��� 298 $(get move_id))

$(run_trip_task check_alarm_apps_scdtime $(get point_dep))

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. �� ������ ���� APPS_NOT_SCD_IN_TIME
??
$(check_trip_alarms $(get point_dep))
>>
$()

#################################

%%
#########################################################################################

###
#   ���� �21
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-������ �� �室�� ��⮬� �� ३� �� ����㭠த��
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER VKO 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep) "uncheck")


%%
#########################################################################################
###
#   ���� �22
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ����� �� ������ ���⠢������, �᫨  ��� pnl �� 㤮���⢮��� �᫮��� ( now - data > 2)
#   �᫨ ��� ��襤�襣� ����� � ���饬 , � ����� �ᥣ�� ���⠢�����
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################
#������� ����ன�� ��� ��࠭ ����� ३ᮢ

$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)

#�஢�ઠ ����.
#���砫� ������ ३� ��⮬ ��襫 PNL , SEND_NEW_APPS_INFO(tlg)

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))
$(make_spp $(ddmmyy -3))
$(deny_ets_interactive �� 300 ���)

$(INB_PNL_UT AMS PRG 300 $(ddmon -3 en))
$(set point_dep $(last_point_id_spp))


# �஢��塞 ���� �� � ���� ����� ��� �����. �� ������ ����
??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
$()

$(INB_PNL_UT PRG AMS 100 $(ddmon -1 en))

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))
$(make_spp $(ddmmyy -1))
$(deny_ets_interactive �� 100 ���)

$(set point_dep $(last_point_id_spp))

# �஢��塞 ���� �� � ���� ����� ��� �����.
??
$(check_flight_tasks $(get point_dep))
>>
SYNC_ALL_CHKD
SEND_ALL_APPS_INFO
EMD_REFRESH
$()


$(PREPARE_SEASON_SCD �� ��� ��� 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))
$(make_spp $(ddmmyy +11))
$(deny_ets_interactive �� 200 ���)

$(INB_PNL_UT PRG AMS 200 $(ddmon +11 en))

$(set point_dep $(last_point_id_spp))

# �஢��塞 ���� �� � ���� ����� ��� �����.
??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
SEND_ALL_APPS_INFO
SEND_NEW_APPS_INFO
$()

%%
#########################################################################################
###
#   ���� �23
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ���������� ����� SEND_ALL_APPS_INFO, ����� �������� ����ன��, � �� ����������, ����� �� ��������
#   �� ���������� ����� SEND_NEW_APPS_INFO, ����� �� �������� APPS ����ன��
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

# ����஥� ����
# $(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)

#� ਢ離� ३� �� ࠭�� ����������� PNL, SEND_ALL_APPS_INFO(flt_binding)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))
$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(set point_dep $(last_point_id_spp))

# �஢��塞 ���� �� � ���� ����� ��� �����. �� ������ ���� SEND_ALL_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>>
SYNC_ALL_CHKD
EMD_REFRESH
$()

#���砫� ������ ३� ��⮬ ��襫 PNL , SEND_NEW_APPS_INFO(tlg)
$(PREPARE_SEASON_SCD �� ��� ��� 190)
$(make_spp)
$(deny_ets_interactive �� 190 ���)

$(INB_PNL_UT PRG AMS 190 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))

# �஢��塞 ���� �� � ���� ����� ��� �����. �� ������ ���� SEND_NEW_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>>
EMD_REFRESH
SYNC_ALL_CHKD
$()

# ������� ����ன��
$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=true outbound=true)

#�ਢ離� ३� �� ࠭�� ����������� PNL, SEND_ALL_APPS_INFO(flt_binding)
$(INB_PNL_UT AER AMS 100 $(ddmon +0 en))
$(PREPARE_SEASON_SCD �� ��� ��� 100)

$(make_spp)
$(deny_ets_interactive �� 100 ���)

$(set point_dep $(last_point_id_spp))

# �஢��塞 ���� �� � ���� ����� ��� �����. ������ ���� SEND_ALL_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>> lines=auto
SEND_ALL_APPS_INFO


#���砫� ������ ३� ��⮬ ��襫 PNL , SEND_NEW_APPS_INFO(tlg)
$(PREPARE_SEASON_SCD �� ��� ��� 200)
$(make_spp)
$(deny_ets_interactive �� 200 ���)

$(INB_PNL_UT AMS PRG 200 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))

# �஢��塞 ���� �� � ���� ����� ��� �����. ������ ���� SEND_NEW_APPS_INFO
??
$(check_flight_tasks $(get point_dep))
>> lines=auto
SEND_ALL_APPS_INFO
SEND_NEW_APPS_INFO

#################################

%%
###
#   ���� �24
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ���뫪� ����ᮢ � ������ ��������� ���ଠ樨 �� ���ᠦ��� �� ��ᠤ��
#   ������ ���뫠���� APPS ����� �� ���������
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

# �⢥� �� ���ᠦ��� TUMALI VALERII

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/3////////


# ����������� �� ���ᠦ��� FA144642 -> FA144643
$(UPDATE_PAX_ON_BOARDING $(get pax_id) $(get point_dep) $(get tid) RUS FA144643 UA 16.04.1968 25.06.2025 M TUMALI VALERII)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/3/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

$(set cicx_msg_id $(capture 1))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/RUS/FA144643//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CICC:$(get cicx_msg_id)/PCC/26/001/CZ/P/UKR/UKR/FA144642//P//$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M//8505/C///////

$(NO_BOARD $(get point_dep) $(get pax_id))

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144643//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

$(OK_TO_BOARD $(get point_dep) $(get pax_id))

#####################################################################

#################################

%%
###
#   ���� �25
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �஢�ઠ ��९��뫪� ᮮ�饭��
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)


$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(set msg_id1 $(capture 1))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

# �� ������ �ந���� ��९��뫪� ��⮬� �� ��諮 ����� 10 ᥪ㭤 ��� �⢥�
$(update_msg $(get msg_id1) 5 2)
$(resend)

# ������ �ந���� ��९��뫪� ��⮬� �� ��諮 ����� 10 ᥪ㭤 ��� �⢥�
$(update_msg $(get msg_id1) 15 2)
$(resend)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

#������ �ந�室��� ��९��뫪� � ���⠢����� ALARM::APPSOutage , ��⮬� �� ������⢮ ��ࠢ�� ࠢ�� 5
$(update_msg $(get msg_id1) 20 6)
$(resend)


$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

??
$(check_trip_alarms $(get point_dep))
>>
APPS_OUTAGE
$()

$(NO_BOARD $(get point_dep) $(get pax_id))

#�� ������ �ந�室��� ��९��뫪� , ��⮬� �� ������⢮ ��ࠢ�� ࠢ�� 99
$(update_msg $(get msg_id1) 30 99)
$(resend)

#####################################################################
%%
###
#   ���� �26
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS-����� �� ��ᠤ�� ������ ���ᠦ�� �室�� �� �६� ॣ����樨
#   APPS-����� ��९��뫠���� ��� ⮣� �� ���ᠦ��, ����� ���� ६�ઠ OVRG
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8507/U/1/////////

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. ������ ���� APPS_ERROR
??
$(check_pax_alarms $(get pax_id))
>>
APPS_ERROR
$()

$(run_trip_task check_alarm_apps_problem $(get point_dep))
# �஢��塞 ���� �� � ���� ����� ��� ३�.
??
$(check_trip_alarms $(get point_dep))
>>
APPS_PROBLEM
$()


$(NO_BOARD $(get point_dep) $(get pax_id))

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set pax_id $(get_single_pax_id $(get point_dep) TUMALI VALERII))
$(set tid    $(get_single_tid    $(get point_dep) TUMALI VALERII))

#������ ���ᠦ�� � ��������묨 ����묨 , �⢥� � ����ᮬ U
!!
$(UPDATE_PAX_PASSPORT $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144777 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144777//P//20250625////TUMALI/VALERII/19680416/M//8507/U/1/////////

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. ������ ���� APPS_ERROR
??
$(check_pax_alarms $(get pax_id))
>>
APPS_ERROR
$()

$(run_trip_task check_alarm_apps_problem $(get point_dep))
??
$(check_trip_alarms $(get point_dep))
>>
APPS_PROBLEM
$()

$(NO_BOARD $(get point_dep) $(get pax_id))


$(set grp_tid    $(get_single_grp_tid    $(get point_dep) TUMALI VALERII))
$(set pax_tid    $(get_single_pax_tid    $(get point_dep) TUMALI VALERII))

#�ਭ㤨⥫쭠� ��९��뫪� apps �� ⮬� �� ���ᠦ��� , �⢥� � ����ᮬ B
!!
$(UPDATE_PAX_REM $(get pax_id) $(get grp_id) $(get grp_tid) $(get pax_tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII OVRG "OVRG AE")

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N "" GAE)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144777//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

#������ ���� ����襭� �� �ॢ���
??
$(check_pax_alarms $(get pax_id))
>>
$()

$(run_trip_task check_alarm_apps_problem $(get point_dep))
??
$(check_trip_alarms $(get point_dep))
>>
$()

$(OK_TO_BOARD $(get point_dep) $(get pax_id))
#$(BOARDING_REQUEST_BY_PAX_ID $(get point_dep) $(get pax_id) 777 "" 0)

$(set grp_tid    $(get_single_grp_tid    $(get point_dep) TUMALI VALERII))
$(set pax_tid    $(get_single_pax_tid    $(get point_dep) TUMALI VALERII))

#�ਭ㤨⥫쭠� ��९��뫪� apps �� ⮬� ��  ���ᠦ��� � ���������� ६�ન
!!
$(UPDATE_PAX_REM $(get pax_id) $(get grp_id) $(get grp_tid) $(get pax_tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII OVRA "OVRA AE")


$(CICX_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          1 P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144777 P 20250625 TUMALI VALERII 19680416 M N N "" AAE)

#$(OK_TO_BOARD $(get point_dep) $(get pax_id))

#####################################################################
%%
###
#   ���� �27
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   ��ࠡ�⪠ PNL ��� APPS �� ������ ������, �᫨ �㭪� �ਫ�� PNL �� ᮢ������ � ����� ������ �����
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER AMS 298 $(ddmon +0 en))
$(set point_dep $(last_point_id_spp))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

??
$(dump_table CRS_PAX fields="name, surname" where="name='OFER'" display=on)
>> lines=auto
[OFER] [OZ] $()

#################################

%%
###
#   ���� �28
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �஢�ઠ ��९��뫪� ᮮ�饭�� CIMR
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD_TRANSIT �� ��� ��� ��� 298) #�� ��㪮�� �ࠣ�
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get_next_trip_point_id $(get point_dep))))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

$(set msg_id1 $(capture 1))

# �� ������ �ந���� ��९��뫪� ��⮬� �� ��諮 ����� 10 ᥪ㭤 ��� �⢥�
$(update_msg $(get msg_id1) 5 2)
$(resend)

# ������ �ந���� ��९��뫪� ��⮬� �� ��諮 ����� 10 ᥪ㭤 ��� �⢥�
$(update_msg $(get msg_id1) 15 2)
$(resend)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

# ����୮ ������ �ந���� ��९��뫪� ��⮬� �� ��諮 ����� 10 ᥪ㭤 ��� �⢥�
$(update_msg $(get msg_id1) 15 2)
$(resend)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*


#������ �ந�室��� ��९��뫪� � ���⠢����� ALARM::APPSOutage , ��⮬� �� ������⢮ ��ࠢ�� ࠢ�� 5
$(update_msg $(get msg_id1) 20 6)
$(resend)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

??
$(check_trip_alarms $(get point_dep))
>>
APPS_OUTAGE
$()

#�� ������ �ந�室��� ��९��뫪� , ��⮬� �� ������⢮ ��ࠢ�� ࠢ�� 99
$(update_msg $(get msg_id1) 30 99)
$(resend)

########################################################

%%
########################################################################
###
#   ���� �29
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   APPS CIMR ��ࠡ�⪠ �⢥� CIMA � 㤠����� ᮮ�饭�� �� APPS_MESSAGES � APPS_MANIFEST_DATA
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=false inbound=false outbound=false)

$(PREPARE_SEASON_SCD_TRANSIT �� ��� ��� ��� 298) #�� ��㪮�� �ࠣ�
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get_next_trip_point_id $(get point_dep))))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set move_id $(get_move_id $(get point_dep)))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 10:15:00
              $(date_format %d.%m.%Y) 11:00:00)


>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/21/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*

#�⢥� �� ����� � �����⨨ � �訡���.
<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIMA:$(capture 1)/MAK/4/CZ/8701/6231/No movements found for this flight/

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� �����⨥ ३�. ������⤫� ��࠭� ��: ����� �⪫����. ��稭�:No movements found for this flight</msg>



%%
#########################################################################################
###
#   ���� �30
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 21
#
#   �⢥� CIRS �����頥� �訡��.  ��������� Alarm APPSCONFlICT. ���⮬� ��ᠤ�� ����� ����饭�
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)

$(INB_PNL_UT AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(PREPARE_FLIGHT $(get point_dep) AER �� 298 ���)

$(set move_id $(get_move_id $(get point_dep)))
$(set pax_id_1 $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_1) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/ERR/3/CZ/6999/AP ERROR: PL-SQL FAILED/

!! capture=on
$(GET_EVENTS $(get point_dep))
>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: �訡�� AP: �� 㤠���� �믮����� PL-SQL �����</msg>

$(NO_BOARD $(get point_dep) $(get pax_id_1))

# �஢��塞 ���� �� � ���� ����� ��� ���ᠦ��. ������ ���� APPS_CONFLICT
??
$(check_pax_alarms $(get pax_id_1))
>>
APPS_CONFLICT
$()

$(run_trip_task check_alarm_apps_problem $(get point_dep))
??
$(check_trip_alarms $(get point_dep))
>>
APPS_PROBLEM
$()

#############################################################################

$(set pax_id_SELIVANOV $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX $(get pax_id_SELIVANOV) $(get point_dep) $(get point_arv) �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 �� UA 12342131 UA 23.09.1983 $(date_format %d.%m.%Y +1y) M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/12342131//P/$(yyyymmdd +1y)////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/////////

$(OK_TO_BOARD $(get point_dep) $(get pax_id_SELIVANOV))

!! capture=on
$(GET_EVENTS $(get point_dep))
>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� SELIVANOV. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

??
$(check_pax_alarms $(get pax_id_SELIVANOV))
>>
$()

$(run_trip_task check_alarm_apps_problem $(get point_dep))
??
$(check_trip_alarms $(get point_dep))
>>
APPS_PROBLEM
$()

###########################################################################

$(set pax_id_ALIMOV $(get_pax_id $(get point_dep) ALIMOV TALGAT))
!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep) $(get point_arv) �� 298 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M)

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
P KAZ KAZ N11024936 P 20261004 ALIMOV TALGAT 19960511 M N N)

#�⢥� �� ��襫
$(NO_BOARD $(get point_dep) $(get pax_id_ALIMOV))

??
$(check_pax_alarms $(get pax_id_ALIMOV))
>>
$()

$(run_trip_task check_alarm_apps_problem $(get point_dep))
??
$(check_trip_alarms $(get point_dep))
>>
APPS_PROBLEM
$()

##########################################################################

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set grp_tid    $(get_single_grp_tid    $(get point_dep) TUMALI VALERII))
$(set pax_tid    $(get_single_pax_tid    $(get point_dep) TUMALI VALERII))

#���������� ���ଠ樨 �� ���ᠦ��� � ६�મ� ORVG AE � �⢥� � ����ᮬ B
!!
$(UPDATE_PAX_REM $(get pax_id_1) $(get grp_id) $(get grp_tid) $(get pax_tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII OVRG "OVRG AE")

$(CIRQ_21 "" UT 298 AER PRG $(yyyymmdd) 101500 $(yyyymmdd) 1[0-9]?0000
          P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N "" GAE)

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1/////////

#���� ���� APPS_CONFLICT � �ਯ ���� APPS_PROBLEM 㤠������.
??
$(check_pax_alarms $(get pax_id_1))
>>
$()

$(run_trip_task check_alarm_apps_problem $(get point_dep))
??
$(check_trip_alarms $(get point_dep))
>>
$()


$(OK_TO_BOARD $(get point_dep) $(get pax_id_1))

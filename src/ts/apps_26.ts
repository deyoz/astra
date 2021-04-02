include(ts/macro.ts)
include(ts/adm_macro.ts)
include(ts/pax/boarding_macro.ts)

# meta: suite apps

###
#   ���� �1
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
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

$(init_apps �� �� APPS_26 closeout=true inbound=false outbound=true)



$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX_WITH_VISA $(get pax_id) $(get point_dep) $(get point_arv)
                        �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                        UA FA144642 UA 16.04.1968 25.06.2025 M
                        123134 UA 20.10.2019 20.10.2030 CZ)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M///N/N///////00011////////PAD/13/1/CZ/V//123134/UKR/20301020///////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/29/001/CZ/P/UKR/UKR/FA144642//P//20250625/////TUMALI/VALERII/19680416/M//8501/B/1////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/00011/.*


# ॣ������ 童�� �����

$(CHECKIN_CREW_WITH_VISA $(get point_dep) $(get point_arv)
                         �� 298 ��� ��� VOLODIN SEMEN ��
                         RUS 2124134 RUS 11.05.1978 15.05.2025 M
                         34534534 RUS 21.10.2019 21.10.2030 CZ)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/C/RUS/RUS/2124134//P//20250515////VOLODIN/SEMEN/19780511/M///N/N///////00C11////////PAD/13/1/CZ/V//34534534/RUS/20301021///////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/29/001/CZ/C/RUS/RUS/2124134//P//20250515////VOLODIN/SEMEN/19780511/M///8501/B/10/////////

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# �⬥�� ॣ����樨 童�� ����� (CICX 㩤�� ⮫쪮 �᫨ �� �⢥� �� CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN ��)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/10/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N/00C11/.*


# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 09:15:00
              $(date_format %d.%m.%Y) 12:00:00)

>> lines=auto mode=regex
.*CIMR:([0-9]+)/UTUTA1/26/INM/3/UT298/PRG/$(yyyymmdd)/MRQ/3/CZ/C/C/.*


%%
#########################################################################################

###
#   ���� �2
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
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

$(init_apps �� �� APPS_26 closeout=false inbound=false outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep))

# �室�� CIRQ-������
$(CIRQ_61_UT_REQS_APPS_VERSION_26 UT 298 PRG AER $(yyyymmdd) $(yyyymmdd) 0[0-9]?1500 1[0-9]?0000)

# ��㫨�㥬 ��室 ��᪮�쪨� apps-�⢥⮢:

# �� ���ᠦ��� KURGINSKAYA/ANNA GRIGOREVNA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id13)/PRS/29/001/CZ/P/RU/RU/0319189298//P//20201008/////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F//8501/B/1////////


# �� ���ᠦ��� SELIVANOV RUSLAN

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id4)/PRS/29/001/CZ/P/RU/RU/9205589611//P//20251220/////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/2////////


# �� ���ᠦ��� TUMALI VALERII

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id16)/PRS/29/001/CZ/P/UA/UA/FA144642//P//20250625/////TUMALI/VALERII/19680416/M//8501/B/3////////


# �� ���ᠦ��� ALIMOV TALGAT

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id2)/PRS/29/001/CZ/P/KZ/KZ/N11024936//P//20261004/////ALIMOV/TALGAT/19960511/M//8501/B/4////////

# �� ���ᠦ��� KHASSENOVA ZULFIYA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id3)/PRS/29/001/CZ/P/KZ/KZ/N07298275//P//20210329/////KHASSENOVA/ZULFIYA/19741106/F//8501/B/5////////

# �� ���ᠦ��� FUKS LIUDMILA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id38)/PRS/29/001/CZ//////////////////8502/D////////


# ॣ������ ���ᠦ�� TUMALI VALERII � ���㬥�⮬ ��� � PNL
# apps �� ������ �� ���� ����, �� ������

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))


!!
$(CHECKIN_PAX_WITH_VISA_AND_DOCA $(get pax_id) $(get point_dep) $(get point_arv)
                                 �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                                 UA FA144642 UA 16.04.1968 25.06.2025 M
                                 213121 CZ 10.03.2019 10.03.2030 CZ D USA REGION ADDRESS CITY 1211)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/3/P/UKR/UKR/FA144642//P/$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M///N/N///////00011////////PAD/13/1/US/V//213121/CZE/20300310/ADDRESS/CITY/REGION/1211.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/29/001/CZ/P/UA/UA/FA144642//P//20250625/////TUMALI/VALERII/19680416/U//8509/X/////////


# ॣ������ ���ᠦ�� SELIVANOV RUSLAN � ��������� ���㬥�⮬
# apps ������ ����

$(set pax_id $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX_WITH_VISA $(get pax_id) $(get point_dep) $(get point_arv)
                        �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 ��
                        UA 12342131 UA 23.09.1983 20.12.2025 M
                        56573563 CZ 10.03.2019 10.03.2030 CZ)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/2/P/RUS/RUS/9205589611//P/$(yyyymmdd +1y)////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/12342131//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N///////00021////////PAD/13/1/CZ/V//56573563/CZE/20300310///////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/29/001/CZ/P/RU/RU/9205589611//P//20251220/////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


!! capture=on
$(GET_EVENTS $(get point_dep))

>> mode=regex
.*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�. ��१����� ����������.</msg>.*
.*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� SELIVANOV. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�.</msg>.*


# ��室�� ADL � 㤠������ ���� ���ᠦ�஢
$(INB_ADL_UT_DEL2PAXES PRG AER 298 $(ddmon +0 en))

# ������ ���� �⬥��, �� �� �室�� - ������ ??
$(run_trip_task send_apps $(get point_dep))

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/4/P/KAZ/KAZ/N11024936//P/$(yyyymmdd +1y)////ALIMOV/TALGAT/19960511/M///N/N/.*

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/5/P/KAZ/KAZ/N07298275//P/$(yyyymmdd +1y)////KHASSENOVA/ZULFIYA/19741106/F///N/N/.*

# ��室�� ADL � ���������� ������ �� ������ ���ᠦ���
$(INB_ADL_UT_CHG1PAX PRG AER 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))

# �室�� ��� CICX/CIRQ, �� ������ �� �᫮��� ����祭�� �⢥�
# ࠭�� �� �⮬� ���ᠦ���!!!

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/1/P/RUS/RUS/0319189298//P/$(yyyymmdd +1y)////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/RUS/RUS/0319189297//P//$(yyyymmdd +1y)////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


# �����⨥ ३�. �� ����ன�� APPS �� ������ �室��� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 09:15:00
              $(date_format %d.%m.%Y) 12:00:00)


%%
#########################################################################################

###
#   ���� �3
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   APPS-������ �� ��ᠤ�� ���ᠦ�஢ �室�� �� �ਢ離� ३�
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_26 inbound=false outbound=true)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(auto_set_craft $(get point_dep))

$(run_trip_task send_all_apps $(get point_dep))

# �室�� CIRQ-������
$(CIRQ_61_UT_REQS_APPS_VERSION_26 UT 298 PRG AER $(yyyymmdd) $(yyyymmdd) 0[0-9]?1500 1[0-9]?0000)

%%
#########################################################################################
###
#   ���� �4
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   �� �ந�室�� APPS-����� �� ��ᠤ�� ������ ���ᠦ�� �室�� �� �६� ॣ����樨
#   �� �ந�室�� APPS-����� �� �⬥�� ������ ���ᠦ�� �室�� �� �⬥�� ॣ����樨
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_26 closeout=true inbound=false outbound=true denial=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX_WITH_VISA $(get pax_id) $(get point_dep) $(get point_arv)
                        �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                        UA FA144642 UA 16.04.1968 25.06.2025 M
                        123134 UA 20.10.2019 20.10.2030 CZ)

# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M///N/N///////00011////////PAD/13/1/CZ/V//123134/UKR/20301020///////.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/29/001/CZ/P/UKR/UKR/FA144642//P//20250625/////TUMALI/VALERII/19680416/M//8501/B/1////////

# !! capture=on
# $(GET_EVENTS $(get point_dep))

# >> lines=auto
#        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/00011/.*


# ॣ������ 童�� �����

$(CHECKIN_CREW_WITH_VISA $(get point_dep) $(get point_arv)
                         �� 298 ��� ��� VOLODIN SEMEN ��
                         RUS 2124134 RUS 11.05.1978 15.05.2025 M
                         34534534 RUS 21.10.2019 21.10.2030 CZ)

# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/C/RUS/RUS/2124134//P//20250515////VOLODIN/SEMEN/19780511/M///N/N///////00C11////////PAD/13/1/CZ/V//34534534/RUS/20301021///////.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/29/001/CZ/C/RUS/RUS/2124134//P//20250515////VOLODIN/SEMEN/19780511/M///8501/B/10/////////

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# �⬥�� ॣ����樨 童�� ����� (CICX 㩤�� ⮫쪮 �᫨ �� �⢥� �� CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN ��)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/10/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N/00C11/.*


# �����⨥ ३�. �� ����ன�� APPS ������ �� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 09:15:00
              $(date_format %d.%m.%Y) 12:00:00)

# >> lines=auto mode=regex
# .*CIMR:([0-9]+)/UTUTA1/26/INM/3/UT298/AER/$(yyyymmdd)/MRQ/3/CZ/C/C/.*


%%
#########################################################################################

###
#   ���� �5
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   �� �ந�室�� APPS-������ �� ��ᠤ�� ���ᠦ�஢ �室�� � ������ ��室� PNL.
#   �� ॣ����樨 ������ 㦥 �� �室��
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_26 closeout=false inbound=false outbound=true denial=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(run_trip_task send_apps $(get point_dep) "uncheck")

# �室�� CIRQ-������
# $(CIRQ_61_UT_REQS_APPS_VERSION_26 UT 298 PRG AER)


# ��㫨�㥬 ��室 ��᪮�쪨� apps-�⢥⮢:

# �� ���ᠦ��� KURGINSKAYA/ANNA GRIGOREVNA

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id13)/PRS/29/001/CZ/P/RU/RU/0319189298//P//20201008/////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F//8501/B/1////////


# �� ���ᠦ��� SELIVANOV RUSLAN

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id4)/PRS/29/001/CZ/P/RU/RU/9205589611//P//20251220/////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8501/B/2////////


# �� ���ᠦ��� TUMALI VALERII

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id16)/PRS/29/001/CZ/P/UA/UA/FA144642//P//20250625/////TUMALI/VALERII/19680416/M//8501/B/3////////


# �� ���ᠦ��� ALIMOV TALGAT

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id2)/PRS/29/001/CZ/P/KZ/KZ/N11024936//P//20261004/////ALIMOV/TALGAT/19960511/M//8501/B/4////////


# �� ���ᠦ��� FUKS LIUDMILA

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(get msg_id38)/PRS/29/001/CZ//////////////////8502/D////////


# ॣ������ ���ᠦ�� TUMALI VALERII � ���㬥�⮬ ��� � PNL
# apps �� ������ �� ���� ����, �� ������

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX_WITH_VISA_AND_DOCA $(get pax_id) $(get point_dep) $(get point_arv)
                                 �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                                 UA FA144642 UA 16.04.1968 25.06.2025 M
                                 213121 CZ 10.03.2019 10.03.2030 CZ)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/3/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*
# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M///N/N///////00011////////PAD/13/1/CZ/V//213121/CZE/20300310/ADDRESS/CITY/REGION/112233.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/29/001/CZ/P/UA/UA/FA144642//P//20250625/////TUMALI/VALERII/19680416/U//8509/X/////////


# ॣ������ ���ᠦ�� SELIVANOV RUSLAN � ��������� ���㬥�⮬
# apps ������ ����

$(set pax_id $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX_WITH_VISA $(get pax_id) $(get point_dep) $(get point_arv)
                        �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 ��
                        UA 12342131 UA 23.09.1983 20.12.2025 M
                        56573563 CZ 10.03.2019 10.03.2030 CZ)

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/2/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/12342131//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

# << h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
# CIRS:$(capture 1)/PRS/29/001/CZ/P/RU/RU/9205589611//P//20251220/////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


# !! capture=on
# $(GET_EVENTS $(get point_dep))

# >> mode=regex
# .*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�. ��१����� ����������.</msg>.*
# .*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� SELIVANOV. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�.</msg>.*


# ��室�� ADL � 㤠������ ���� ���ᠦ�஢
$(INB_ADL_UT_DEL2PAXES PRG AER 298 $(ddmon +0 en))

# ������ ���� �⬥��, �� �� �室�� - ������ ??
$(run_trip_task send_apps $(get point_dep) "uncheck")


# ��室�� ADL � ���������� ������ �� ������ ���ᠦ���
$(INB_ADL_UT_CHG1PAX PRG AER 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep) "uncheck")

# �室�� ��� CICX/CIRQ, �� ������ �� �᫮��� ����祭�� �⢥�
# ࠭�� �� �⮬� ���ᠦ���!!!

# >> lines=auto mode=regex
# .*CICX:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/1/P/RUS/RUS/0319189298//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

# >> lines=auto mode=regex
# .*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/RUS/RUS/0319189297//P//20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


# �����⨥ ३�. �� ����ன�� APPS �� ������ �室��� CIMR

!! err=ignore
$(WRITE_DESTS $(get point_dep) $(get point_arv) $(get move_id) �� 298 ��� ���
              $(date_format %d.%m.%Y) 09:15:00
              $(date_format %d.%m.%Y) 12:00:00)


%%
#########################################################################################

###
#   ���� �6
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   �� �ந�室�� APPS-������ �� ��ᠤ�� ���ᠦ�஢ �室�� �� �ਢ離� ३�
#
###

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_26 inbound=false outbound=true denial=true)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(auto_set_craft $(get point_dep))

$(run_trip_task send_all_apps $(get point_dep) "uncheck")

# �室�� CIRQ-������
# $(CIRQ_61_UT_REQS_APPS_VERSION_26 UT 298 PRG AER)

%%
#########################################################################################
###
#   ���� �7
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   2 APPS-����� �� ��ᠤ�� ������ ���ᠦ�� �室�� �� �६� ॣ����樨
#   2 APPS-����� �� �⬥�� ������ ���ᠦ�� �室�� �� �⬥�� ॣ����樨
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_26 closeout=true inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)


$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX_WITH_VISA $(get pax_id) $(get point_dep) $(get point_arv)
                        �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                        UA FA144642 UA 16.04.1968 25.06.2025 M
                        123134 UA 20.10.2019 20.10.2030 CZ)

$(CIRQ_26 UT 298 PRG AER $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
        P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N
        00011 CZ V 123134 UKR 20301020)

$(set msg_id1 $(capture 1))

$(CIRQ_21 "" UT 298 PRG AER $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
        P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(set msg_id2 $(capture 1))

# �⢥� �� ���ᠦ��� TUMALI VALERII
<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id1)/PRS/29/001/CZ/P/UKR/UKR/FA144642//P//20250625/////TUMALI/VALERII/19680416/M//8501/B/1////////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id2)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/2////////

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/00011.*

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/2/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

%%
##############################################################################################
###
#   ���� �8
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

$(init_apps �� �� APPS_26 closeout=true inbound=true outbound=true)
$(init_apps �� �� APPS_21 closeout=true inbound=true outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))



!!
$(CHECKIN_PAX_WITH_VISA $(get pax_id) $(get point_dep) $(get point_arv)
                        �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                        UA FA144642 UA 16.04.1968 25.06.2025 M
                        123134 UA 20.10.2019 20.10.2030 CZ)

$(CIRQ_26 UT 298 PRG AER $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
        P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N
        00011 CZ V 123134 UKR 20301020)

$(set msg_id1 $(capture 1))

$(CIRQ_21 "" UT 298 PRG AER $(yyyymmdd) 0[0-9]?1500 $(yyyymmdd) 1[0-9]?0000
        P UKR UKR FA144642 P 20250625 TUMALI VALERII 19680416 M N N)

$(set msg_id2 $(capture 1))

# �⢥� �� ���ᠦ��� TUMALI VALERII
<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id1)/PRS/29/001/CZ/P/UKR/UKR/FA144642//P//20250625/////TUMALI/VALERII/19680416/M//8501/B/1////////

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id2)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/2////////

$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))


# ����������� �� ���ᠦ��� FA144642 -> FA144643

$(UPDATE_PAX_ON_BOARDING $(get pax_id) $(get point_dep) $(get tid) RUS FA144643 UA 16.04.1968 25.06.2025 M TUMALI VALERII 123134 UA 20.10.2019 20.10.2030 CZ)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/21/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/00011/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/RUS/FA144643//P//20250625////TUMALI/VALERII/19680416/M///N/N///////00011////////PAD/13/1/CZ/V//123134/UKR/20301020///////.*.*

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PCX/20/1/2/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/22/1/P/UKR/RUS/FA144643//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

#####################################################################

%%
###
#   ���� �9
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   APPS-����� �� ��ᠤ�� ������ ���ᠦ�� ��� ����(doco) �� � Docad �室�� �� �६� ॣ����樨
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

$(init_apps �� �� APPS_26 closeout=true inbound=false outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX_WITH_DOCA $(get pax_id) $(get point_dep) $(get point_arv)
                                 �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                                 UA FA144642 UA 16.04.1968 25.06.2025 M
                                 USA OHIO WillardAve Cleveland 44102)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M///N/N///////00011////////PAD/13/1/US//////WillardAve/Cleveland/OHIO/44102///.*

###################################################################################################
%%
###
#   ���� �10
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   APPS-����� �� ��ᠤ�� ������ ���ᠦ�� �室�� �� �६� ॣ����樨
#   Visa � DOCA ��࠭� �� �������, PAD ���� �� ������ ��ࠢ������
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps �� �� APPS_26 closeout=true inbound=false outbound=true)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)

$(INB_PNL_UT PRG AER 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX_WITH_DOCA $(get pax_id) $(get point_dep) $(get point_arv)
                                 �� 298 ��� ��� TUMALI VALERII 2986145115578 ��
                                 UA FA144642 UA 16.04.1968 25.06.2025 M
                                 "" OHIO WillardAve Cleveland 44102)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//26/INT/8/S/UT298/PRG/AER/$(yyyymmdd)/0[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M///N/N///////00011////////.*


###################################################################################################
%%
###
#   ���� �11
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   �� ����祭�� �����४⭮�� ���� DOCO country_issuance �� PNL � ������⢨� DOCA ,
#   APPS �� ��ࠢ����� ��� PAD(��� doco � doca)
#
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps �� �� APPS_26 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)


$(defmacro PNL_SVX
    depp=AER
    arrp=PRG
    fltno=298
    depd=$(ddmon +0 en)
    addr_to=MOWKK1H
    addr_from=TJMRMUT
{$(addr_to)
.$(addr_from) 091320
PNL
UT$(fltno)/$(depd) $(depp) PART1
-$(arrp)006L
1TUMALI/VALERII
.L/F50CF0/UT
.L/5Z21M5/1H
.R/TKNE HK1 2986145115578/1
.R/DOCS HK1/P/UA/FA144642/UA/16APR68/M/$(ddmonyy +1y)/TUMALI/VALERII
.R/DOCO HK1//V/5576/SVX
.R/PSPT HK1 ZAFA144642/UA/16APR68/TUMALI/VALERII/M
.R/FOID PPZAFA144642
ENDPNL}
)

<<
$(PNL_SVX AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))
$(run_trip_task send_apps $(get point_dep))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/1[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/FA144642//P//$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N///////00001////////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:([0-9]+)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1////////


###################################################################################################
%%
###
#   ���� �12
#
#   ���ᠭ��: ���ᠦ�஢: 61,
#             ���ࠪ⨢: �몫
#            ����� apps: 26
#
#   ���� doca D �� doco(����) c �����४�� ����� country_issuance, APPS ��ࠢ����� � PAD(⮫쪮 doca)
#
###
#########################################################################################

$(settcl APPS_H2H_ADDR APTXS)
$(settcl APPS_ROT_NAME APPGT)

$(init_term)

$(init_apps �� �� APPS_26 closeout=true inbound=true outbound=false)

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(make_spp)
$(deny_ets_interactive �� 298 ���)


$(defmacro PNL_DOCA
    depp=AER
    arrp=PRG
    fltno=298
    depd=$(ddmon +0 en)
    addr_to=MOWKK1H
    addr_from=TJMRMUT
{$(addr_to)
.$(addr_from) 091320
PNL
UT$(fltno)/$(depd) $(depp) PART1
-$(arrp)006L
1TUMALI/VALERII
.L/F50CF0/UT
.L/5Z21M5/1H
.R/TKNE HK1 2986145115578/1
.R/DOCS HK1/P/UA/FA144642/UA/16APR68/M/$(ddmonyy +1y)/TUMALI/VALERII
.R/DOCA HK1/D/UA
.R/DOCO HK1//V/5576/SVX
.R/PSPT HK1 ZAFA144642/UA/16APR68/TUMALI/VALERII/M
.R/FOID PPZAFA144642
ENDPNL}
)

<<
$(PNL_DOCA AER PRG 298 $(ddmon +0 en))

$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))
$(set move_id $(get_move_id $(get point_dep)))

$(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep))
$(run_trip_task send_apps $(get point_dep))

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/26/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/1[0-9]?1500/$(yyyymmdd)/1[0-9]?0000/PRQ/34/1/P/UKR/UKR/FA144642//P//$(yyyymmdd +1y)////TUMALI/VALERII/19680416/M///N/N///////00001////////PAD/13/1/UA////////////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:([0-9]+)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1////////


include(ts/macro.ts)

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

$(init_apps �� �� APPS_21 closeout=true)

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

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UKR/UKR/FA144642//P//20250625////TUMALI/VALERII/19680416/M//8501/B/1////////

!! capture=on
$(GET_EVENTS $(get point_dep))

>> lines=auto
        <msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ࠧ�襭�.</msg>

$(set grp_id $(get_single_grp_id $(get point_dep) TUMALI VALERII))
$(set tid $(get_single_tid $(get point_dep) TUMALI VALERII))

!!
$(CANCEL_PAX $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 ��)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*


# ॣ������ 童�� �����

$(CHECKIN_CREW $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN �� RUS 2124134 RUS 11.05.1978 15.05.2025 M)

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///8501/B/10////////

$(set grp_id $(get_single_grp_id $(get point_dep) VOLODIN SEMEN))
$(set pax_id $(get_single_pax_id $(get point_dep) VOLODIN SEMEN))
$(set tid $(get_single_tid $(get point_dep) VOLODIN SEMEN))

# �⬥�� ॣ����樨 童�� ����� (CICX 㩤�� ⮫쪮 �᫨ �� �⢥� �� CIRS)

!!
$(CANCEL_CHECKIN_CREW $(get pax_id) $(get grp_id) $(get tid) $(get point_dep) $(get point_arv) �� 298 ��� ��� VOLODIN SEMEN ��)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/10/C/RUS/RUS/2124134//P/20250515////VOLODIN/SEMEN/19780511/M///N/N/.*


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

$(init_apps �� �� APPS_21 closeout=false)

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


# �� ���ᠦ��� FUKS LIUDMILA

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(get msg_id38)/PRS/27/001/CZ/////////////////8502/D////////


# ॣ������ ���ᠦ�� TUMALI VALERII � ���㬥�⮬ ��� � PNL
# apps �� ������ �� ���� ����, �� ������

$(set pax_id $(get_pax_id $(get point_dep) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/3/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/UA/UA/FA144642//P//20250625////TUMALI/VALERII/19680416/U//8509/X/////////


# ॣ������ ���ᠦ�� SELIVANOV RUSLAN � ��������� ���㬥�⮬
# apps ������ ����

$(set pax_id $(get_pax_id $(get point_dep) SELIVANOV "RUSLAN NAILYEVICH MR"))

!!
$(CHECKIN_PAX $(get pax_id) $(get point_dep) $(get point_arv) �� 298 ��� ��� SELIVANOV "RUSLAN NAILYEVICH" 2985085963078 �� UA 12342131 UA 23.09.1983 20.12.2025 M)

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/2/P/RUS/RUS/9205589611//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N/.*
>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/UKR/UKR/12342131//P/20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M///N/N////.*

<< h2h=V.\VHLG.WA/I5APTXS/E5ASTRA/P002D\VGZ.\VUT/MOW/////////RU\$()
CIRS:$(capture 1)/PRS/27/001/CZ/P/RU/RU/9205589611//P//20251220////SELIVANOV/RUSLAN NAILYEVICH/19830923/M//8502/D/////////


!! capture=on
$(GET_EVENTS $(get point_dep))

>> mode=regex
.*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� TUMALI. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�. ��१����� ����������.</msg>.*
.*<msg>����� �� ��ᠤ�㤫� ���ᠦ�� SELIVANOV. ������⤫� ��࠭� ��: ��ᠤ�� ����饭�.</msg>.*


# ��室�� ADL � 㤠������ ���� ���ᠦ�஢
$(INB_ADL_UT_DEL2PAXES AER PRG 298 $(ddmon +0 en))

# ������ ���� �⬥��, �� �� �室�� - ������ ??
$(run_trip_task send_apps $(get point_dep))


# ��室�� ADL � ���������� ������ �� ������ ���ᠦ���
$(INB_ADL_UT_CHG1PAX AER PRG 298 $(ddmon +0 en))

$(run_trip_task send_apps $(get point_dep))

# �室�� ��� CICX/CIRQ, �� ������ �� �᫮��� ����祭�� �⢥�
# ࠭�� �� �⮬� ���ᠦ���!!!

>> lines=auto mode=regex
.*CICX:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/1/P/RUS/RUS/0319189298//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N/.*

>> lines=auto mode=regex
.*CIRQ:([0-9]+)/UTUTA1/N/P/21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/RUS/RUS/0319189297//P/20201008////KURGINSKAYA/ANNA GRIGOREVNA/19870602/F///N/N////.*


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

$(init_apps �� �� APPS_21)

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
#             ���ࠪ⨢: ���
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
.*CIRQ:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PRQ/22/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N////.*


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
.*CICX:([0-9]+)/UTUTA1/N//21/INT/8/S/UT298/AER/PRG/$(yyyymmdd)/101500/$(yyyymmdd)/100000/PCX/20/1/1/P/UKR/UKR/FA144642//P/20250625////TUMALI/VALERII/19680416/M///N/N/.*


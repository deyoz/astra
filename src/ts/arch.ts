include(ts/macro.ts)
include(ts/adm_macro.ts)

# meta: suite arch

#########################################################################################
###
#   ���� �1
#
#
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD �� ��� ��� 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +15))
$(make_spp $(ddmmyy +13))

$(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
$(set point_dep $(last_point_id_spp))
$(set point_arv $(get_next_trip_point_id $(get point_dep)))

$(set move_id $(get_move_id $(get point_dep)))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt, pr_del")

#�� ���� ��娢�樨 �१ 133 ��� �� ⥪�饩 ���� ��࠭���� ३�� , ����� �뫨 �१ 12 ����, �� ⥪�饩 ����
$(run_arch_step $(ddmmyy +133))

??
$(check_dump ARX_POINTS order="point_id" display="on")
>>
[$(get point_dep)] [$(get move_id)] [0] [���] [0] [NULL] [��] [300] [NULL] [��5] [NULL] [NULL] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] [NULL] [NULL] [�] [NULL] [NULL] [NULL] [NULL] [1] [0] [0] [0] [1] [NULL] [...] [$(date_format %d.%m.%Y +1)] $()
[$(get point_arv)] [$(get move_id)] [1] [���] [0] [$(get point_dep)] [NULL] [NULL] [NULL] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [NULL] [0] [0] [0] [NULL] [NULL] [NULL] [...] [$(date_format %d.%m.%Y +1)] $()
$()

%%
#########################################################################################
###
#   ���� �2
#   �஢�ઠ ��娢�樨 MOVE_ARX_EXT � ARX_MOVE_REF
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD �� ��� ��� 200 -1 TU5 $(date_format %d.%m.%Y -12) $(date_format %d.%m.%Y +12))

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
$(make_spp $(ddmmyy +20))

#$(run_arch_step $(ddmmyy +121))
#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")
#$(run_arch_step $(ddmmyy +131))
#$(dump_table MOVE_ARX_EXT fields = "move_id, part_key, date_range")

$(run_arch_step $(ddmmyy +151))

??
$(check_dump MOVE_ARX_EXT order="part_key" display="on")
>>
[2] [...] [$(date_format %d.%m.%Y +20)] $()
[2] [...] [$(date_format %d.%m.%Y +21)] $()
$()

??
$(check_dump ARX_MOVE_REF order="part_key" display="on")
>>
[...] [$(date_format %d.%m.%Y +1)] [NULL] $()
[...] [$(date_format %d.%m.%Y +20)] [NULL] $()
[...] [$(date_format %d.%m.%Y +21)] [NULL] $()
$()

%%
#########################################################################################
###
#   ���� �3
#   �஢�ઠ ��娢�樨 ARX_EVENTS
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(make_spp $(ddmmyy +1))
$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +12) $(date_format %d.%m.%Y +267))
$(make_spp $(ddmmyy +265))

$(run_arch_step $(ddmmyy +387))

??
$(check_dump ARX_EVENTS display="on" order="ev_order, lang")
>>
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Web check-in forbidden for flight.] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�� ३� ����饭� web-ॣ������.] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Preparation for check-in': scheduled time 00:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⮢�� � ॣ����樨': ����. �६� 00:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Check-in opening': scheduled time 04:14 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����⨥ ॣ����樨': ����. �६� 04:14 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in opening': scheduled time 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����⨥ web-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in opening': scheduled time 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����⨥ kiosk-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Check-in closing': scheduled time 06:35 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⨥ ॣ����樨': ����. �६� 06:35 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Ban of the cancel web check-in': scheduled time 06:25 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����� �⬥�� web-ॣ����樨': ����. �६� 06:25 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in closing': scheduled time 04:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⨥ web-ॣ����樨': ����. �६� 04:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in closing': scheduled time 05:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⨥ kiosk-ॣ����樨': ����. �६� 05:15 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Aircraft readiness for boarding': scheduled time 06:30 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '��⮢����� �� � ��ᠤ��': ����. �६� 06:30 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Boarding (doc. processing) completion': scheduled time 06:50 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����砭�� ��ᠤ�� (��ଫ���� ����.)': ����. �६� 06:50 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Rollback stairs': scheduled time 07:00 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�⪠� �࠯�': ����. �६� 07:00 $(date_format %d.%m.%y +1) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Passenger standard weight data input for the flight: ] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�����祭�� ��ᮢ ���ᠦ�஢ �� ३�: ] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [System task EMD_REFRESH <CloseCheckIn 0> created; Scheduled time: $(date_format %d.%m.%y +1) 06:35:00 (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [����� EMD_REFRESH <CloseCheckIn 0> ᮧ����; ����. ��.: $(date_format %d.%m.%y +1) 06:35:00 (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Web check-in forbidden for flight.] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�� ३� ����饭� web-ॣ������.] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Preparation for check-in': scheduled time 00:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⮢�� � ॣ����樨': ����. �६� 00:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Check-in opening': scheduled time 04:14 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����⨥ ॣ����樨': ����. �६� 04:14 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in opening': scheduled time 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����⨥ web-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in opening': scheduled time 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����⨥ kiosk-ॣ����樨': ����. �६� 07:15 $(date_format %d.%m.%y +264) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Check-in closing': scheduled time 06:35 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⨥ ॣ����樨': ����. �६� 06:35 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Ban of the cancel web check-in': scheduled time 06:25 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����� �⬥�� web-ॣ����樨': ����. �६� 06:25 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Web check-in closing': scheduled time 04:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⨥ web-ॣ����樨': ����. �६� 04:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Kiosk check-in closing': scheduled time 05:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�����⨥ kiosk-ॣ����樨': ����. �६� 05:15 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Aircraft readiness for boarding': scheduled time 06:30 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '��⮢����� �� � ��ᠤ��': ����. �६� 06:30 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'Boarding (doc. processing) completion': scheduled time 06:50 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '����砭�� ��ᠤ�� (��ଫ���� ����.)': ����. �६� 06:50 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Stage 'airstairs driving away': scheduled time 07:00 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�⠯ '�⣮� �࠯�': ����. �६� 07:00 $(date_format %d.%m.%y +265) (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Passenger standard weight data input for the flight: ] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [�����祭�� ��ᮢ ���ᠦ�஢ �� ३�: ] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [System task EMD_REFRESH <CloseCheckIn 0> created; Scheduled time: $(date_format %d.%m.%y +265) 06:35:00 (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [����� EMD_REFRESH <CloseCheckIn 0> ᮧ����; ����. ��.: $(date_format %d.%m.%y +265) 06:35:00 (UTC)] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Flight's statistics collection] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [���� ����⨪� �� ३��] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [���� ��६�饭 � ��娢] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [���� ��६�饭 � ��娢] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +1)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Flight's statistics collection] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [���� ����⨪� �� ३��] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [���� ��६�饭 � ��娢] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [EN] [Flight is transferred to archives] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
[...] [������� �.�.] [...] [NULL] [NULL] [RU] [���� ��६�饭 � ��娢] [1] [AIR.EXE] [������] [$(date_format %d.%m.%Y -3h)] [���] [NULL] [$(date_format %d.%m.%Y +265)] $()
$()

#$(dump_table ARX_EVENTS order="ev_order, lang)


%%
#########################################################################################
###
#   ���� �4
#   �஢�ઠ ��娢�樨 ARX_MARK_TRIPS � ARX_PAX_GRP
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
$(deny_ets_interactive �� 300 ���)
$(make_spp $(ddmmyy +1))
$(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
$(set point_dep_UT_300 $(last_point_id_spp))
$(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))

$(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) ALIMOV TALGAT))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))


!!
$(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) �� 300 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_300) $(get point_arv_UT_100) �� 300 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

#��娢������ ⮫쪮 ���-���
$(run_arch_step $(ddmmyy +122))
#$(check_dump ARX_MARK_TRIPS)

#��娢������ � ���-���
$(run_arch_step $(ddmmyy +141))


??
$(check_dump ARX_MARK_TRIPS display="on")
>>
[��] [���] [300] [...] [$(date_format %d.%m.%Y)] [NULL] [$(date_format %d.%m.%Y +1)] $()
$()

??
$(check_dump ARX_PAX_GRP display="on")
>>
[���] [���] [0] [�] [12] [TERM] [������] [0] [0] [...] [1] [0] [$(get point_arv_UT_300)] [$(get point_dep_UT_300)] [...] [0] [K] [...] [$(date_format %d.%m.%Y -3h)] [5] [NULL] [NULL] [$(date_format %d.%m.%Y +1)] $()
$()



%%
#########################################################################################
###
#   ���� �5
#   �஢�ઠ ��娢�樨  STAT ⠡���
#
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

# $(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
# $(deny_ets_interactive �� 300 ���)
# $(make_spp $(ddmmyy +1))
# $(INB_PNL_UT AMS PRG 300 $(ddmon +1 en))
# $(set point_dep_UT_300 $(last_point_id_spp))
# $(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))
# $(set pax_id_ALIMOV $(get_pax_id $(get point_dep_UT_300) TUMALI VALERII))
#
# !!
# $(CHECKIN_PAX $(get pax_id_ALIMOV) $(get point_dep_UT_300) $(get point_arv_UT_300) �� 300 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M KIOSK2)

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

# �� �㦭� ��� ⮣� �⮡� pr_brd = 1 � ⠡��� PAX,� ���� ��ᠤ��� ���ᠦ��
# � �� � ᢮� ��।� �㦭� �⮡� ����������� ⠡��� STAT_AD � ��⮬ ARX_STAT_AD
$(sql {INSERT INTO trip_hall(point_id, type, hall, pr_misc)
       VALUES($(get point_dep_UT_100), 101, NULL, 1)})

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

??
$(check_dump ARX_AGENT_STAT display="on")
>>
[0] [0] [0] [0] [������] [1] [0] [0] [0] [0] [0] [0] [0] [$(date_format %d.%m.%Y -3h)] [1] [3] [$(get point_dep_UT_100)] [0] [$(date_format %d.%m.%Y -3h)] [$(date_format %d.%m.%Y +20)] $()
$()


??
$(check_dump ARX_PFS_STAT order="pax_id" display="on")
>>
[���] [04.11.1960] [M] [VASILII LEONIDOVICH] [...] [F58457] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [VERGUNOV] [$(date_format %d.%m.%Y +20)] $()
[���] [11.05.1996] [M] [TALGAT] [...] [F57K6C] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [ALIMOV] [$(date_format %d.%m.%Y +20)] $()
[���] [06.11.1974] [F] [ZULFIYA] [...] [F57K6C] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KHASSENOVA] [$(date_format %d.%m.%Y +20)] $()
[���] [23.09.1983] [M] [RUSLAN NAILYEVICH MR] [...] [F56KFM] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [SELIVANOV] [$(date_format %d.%m.%Y +20)] $()
[���] [03.10.1972] [F] [����� �����������] [...] [F58262] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
[���] [21.06.1993] [M] [ANDREI] [...] [F522FC] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [AKOPOV] [$(date_format %d.%m.%Y +20)] $()
[���] [05.04.2010] [F] [KIRA] [...] [F522FC] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [BABAKHANOVA] [$(date_format %d.%m.%Y +20)] $()
[���] [23.07.1982] [F] [ANGELINA] [...] [F522FC] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [STIPIDI] [$(date_format %d.%m.%Y +20)] $()
[���] [31.12.1986] [M] [ALEKSEY MR] [...] [F55681] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KOBYLINSKIY] [$(date_format %d.%m.%Y +20)] $()
[���] [NULL] [NULL] [OFER] [...] [F554G3] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [OZ] [$(date_format %d.%m.%Y +20)] $()
[���] [22.10.1977] [F] [������] [...] [F543BB] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [�����] [$(date_format %d.%m.%Y +20)] $()
[���] [02.06.1987] [F] [ANNA GRIGOREVNA] [...] [F5659B] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KURGINSKAYA] [$(date_format %d.%m.%Y +20)] $()
[���] [14.05.1951] [M] [ODISSEI AFANASEVICH] [...] [F50266] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [BUMBURIDI] [$(date_format %d.%m.%Y +20)] $()
[���] [17.03.1990] [M] [KONSTANTIN ALEKSANDROVICH] [...] [F52MLM] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [CHEKMAREV] [$(date_format %d.%m.%Y +20)] $()
[���] [13.09.1984] [F] [KSENIYA VALEREVNA] [...] [F52MM0] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [VASILIADI] [$(date_format %d.%m.%Y +20)] $()
[���] [02.01.1959] [F] [RAISA GRIGOREVNA] [...] [F5203D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [ZAINULLINA] [$(date_format %d.%m.%Y +20)] $()
[���] [17.01.1966] [M] [�������� ����������] [...] [F514B8] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [������] [$(date_format %d.%m.%Y +20)] $()
[���] [30.12.1988] [M] [DENIS DMITRIEVICH] [...] [F4F4F0] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [AGAFONOV] [$(date_format %d.%m.%Y +20)] $()
[���] [07.08.1993] [F] [MARIIA DMITRIEVNA] [...] [F4F4F0] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [POLETAEVA] [$(date_format %d.%m.%Y +20)] $()
[���] [07.07.1979] [M] [DMITRII VLADIMIROVICH] [...] [F4F2D2] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [ASTAFEV] [$(date_format %d.%m.%Y +20)] $()
[���] [13.09.1987] [F] [KRISTINA VALEREVNA] [...] [F4F2D2] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [TIKHOMIROVA] [$(date_format %d.%m.%Y +20)] $()
[���] [31.08.1951] [M] [SERGEI MIKHAILOVICH] [...] [F4F617] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [BALASHOV] [$(date_format %d.%m.%Y +20)] $()
[���] [31.03.1952] [F] [EKATERINA SERGEEVNA] [...] [F50234] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [BUMBURIDI] [$(date_format %d.%m.%Y +20)] $()
[���] [03.02.2011] [M] [OLEG VIKTOROVICH] [...] [F4L8L3] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [MOKSOKHOEV] [$(date_format %d.%m.%Y +20)] $()
[���] [07.09.1981] [M] [VICTOR SERGEEVICH] [...] [F4L8L3] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [MOKSOKHOEV] [$(date_format %d.%m.%Y +20)] $()
[���] [12.10.1980] [F] [MARINA MRS] [...] [F4C271] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [RUBLEVA] [$(date_format %d.%m.%Y +20)] $()
[���] [24.08.1972] [F] [�������� �������������] [...] [F4DM92] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
[���] [27.06.1973] [F] [����� ������������] [...] [F4B34L] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
[���] [04.08.1978] [M] [���� ������������] [...] [F4D1G4] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
[���] [06.09.1974] [M] [������] [...] [F4CMMB] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [�������] [$(date_format %d.%m.%Y +20)] $()
[���] [07.02.1968] [M] [������� ������������] [...] [F4K2C3] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [����������] [$(date_format %d.%m.%Y +20)] $()
[���] [30.04.1979] [M] [������ ����������] [...] [F4BG9L] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
[���] [02.02.1982] [M] [PAVEL VALEREVICH MR] [...] [F0K77C] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [BUGAEV] [$(date_format %d.%m.%Y +20)] $()
[���] [21.12.1979] [F] [JULIA RAVILEVNA MS] [...] [F0K77C] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [CHETVERIKOVA] [$(date_format %d.%m.%Y +20)] $()
[���] [07.07.1990] [F] [LIUDMILA MS] [...] [F4828M] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [FUKS] [$(date_format %d.%m.%Y +20)] $()
[���] [26.01.1967] [F] [ALBINA VALENTINOVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[���] [24.07.2014] [F] [EKATERINA SERGEEVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[���] [24.07.2014] [F] [ELIZAVETA SERGEEVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[���] [24.07.2014] [M] [SERGEY SERGEEVICH] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[���] [01.01.1961] [M] [SERGEY VIKTOROVICH] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[���] [24.07.2014] [F] [SOFIYA SERGEEVNA] [...] [F3K91D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [KARUNA] [$(date_format %d.%m.%Y +20)] $()
[���] [07.11.1992] [M] [GEVORGSIMAVONOVICH MR] [...] [D8159D] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [MAKEYAN] [$(date_format %d.%m.%Y +20)] $()
[���] [01.09.1958] [F] [������] [...] [F36M42] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [�������] [$(date_format %d.%m.%Y +20)] $()
[���] [20.12.2014] [M] [����] [...] [F36M42] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [�������] [$(date_format %d.%m.%Y +20)] $()
[���] [28.12.1987] [F] [������] [...] [F36M42] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [������] [$(date_format %d.%m.%Y +20)] $()
[���] [23.04.1978] [F] [������� ����������] [...] [F29745] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [������] [$(date_format %d.%m.%Y +20)] $()
[���] [16.10.1963] [F] [�������] [...] [F44592] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [������] [$(date_format %d.%m.%Y +20)] $()
[���] [02.03.1975] [M] [������� ����������] [...] [F451B5] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [�������] [$(date_format %d.%m.%Y +20)] $()
[���] [24.10.1982] [F] [����� ���������] [...] [F3808K] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [������] [$(date_format %d.%m.%Y +20)] $()
[���] [23.08.1985] [F] [���� �������������] [...] [F43LF1] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [���������] [$(date_format %d.%m.%Y +20)] $()
[���] [21.11.2015] [M] [������ �����������] [...] [F43LF1] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [������] [$(date_format %d.%m.%Y +20)] $()
[���] [19.06.1967] [M] [����� �������������] [...] [F451F4] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [����������] [$(date_format %d.%m.%Y +20)] $()
[���] [27.05.1986] [M] [��������� ����������] [...] [F1K182] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [���������] [$(date_format %d.%m.%Y +20)] $()
[���] [11.01.1990] [F] [����� ���������] [...] [F47290] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [������������] [$(date_format %d.%m.%Y +20)] $()
[���] [14.07.2015] [F] [����� ���������] [...] [F2L743] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
[���] [12.04.1991] [F] [������� �������������] [...] [F2L743] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
[���] [25.05.1980] [M] [����� ������������] [...] [F2KFMB] [$(get point_dep_UT_100)] [1] [NOSHO] [�] [��������] [$(date_format %d.%m.%Y +20)] $()
$()

??
$(check_dump ARX_STAT_AD display="on")
>>
[NULL] [NULL] [�] [TERM] [������] [...] [F50CF0] [$(get point_dep_UT_100)] [$(date_format %d.%m.%Y +20)] [5�] [5D] [NULL] [$(date_format %d.%m.%Y +20)] $()
$()

??
$(check_dump ARX_STAT display="on")
>>
[1] [���] [0] [0] [0] [0] [0] [TERM] [0] [0] [0] [1] [0] [$(get point_dep_UT_100)] [N] [0] [0] [0] [0] [0] [1] [$(date_format %d.%m.%Y +20)] $()
$()


#??
#$(check_dump ARX_SELF_CKIN_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_RFISC_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_SERVICES display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_REM display="on")
#>> lines=auto
#??
#$(check_dump ARX_LIMITED_CAPABILITY_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_HA display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_VO display="on")
#>> lines=auto
#??
#$(check_dump ARX_STAT_REPRINT display="on")
#>> lines=auto
#??
#$(check_dump ARX_TRFER_PAX_STAT display="on")
#>> lines=auto
#??
#$(check_dump ARX_BI_STAT display="on")
#>> lines=auto

#??
#$(check_dump ARX_TRFER_STAT display="on")
#>> lines=auto


%%
#########################################################################################
###
#   ���� �6
#   �஢�ઠ ��娢�樨 TRIP ⠡���
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +141))

??
$(check_dump ARX_TRIP_CLASSES  order="cfg" display="on")
>>
[0] [11] [�] [$(get point_dep_UT_100)] [0] [$(date_format %d.%m.%Y +20)] $()
[0] [63] [�] [$(get point_dep_UT_100)] [0] [$(date_format %d.%m.%Y +20)] $()
$()

??
$(check_dump ARX_TRIP_SETS  display="on")
>>
[0] [...] [0] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [1] [0] [-1] $()
$()

??
$(check_dump ARX_TRIP_STAGES  order="stage_id" display="on")
>>
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [10] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [20] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +19)] [25] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +19)] [26] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [30] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +20)] [31] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [35] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [1] [0] [$(date_format %d.%m.%Y +20)] [36] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [40] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [50] $()
[NULL] [NULL] [$(date_format %d.%m.%Y +20)] [$(get point_dep_UT_100)] [0] [0] [$(date_format %d.%m.%Y +20)] [70] $()
$()

#??
#$(check_dump ARX_TRIP_DELAYS  display="on")
#>> lines=auto
#??
#$(check_dump ARX_TRIP_LOAD  display="on")
#>> lines=auto
#??
#$(check_dump ARX_CRS_DISPLACE2 display="on")
#>> lines=auto


%%
#########################################################################################
###
#   ���� �7
#   �஢�ઠ ��娢�樨 ⠡��� ARX_TCKIN_SEGMENTS
#
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

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

$(set grp_id1 $(get_single_grp_id $(get point_dep_UT_298) OZ OFER))
$(set grp_id2 $(get_single_grp_id $(get point_dep_UT_190) OZ OFER))
$(set grp_id3 $(get_single_grp_id $(get point_dep_UT_450) OZ OFER))

$(run_arch_step $(ddmmyy +141))

??
$(check_dump ARX_TCKIN_SEGMENTS order="grp_id, seg_no")
>>
[��] [���] [���] [190] [$(get grp_id1)] [$(date_format %d.%m.%Y)] [0] [$(date_format %d.%m.%Y)] [1] [NULL] $()
[��] [���] [���] [450] [$(get grp_id1)] [$(date_format %d.%m.%Y)] [1] [$(date_format %d.%m.%Y)] [2] [NULL] $()
[��] [���] [���] [450] [$(get grp_id2)] [$(date_format %d.%m.%Y)] [1] [$(date_format %d.%m.%Y)] [1] [NULL] $()
$()

#$(check_dump ARX_PAX)
#$(check_dump ARX_PAX_GRP)
#$(check_dump ARX_POINTS)
#$(check_dump ARX_STAT_SERVICES)
#$(check_dump ARX_TRANSFER)

%%
#########################################################################################
###
#   ���� �8
#   �஢�ઠ ��娢�樨 ⠡���� ARX_PAX_DOC
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +22))
$(INB_PNL_UT AER LHR 100 $(ddmon +22 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M)

$(run_arch_step $(ddmmyy +161))

??
$(check_dump ARX_PAX_DOC)
>>
[16.04.1968] [25.06.2025] [VALERII] [M] [UKR] [UKR] [FA144642] [$(get pax_id_TUMALI)] [0] [0] [NULL] [NULL] [TUMALI] [P] [NULL] [$(date_format %d.%m.%Y +22)] $()
$()

#??
#$(check_dump ARX_PAX_NORMS)
#>> lines=auto
#
#??
#$(check_dump ARX_PAX_REM)
#>> lines=auto
#
#??
#$(check_dump ARX_TRANSFER_SUBCLS)
#>> lines=auto
#
#
#??
#$(check_dump ARX_PAX_DOCO)
#>> lines=auto
#
#??
#$(check_dump ARX_PAX_DOCA)
#>> lines=auto


%%
#########################################################################################
###
#   ���� �9
#   �஢�ઠ ��娢�樨 SELF_CKIN_STAT
#
###
#########################################################################################

$(init_term)

$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30))
$(deny_ets_interactive �� 100 ���)
$(make_spp $(ddmmyy +20))
$(INB_PNL_UT AER LHR 100 $(ddmon +20 en))
$(set point_dep_UT_100 $(last_point_id_spp))
$(set point_arv_UT_100 $(get_next_trip_point_id $(get point_dep_UT_100)))
$(set pax_id_TUMALI $(get_pax_id $(get point_dep_UT_100) TUMALI VALERII))

$(init_kiosk)

!!
$(CHECKIN_PAX $(get pax_id_TUMALI) $(get point_dep_UT_100) $(get point_arv_UT_100) �� 100 ��� ��� TUMALI VALERII 2986145115578 �� UA FA144642 UA 16.04.1968 25.06.2025 M KIOSK2)

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(collect_flight_stat $(get point_dep_UT_100))

$(db_dump_table SELF_CKIN_STAT)

$(run_arch_step $(ddmmyy +141))

$(db_dump_table SELF_CKIN_STAT)

??
$(check_dump ARX_SELF_CKIN_STAT display="on")
>>
[1] [0] [0] [KIOSK] [����� ���������������  ��] [KIOSK2] [NULL] [$(get point_dep_UT_100)] [0] [0] [0] [0] [$(date_format %d.%m.%Y +20)] $()
$()


%%
#########################################################################################
###
#   ���� �10
#   �஢�ઠ ��娢�樨 ��� ⠡��� ��᫥ ࠡ��� arx_daily
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

#$(PREPARE_SEASON_SCD �� ��� ��� 300 -1 TU5 $(date_format %d.%m.%Y +1) $(date_format %d.%m.%Y +2))
#$(make_spp $(ddmmyy +1))
#$(PREPARE_SEASON_SCD �� ��� ��� 100 -1 TU5 $(date_format %d.%m.%Y +10) $(date_format %d.%m.%Y +30) 30.12.1899 31.12.1899)
#$(make_spp $(ddmmyy +20))


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


$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

$(run_arch $(ddmmyy +151))

$(dump_table POINTS fields="point_id, move_id, airline, flt_no, airp, scd_in, scd_out, est_in, est_out, act_in, act_out, time_in, time_out, airp_fmt")

??
$(check_dump ARX_PAX order="coupon_no" )
>>
[NULL] [�] [12] [�] [1] [0] [NULL] [...] [0] [0] [OFER] [$(date_format %d.%m.%Y )] [$(get pax_id1)] [��] [0] [0] [NULL] [1] [1] [5�] [NULL] [�] [OZ] [0] [2985523437721] [TKNE] [...] [NULL] $()
[NULL] [�] [12] [�] [2] [0] [NULL] [...] [0] [0] [OFER] [$(date_format %d.%m.%Y )] [$(get pax_id2)] [��] [0] [0] [NULL] [1] [1] [5�] [NULL] [�] [OZ] [0] [2985523437721] [TKNE] [...] [NULL] $()
[NULL] [�] [12] [�] [3] [0] [NULL] [...] [0] [0] [OFER] [$(date_format %d.%m.%Y )] [$(get pax_id3)] [��] [0] [0] [NULL] [1] [1] [5�] [NULL] [�] [OZ] [0] [2985523437721] [TKNE] [...] [NULL] $()
$()

??
$(check_dump ARX_TRFER_PAX_STAT)
>> lines=auto
[0] [0] [$(get pax_id1)] [$(get point_dep_UT_298)] [0] [$(date_format %d.%m.%Y)] [��,���,298,,$(date_format %d.%m.%Y) 07:15:00,���;��,���,190,,$(date_format %d.%m.%Y) 07:15:00,���;��,���,450,,$(date_format %d.%m.%Y) 07:15:00,���] [$(date_format %d.%m.%Y )] $()
$()

#�஢�ઠ ��� ⠡��� �ࠧ� ��������஢���, ��⮬� �� � १���� ��� ��� ����� � �஢����� ���� �� ����� ��᫠,
#���� �� ��娢��� 㦥 �஢�७� � ��㣨� ����

#$(check_dump ARX_POINTS)
#$(check_dump MOVE_ARX_EXT)
#$(check_dump ARX_MOVE_REF)
##$(check_dump ARX_EVENTS order="ev_order, lang")
#$(check_dump ARX_MARK_TRIPS)
#$(check_dump ARX_PAX_GRP)

#$(check_dump ARX_SELF_CKIN_STAT)
#$(check_dump ARX_RFISC_STAT)
#$(check_dump ARX_STAT_SERVICES)
#$(check_dump ARX_STAT_REM)
#$(check_dump ARX_LIMITED_CAPABILITY_STAT)
#$(check_dump ARX_PFS_STAT)
#$(check_dump ARX_STAT_AD)
#$(check_dump ARX_STAT_HA)
#$(check_dump ARX_STAT_VO)
#$(check_dump ARX_STAT_REPRINT)

#$(check_dump ARX_BI_STAT)
#$(are_agent_stat_equal)
#$(check_dump ARX_STAT)
#$(check_dump ARX_TRFER_STAT)
#$(check_dump ARX_TRIP_CLASSES)
#$(check_dump ARX_TRIP_DELAYS)
#$(check_dump ARX_TRIP_LOAD)
#$(check_dump ARX_TRIP_SETS)
#$(check_dump ARX_CRS_DISPLACE2)
#$(check_dump ARX_TRIP_STAGES)

#$(check_dump ARX_BAG_RECEIPTS)
#$(check_dump ARX_BAG_PAY_TYPES)

#$(check_dump ARX_ANNUL_BAG)
#$(check_dump ARX_ANNUL_TAGS)
#$(check_dump ARX_UNACCOMP_BAG_INFO)
#$(check_dump ARX_BAG2)
#$(check_dump ARX_BAG_PREPAY)
#$(check_dump ARX_PAID_BAG)
#$(check_dump ARX_VALUE_BAG)
#$(check_dump ARX_GRP_NORMS)

#$(check_dump ARX_TRANSFER)
#$(check_dump ARX_TCKIN_SEGMENTS)

#$(check_dump ARX_PAX_NORMS)
#$(check_dump ARX_PAX_REM)
#$(check_dump ARX_TRANSFER_SUBCLS)

#$(check_dump ARX_PAX_DOC)
#$(check_dump ARX_PAX_DOCO)
#$(check_dump ARX_PAX_DOCA)

##$(check_dump ARX_EVENTS order="ev_order, lang")
#$(check_dump ARX_TLG_OUT)
#$(check_dump ARX_STAT_ZAMAR)

#$(check_dump ARX_BAG_NORMS)
#$(check_dump ARX_BAG_RATES)
#$(check_dump ARX_VALUE_BAG_TAXES)
#$(check_dump ARX_EXCHANGE_RATES)

#$(check_dump ARX_TLG_STAT)


%%
#########################################################################################
###
#   ���� �11
#   �஢�ઠ ��ࠡ�⪨ �᪫�祭�� DUP_VAL_ON_INDEX
#
###
#########################################################################################


$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(PREPARE_SEASON_SCD �� ��� ��� 298)
$(PREPARE_SEASON_SCD �� ��� ��� 300)

$(make_spp)

$(deny_ets_interactive �� 298 ���)
$(deny_ets_interactive �� 300 ���)

$(INB_PNL_UT_MARK1 AER PRG 298 $(ddmon +0 en))
$(set point_dep_UT_298_tlg $(last_point_id_tlg 0))
$(set point_dep_UT_298 $(last_point_id_spp 0))
$(set point_arv_UT_298 $(get_next_trip_point_id $(get point_dep_UT_298)))


$(INB_PNL_UT_MARK2 AER PRG 300 $(ddmon +0 en))
$(set point_dep_UT_300_tlg $(last_point_id_tlg 0))
$(set point_dep_UT_300 $(last_point_id_spp 0))
$(set point_arv_UT_300 $(get_next_trip_point_id $(get point_dep_UT_300)))


# $(combine_brd_with_reg $(get point_dep))
$(auto_set_craft $(get point_dep_UT_298))

# $(set move_id $(get_move_id $(get point_dep)))

$(set pax_id1 $(get_pax_id $(get point_dep_UT_298) ALIMOV TALGAT))
$(set pax_id2 $(get_pax_id $(get point_dep_UT_300) OZ OFER))

!!
$(CHECKIN_PAX $(get pax_id1) $(get point_dep_UT_298) $(get point_arv_UT_298) �� 298 ��� ��� ALIMOV TALGAT 2982425696898 �� KZ N11024936 KZ 11.05.1996 04.10.2026 M)


!!
$(CHECKIN_PAX $(get pax_id2) $(get point_dep_UT_300) $(get point_arv_UT_300) �� 300 ��� ��� OZ OFER 2985523437721 �� UA 32427293 UA 16.04.1968 25.06.2025 M)

$(set pax_tid1 $(get_single_pax_tid  $(get point_dep_UT_298) ALIMOV TALGAT))
$(set pax_tid2 $(get_single_pax_tid  $(get point_dep_UT_300) OZ OFER))

??
$(db_dump_table PAX_TRANSLIT fields="point_id, translit_format" display="on")
>> lines=3:-3
[$(get point_dep_UT_298)] [1] $()
[$(get point_dep_UT_298)] [2] $()
[$(get point_dep_UT_298)] [3] $()
[$(get point_dep_UT_300)] [1] $()
[$(get point_dep_UT_300)] [2] $()
[$(get point_dep_UT_300)] [3] $()
;;

??
$(db_dump_table CRS_PAX_TRANSLIT fields="point_id, translit_format" display="on")
>> lines=3:-3
[$(get point_dep_UT_298_tlg)] [1] $()
[$(get point_dep_UT_298_tlg)] [2] $()
[$(get point_dep_UT_298_tlg)] [3] $()
[$(get point_dep_UT_300_tlg)] [1] $()
[$(get point_dep_UT_300_tlg)] [2] $()
[$(get point_dep_UT_300_tlg)] [3] $()
;;

$(run_arch_step $(ddmmyy +141))
??
$(check_dump ARX_MARK_TRIPS order="point_id")
>>
[��] [���] [298] [$(get point_dep_UT_298)] [$(date_format %d.%m.%Y)] [NULL] [$(date_format %d.%m.%Y)] $()
[��] [���] [300] [$(get point_dep_UT_300)] [$(date_format %d.%m.%Y)] [NULL] [$(date_format %d.%m.%Y)] $()
$()

??
$(check_dump ARX_PAX_GRP order="point_dep,grp_id")
>>
[���] [���] [0] [�] [12] [TERM] [������] [0] [0] [...] [1] [0] [$(get point_arv_UT_298)] [$(get point_dep_UT_298)] [$(get point_dep_UT_298)] [0] [K] [$(get pax_tid1)] [$(date_format %d.%m.%Y -3h)] [5] [NULL] [NULL] [$(date_format %d.%m.%Y)] $()
[���] [���] [0] [�] [12] [TERM] [������] [0] [0] [...] [1] [0] [$(get point_arv_UT_300)] [$(get point_dep_UT_300)] [$(get point_dep_UT_300)] [0] [K] [$(get pax_tid2)] [$(date_format %d.%m.%Y -3h)] [5] [NULL] [NULL] [$(date_format %d.%m.%Y)] $()
$()

??
$(db_dump_table PAX_TRANSLIT fields="point_id, translit_format" display="on")
>> lines=auto
------------------- END PAX_TRANSLIT DUMP COUNT=0 -------------------
;;

$(run_arch_step $(ddmmyy +141) 3)

??
$(db_dump_table CRS_PAX_TRANSLIT fields="point_id, translit_format" display="on")
>> lines=auto
------------------- END CRS_PAX_TRANSLIT DUMP COUNT=0 -------------------
;;

%%
#########################################################################################
###
#   ���� �12
#   �஢�ઠ 㤠����� ��ப � 6 蠣� TArxTlgsFilesEtc
#
###
#########################################################################################

$(init_jxt_pult ������)
$(set_desk_version 201707-0195750)
$(login)

################################################################################

$(set first_date "$(date_format %y%m%d +0)")

$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(1, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'), 1, 'A')")
$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(2, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'), 2, 'B')")
$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(3, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'), 3, 'C')")
$(db_sql TLGS "INSERT INTO TLGS(id, receiver, sender, time, tlg_num, type)  VALUES(4, 'ab', 'AC', TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'), 4, 'D')")

$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP$(get first_date).txt', 'RASTRV', -1, '��')")
$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP200618.txt', 'RASTRV', -1, '��')")
$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP$(get first_date).txt', 'SINSVO', 349, '��')")
$(db_sql aodb_spp_files "INSERT INTO aodb_spp_files(filename, point_addr, rec_no, airline)  VALUES('SPP210218.txt', 'SINSVO', 349, '��')")

$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(1, 'AB', 'BC', '$(date_format %Y-%m-%d)', '000')")
$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(2, 'AB', 'BC', '$(date_format %Y-%m-%d)', '000')")
$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(3, 'AB', 'BC', '$(date_format %Y-%m-%d -130)', '000')")
$(db_sql FILES "INSERT INTO FILES(id, receiver, sender, time, type)  VALUES(4, 'AB', 'BC', '$(date_format %Y-%m-%d -130)' , '000')")

$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(1, 1, TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'))")
$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(2, 5, TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'))")
$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(3, 8, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'))")
$(db_sql KIOSK_EVENTS "INSERT INTO KIOSK_EVENTS(id, ev_order, time)  VALUES(4, 9, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'))")

$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(1, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d)',      'BC', '000')")
$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(2, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d)',      'BG', '001')")
$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(3, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d -130)', 'BH', '002')")
$(db_sql ETICKS_DISPLAY "INSERT INTO ETICKS_DISPLAY(coupon_no, fare_basis, issue_date, last_display, surname, ticket_no)  VALUES(4, 'AB', TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),  '$(date_format %Y-%m-%d -130)' ,'BK', '003')")

$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(1, TO_DATE('$(date_format %Y-%m-%d)','yyyy-mm-dd'),      10, 'AB', 'DR', 0)")
$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(2, TO_DATE('$(date_format %Y-%m-%d)', 'yyyy-mm-dd'),     11, 'AC', 'DE', 2)")
$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(3, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'),12, 'AD', 'DH', 3)")
$(db_sql ETICKS_DISPLAY_TLGS "INSERT INTO ETICKS_DISPLAY_TLGS(coupon_no, last_display, page_no, ticket_no, tlg_text, tlg_type)  VALUES(4, TO_DATE('$(date_format %Y-%m-%d -130)', 'yyyy-mm-dd'),13, 'AE', 'DN', 9)")


#�� ���� ��娢�樨 �१ 133 ��� �� ⥪�饩 ���� ��࠭���� ३�� , ����� �뫨 �१ 12 ����, �� ⥪�饩 ����
$(run_arch_step $(ddmmyy +1) 6)

#� ⠡��� ������ ������� ⮫쪮 2 ��ப� ����� �� ������ ��� ��娢���, � ������� 䠩��� �� ⥪�饩 ����

??
$(db_dump_table TLGS fields="id, receiver, sender, time, tlg_num, type" display="on")
>> lines=auto
[1] [ab] [AC] [$(date_format %Y-%m-%d) 00:00:00] [1] [A] $()
[2] [ab] [AC] [$(date_format %Y-%m-%d) 00:00:00] [2] [B] $()

??
$(db_dump_table AODB_SPP_FILES display="on")
>> lines=auto
[��] [SPP$(get first_date).txt] [RASTRV] [-1] $()
[��] [SPP$(get first_date).txt] [SINSVO] [349] $()

??
$(db_dump_table FILES display="on")
>> lines=auto
[NULL] [NULL] [1] [AB] [BC] [$(date_format %Y-%m-%d) 00:00:00] [000] $()
[NULL] [NULL] [2] [AB] [BC] [$(date_format %Y-%m-%d) 00:00:00] [000] $()

??
$(db_dump_table KIOSK_EVENTS display="on")
>> lines=auto
[NULL] [1] [1] [NULL] [NULL] [NULL] [$(date_format %y%m%d)] [NULL] $()
[NULL] [5] [2] [NULL] [NULL] [NULL] [$(date_format %y%m%d)] [NULL] $()

??
$(db_dump_table ETICKS_DISPLAY fields="coupon_no, ticket_no, last_display" display="on")
>> lines=auto
[1] [000] [$(date_format %Y-%m-%d) 00:00:00] $()
[2] [001] [$(date_format %Y-%m-%d) 00:00:00] $()

??
$(db_dump_table ETICKS_DISPLAY_TLGS fields="coupon_no, ticket_no, last_display, tlg_type, tlg_text" display="on")
>> lines=auto
[1] [AB] [$(date_format %Y-%m-%d) 00:00:00] [0] [DR] $()
[2] [AC] [$(date_format %Y-%m-%d) 00:00:00] [2] [DE] $()





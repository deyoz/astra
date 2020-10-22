create or replace procedure SP_WB_SAVE_LOADSHEET
(cXML_in in clob,
   cXML_out out clob)
as
-- сохранение текущих данных в базе
P_ELEM_ID number := -1;
P_DATE varchar(20);
P_TIME varchar(20);
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'LoadSheet';
DT_LAST date;
cXML_LS clob;
cXML_Data clob;
cXML_in_2 clob;
-- Текущие данные
f1 char(3); f2 char(3); f3a char(8); f3b char(2); f4 char(6); f5 char(11); f6 char(7); f7 char(7); f8 char(4);
f9a number; f9b varchar(3); f10 number; f11 varchar(1000); f11a char(32); f11b char(32); f12 number; f13 varchar(50); f14 varchar(50); f15 varchar(50);
f16 varchar(50); f13_14_15_16 char(15); f17 number; f18 number; f19 char(3); f20 char(2); f21 char(11); f22 char(8); f23 char(8); f24 varchar(50);
f25 varchar(50); f26 varchar(50); f27 varchar(50); f28 varchar(50); f29 varchar(50); f30 varchar(50); f31 varchar(50); f32 varchar(50); f33 varchar(50); f34a char(1);
f34b char(1); f34c char(1); f35 number; f36a varchar(100); f36b varchar(100); f36c varchar(100); f36d varchar(100); f36e varchar(100); f36f varchar(100); f36g varchar(100);
f36h varchar(100); f42 varchar(50); f47 char(12); f48 varchar(50);

-- Последние сохраненные данные
f1_old char(3); f2_old char(3); f3a_old char(8); f3b_old char(2); f4_old char(6); f5_old char(11); f6_old char(7); f7_old char(7); f8_old char(4);
f9a_old number; f9b_old varchar(3); f10_old number; f11_old varchar(1000); f11a_old char(32); f11b_old char(32); f12_old number; f13_old varchar(50); f14_old varchar(50); f15_old varchar(50);
f16_old varchar(50); f13_14_15_16_old char(15); f17_old number; f18_old number; f19_old char(3); f20_old char(2); f21_old char(11); f22_old char(8); f23_old char(8); f24_old varchar(50);
f25_old varchar(50); f26_old varchar(50); f27_old varchar(50); f28_old varchar(50); f29_old varchar(50); f30_old varchar(50); f31_old varchar(50); f32_old varchar(50); f33_old varchar(50); f34a_old char(1);
f34b_old char(1); f34c_old char(1); f35_old number; f36a_old varchar(100); f36b_old varchar(100); f36c_old varchar(100); f36d_old varchar(100); f36e_old varchar(100); f36f_old varchar(100); f36g_old varchar(100);
f36h_old varchar(100); f42_old varchar(50); f47_old char(12); f48_old varchar(50);
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  -- Получение текущих данных
  cXML_in_2 := '<?xml version="1.0"?>'
              || '<root name="get_loadsheet"'
              || ' elem_id="' || to_char(P_ELEM_ID) || '"'
              || '>'
              || '</root>';

  SP_WB_GET_LOADSHEET(cXML_in_2, cXML_Data);

  -- Сравнить текущие данные с последними сохраненными
  if (cXML_Data is not null) then
  begin
    select to_char(EXTRACTVALUE(value(b), '/loadsheet/@cFrom_1')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTo_2')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cFlight_3a')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIdentifier_3b')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cA_C_Reg_4')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cVersion_5')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cCrew_6')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDate_7')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTime_8')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cEd_No_9a')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cEd_No_Status_9b')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cWeight_10')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cLoad_in_compartments_11')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPessenger_Cabin_bag_Weight_12')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDistribution_13')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cAdults_14')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cChd_15')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cInf_16')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPass_13_14_15_16')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_No_17')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cCabin_Bag_18')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPax_19')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cAct_class_serv_destinator_20')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_number_seats_21')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cSOC_22')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBLKD_23')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_Traffic_Load_24')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDry_Operationg_Weight_25')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cZero_Fuel_Weight_26')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cZero_Fuel_Weight_MAX_27')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTake_Off_Fuel_28')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTake_Off_Weight_29')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cMAX_Take_Off_Weight_30')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTrip_Fuel_31')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cLanding_Weight_32')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cMAX_Landing_Weight_33')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_1')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_2')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_3')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cUnderload_before_LMC_35')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_DOI')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_TOFI')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_ZFW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_TOW')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_LAW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_ZFW')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_TOW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_LAW'))
    into f1, f2, f3a, f3b, f4, f5, f6, f7, f8, f9a, f9b, f10, f11, f12, f13, f14, f15, f16, f13_14_15_16, f17, f18, f19, f20, f21, f22,
      f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34a, f34b, f34c, f35, f36a, f36b, f36c, f36d, f36e, f36f, f36g, f36h
    from table(XMLSequence(Extract(xmltype(cXML_Data), '/root/loadsheet'))) b;
  end;
  end if;

  -- Получаем данные по последнему сохраненному LoadSheet
  cXML_LS := '';

  select count(t1.ELEM_ID)
  into REC_COUNT
  from WB_CALCS_XML t1
  where (DATA_NAME = TABLEVAR) and (t1.ELEM_ID = P_ELEM_ID);

  -- Если есть данные
  if REC_COUNT > 0 then
  begin
    select max(t1.DT), to_char(max(t1.DT), 'DD.MM.YYYY'), to_char(max(t1.DT), 'HH24:MI:SS')
    into DT_LAST, P_DATE, P_TIME
    from WB_CALCS_XML t1
    where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID;

    select t1.XML_VALUE
    into cXML_LS
    from WB_CALCS_XML t1
    where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID and t1.DT = DT_LAST;
  end;
  end if;

  -- f1_old := nvl(f1_old, ' ');

  if cXML_LS is not null then
  begin
    select to_char(EXTRACTVALUE(value(b), '/loadsheet/@cFrom_1')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTo_2')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cFlight_3a')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIdentifier_3b')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cA_C_Reg_4')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cVersion_5')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cCrew_6')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDate_7')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTime_8')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cEd_No_9a')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cEd_No_Status_9b')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cWeight_10')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cLoad_in_compartments_11')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPessenger_Cabin_bag_Weight_12')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDistribution_13')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cAdults_14')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cChd_15')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cInf_16')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPass_13_14_15_16')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_No_17')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cCabin_Bag_18')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPax_19')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cAct_class_serv_destinator_20')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_number_seats_21')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cSOC_22')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBLKD_23')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_Traffic_Load_24')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDry_Operationg_Weight_25')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cZero_Fuel_Weight_26')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cZero_Fuel_Weight_MAX_27')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTake_Off_Fuel_28')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTake_Off_Weight_29')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cMAX_Take_Off_Weight_30')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTrip_Fuel_31')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cLanding_Weight_32')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cMAX_Landing_Weight_33')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_1')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_2')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_3')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cUnderload_before_LMC_35')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_DOI')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_TOFI')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_ZFW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_TOW')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_LAW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_ZFW')),
        to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_TOW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_LAW'))
    into f1_old, f2_old, f3a_old, f3b_old, f4_old, f5_old, f6_old, f7_old, f8_old, f9a_old, f9b_old, f10_old, f11_old, f12_old, f13_old, f14_old, f15_old, f16_old,
      f13_14_15_16_old, f17_old, f18_old, f19_old, f20_old, f21_old, f22_old, f23_old, f24_old, f25_old, f26_old, f27_old, f28_old, f29_old, f30_old, f31_old, f32_old,
      f33_old, f34a_old, f34b_old, f34c_old, f35_old, f36a_old, f36b_old, f36c_old, f36d_old, f36e_old, f36f_old, f36g_old, f36h_old
    from table(XMLSequence(Extract(xmltype(cXML_LS), '/root/loadsheet'))) b;
  end;
  end if;

  --cXML_out := cXML_out || ' f1 = ' || f1 || ' f1_old = ' || f1_old;

  -- Сравнить данные из cXML_LS и cXML_in
  if (nvl(f1_old, ' ') <> f1)
  or (nvl(f2_old, ' ') != f2)
  or (nvl(f3a_old, ' ') != f3a)
  or (nvl(f3b_old, ' ') != f3b)
  or (nvl(f4_old, ' ') != f4)
  or (nvl(f5_old, ' ') != f5)
  or (nvl(f6_old, ' ') != f6)
  or (nvl(f7_old, ' ') != f7)
  or (nvl(f8_old, ' ') != f8)
  or (nvl(f10_old, 0) != f10)
  or (nvl(f11_old, ' ') != f11)
  or (nvl(f12_old, 0) != f12)
  or (nvl(f13_old, ' ') != f13)
  or (nvl(f14_old, ' ') != f14)
  or (nvl(f15_old, ' ') != f15)
  or (nvl(f16_old, ' ') != f16)
  or (nvl(f13_14_15_16_old, ' ') != f13_14_15_16)
  or (nvl(f17_old, 0) != f17)
  or (nvl(f18_old, 0) != f18)
  or (nvl(f19_old, ' ') != f19)
  or (nvl(f20_old, ' ') != f20)
  or (nvl(f21_old, ' ') != f21)
  or (nvl(f22_old, ' ') != f22)
  or (nvl(f23_old, ' ') != f23)
  or (nvl(f24_old, ' ') != f24)
  or (nvl(f25_old, ' ') != f25)
  or (nvl(f26_old, ' ') != f26)
  or (nvl(f27_old, ' ') != f27)
  or (nvl(f28_old, ' ') != f28)
  or (nvl(f29_old, ' ') != f29)
  or (nvl(f30_old, ' ') != f30)
  or (nvl(f31_old, ' ') != f31)
  or (nvl(f32_old, ' ') != f32)
  or (nvl(f33_old, ' ') != f33)
  or (nvl(f34a_old, ' ') != f34a)
  or (nvl(f34b_old, ' ') != f34b)
  or (nvl(f34c_old, ' ') != f34c)
  or (nvl(f35_old, 0) != f35)
  or (nvl(f36a_old, ' ') != f36a)
  or (nvl(f36b_old, ' ') != f36b)
  or (nvl(f36c_old, ' ') != f36c)
  or (nvl(f36d_old, ' ') != f36d)
  or (nvl(f36e_old, ' ') != f36e)
  or (nvl(f36f_old, ' ') != f36f)
  or (nvl(f36g_old, ' ') != f36g)
  or (nvl(f36h_old, ' ') != f36h)
  then
  begin
    -- cXML_out := cXML_out || ' выполнить... ';

    -- Сохранить данные в базу
    insert into WB_CALCS_XML (ELEM_ID, DATA_NAME, XML_VALUE)
    values (P_ELEM_ID, TABLEVAR, cXML_data);

    select to_char(max(t1.DT), 'DD.MM.YYYY'), to_char(max(t1.DT), 'HH24:MI:SS')
    into P_DATE, P_TIME
    from WB_CALCS_XML t1
    where t1.DATA_NAME = 'LoadSheet' and t1.ELEM_ID = P_ELEM_ID;
  end;
  /*
  else
  begin
    cXML_out := cXML_out || ' nooooo test ';
  end;
  */
  end if;

  -- cXML_out := cXML_out || ' f1_old = ' || f1_old || ' f1 = ' || f1 || ' ';
  -- cXML_out := cXML_Data;

  cXML_out := '<?xml version="1.0"  encoding="utf-8"?>'
              || '<root name="save_loadsheet" result="ok"'
              || ' elem_id="' || to_char(P_ELEM_ID) || '"'
              || ' date="' || P_DATE || '"'
              || ' time="' || P_TIME || '"'
              || '>'
              || '</root>';

  commit;
end SP_WB_SAVE_LOADSHEET;
/

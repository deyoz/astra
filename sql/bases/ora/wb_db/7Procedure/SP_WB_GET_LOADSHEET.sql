create or replace PROCEDURE SP_WB_GET_LOADSHEET
(cXML_in in clob, cXML_out out clob)
AS
-- получение данных на основе вычислений
cXML_data XMLType; P_ID number := -1; vID_AC number; vID_WS number; vID_BORT number; vID_SL number;
vCount number; vCount2 number; vXMLParamIn clob; vXMLParamOut clob; sCrewCodeName varchar(100) := '';

-- Все поля
cFrom_1 varchar(40) := ' '; cTo_2 varchar(40) := ' '; cFlight_3a varchar(40) := ' '; cIdentifier_3b varchar(40) := ' '; cA_C_Reg_4 varchar(40) := ' ';
cVersion_5 varchar(40) := ' '; cCrew_6 varchar(40) := ' '; cDate_7 varchar(40) := ' '; cTime_8 varchar(40) := ' '; cEd_No_9a varchar(2) := 0;
cEd_No_Status_9b varchar(40) := ' '; cWeight_10 varchar(40) := ' '; cLoad_in_compartments_11 varchar(100) := ' '; cPessenger_Cabin_bag_Weight_12 varchar(40) := ' ';
cDistribution_13 varchar(40) := ' '; cAdults_14 varchar(40) := ' '; cFemale_14 varchar(10) := ' '; cChd_15 varchar(40) := ' '; cInf_16 varchar(40) := ' '; cTotal_No_17 varchar(40) := ' ';
cPass_13_14_15_16 varchar(50) := ' ';
cCabin_Bag_18 varchar(40) := ' '; cPax_19 varchar(40) := 'PAX'; cAct_class_serv_destinator_20 varchar(40) := ' '; cTotal_number_seats_21 varchar(40) := ' ';
cSOC_22 varchar(40) := ' '; cBLKD_23 varchar(40) := ' '; cTotal_Traffic_Load_24 varchar(40) := ' '; cDry_Operationg_Weight_25 varchar(40) := ' ';
cZero_Fuel_Weight_26 number := 0; cZero_Fuel_Weight_MAX_27 number := 0; cTake_Off_Fuel_28 number := 0; cTake_Off_Weight_29 number := 0;
cMAX_Take_Off_Weight_30 number := 0; cTrip_Fuel_31 number := 0; cLanding_Weight_32 number := 0; cMAX_Landing_Weight_33 number := 0;
cIndctr_for_max_weight_34_1 varchar(40) := ' '; cIndctr_for_max_weight_34_2 varchar(40) := ' '; cIndctr_for_max_weight_34_3 varchar(40) := ' '; cUnderload_before_LMC_35 varchar(40) := ' ';
cBlnce_and_Seat_Cnd_36_DOI varchar(40) := ' '; cBlnce_and_Seat_Cnd_36_TOFI varchar(40) := ' ';
cBlnce_and_Seat_Cnd_36_LI_ZFW varchar(40) := ' '; cBlnce_and_Seat_Cnd_36_LI_TOW varchar(40) := ' '; cBlnce_and_Seat_Cnd_36_LI_LAW varchar(40) := ' ';
cBlnce_and_Seat_Cnd_36_MAC_ZFW varchar(40) := ' '; cBlnce_and_Seat_Cnd_36_MAC_TOW varchar(40) := ' '; cBlnce_and_Seat_Cnd_36_MAC_LAW varchar(40) := ' ';
cDest_37 varchar(40) := ' '; cSpecification_38 varchar(40) := ' '; cCL_Cpt_39 varchar(40) := ' '; cPlus_Minus_40 varchar(40) := ' '; cWeight_of_LMC_41 varchar(40) := ' ';
cLMC_total_Plus_Minus_42 varchar(40) := '+ -'; cLMC_Total_Weight_43 varchar(40) := ' '; cAdj_44 varchar(40) := ' '; cCaptains_Inform_Notes_45 varchar(40) := ' ';
cLoadmessage_46 varchar(40) := ' '; cDeadload_breakdown_46a varchar(40) := ' '; cChecked_47 varchar(40) := ' '; cApproved_48 varchar(40) := ' ';

-- Веса
vPAX number; vCabinBaggageF number; vCabinBaggageC number; vCabinBaggageY number; vPAX_curr number; vTransitLoad number; vTransitLoad_curr number; vDryOperatingWeight number;
vCorrDryOperatingWeight number; vDOI number; vIdxTemp number; vDryOperatingIndex number; vWeightTemp number; vBlockFuel number; vTaxiFuel number;
vLI_ZFW_pass number; vLI_ZFW_bag number; vLI_ZFW_cb number;
-- vTOW number; vLAW number; vZFW number;
vLI_ZFW number; vLI_TOW number; vLI_LAW number;
vTakeOffFuelIdx number; vLandingFuelIdx number;
vC_CONST number; vK_CONST number; vREF_ARM number; vLEMAC_LERC number; vLEN_MAC_RC number;
vMAC_ZFW number; vMAC_TOW number; vMAC_LAW number;

 -- Пассажиры
vFcode number; vCcode number; vYcode number;

-- Багаж
vFBag number; vCBag number; vYBag number;

-- Груз
vFCargo number; vCCargo number; vYCargo number;

-- Почта
vFMail number; vCMail number; vYMail number;

vEqWeight number; vEqIdx number;
vULDWeight number; vULDIdx number;
vCC_ID number; vCC_Name varchar(100);
vPT_ID number; vPT_Name varchar(100);
vPW_ID number; vPW_Name varchar(100);
cAHMDOW clob;
vCabC_ID number; vCabC_Name varchar(100);

-- Курсор для PassengerDetails On
CURSOR cPassDetails (cP_ID NUMBER) IS
    select EXTRACTVALUE(value(b), '/class/@code') code, EXTRACTVALUE(value(b), '/class/@Adult') Adult, EXTRACTVALUE(value(b), '/class/@Male') Male, EXTRACTVALUE(value(b), '/class/@Female') Female,
      EXTRACTVALUE(value(b), '/class/@Child') Child, EXTRACTVALUE(value(b), '/class/@Infant') Infant, EXTRACTVALUE(value(b), '/class/@CabinBaggage') CabinBaggage
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/On/class'))) b
    where t1.ELEM_ID = cP_ID and t1.DATA_NAME = 'PassengersDetails';

-- Курсор для PassengerDetails Tr
CURSOR cPassDetailsTr (cP_ID NUMBER) IS
    select EXTRACTVALUE(value(b), '/class/@code') code, EXTRACTVALUE(value(b), '/class/@Adult') Adult, EXTRACTVALUE(value(b), '/class/@Male') Male, EXTRACTVALUE(value(b), '/class/@Female') Female,
      EXTRACTVALUE(value(b), '/class/@Child') Child, EXTRACTVALUE(value(b), '/class/@Infant') Infant, EXTRACTVALUE(value(b), '/class/@CabinBaggage') CabinBaggage
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/Tr/class'))) b
    where t1.ELEM_ID = cP_ID and t1.DATA_NAME = 'PassengersDetails';

-- Курсор для DeadloadDetails
CURSOR cDeadloadDetails (cP_ID NUMBER) IS
    select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
          EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b
    where t1.ELEM_ID = cP_ID and t1.DATA_NAME = 'DeadloadDetails';
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.ID_SL, t1.NR, to_char(t1.STD, 'DDMONYY', 'nls_date_language=English'), to_char(t1.STD, 'HH24MI'), to_char(t1.STD, 'DD')
  into vID_AC, vID_WS, vID_BORT, vID_SL, cFlight_3a, cDate_7, cTime_8, cIdentifier_3b
  from WB_SCHED t1 -- WB_REF_WS_SCHED_TEMP
  where t1.ID = P_ID;

  select nvl(t2.IATA, t2.AP),  nvl(t3.IATA, t3.AP)
  into cFrom_1, cTo_2
  from WB_SCHED_MRSHR t1 inner join WB_REF_AIRPORTS t2 on t1.ID_AP1 = t2.ID
                        inner join WB_REF_AIRPORTS t3 on t1.ID_AP2 = t3.ID
  where t1.ELEM_ID = P_ID and rownum <= 1
  order by t1.ID;

  select t3.TABLE_NAME
  into cVersion_5
  from WB_SCHED t1 inner join WB_REF_WS_AIR_S_L_C_ADV t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS and t1.ID_SL = t2.IDN -- and t1.ID_BORT = t2.ID_BORT
                inner join WB_REF_WS_AIR_S_L_C_IDN t3 on t2.IDN = t3.ID
  where t1.ID = P_ID and rownum <= 1
  order by t2.ID_WS, t2.ADV_ID;

  -- 9 EdNo
  select trim(to_char(count(t1.ELEM_ID) + 1, '09'))
  into cEd_No_9a
  from WB_CALCS_XML t1
  where t1.DATA_NAME = 'LoadSheet' and t1.ELEM_ID = P_ID;

  -- Вычисление значений
  -- distribution (начало)
  -- 10
  with tblDeadloadDecks as
  (
      select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@Position') Position, EXTRACTVALUE(value(b), '/row/@IsBulk') IsBulk,
            EXTRACTVALUE(value(b), '/row/@ULDIATAType') ULDIATAType, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
            EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked
      from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b
      where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
  )
  select sum(nvl(t11.NetWeight, 0))
  into cWeight_10
  from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                                inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                                inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                                inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                                inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                                left join WB_REF_WS_AIR_SEC_BAY_TT t8 on t4.ID = t8.T_ID
                                left join WB_REF_ULD_IATA t9 on t8.ULD_IATA_ID = t9.ID
                                left join WB_REF_ULD_TYPES t10 on t9.TYPE_ID = t10.ID
                                inner join tblDeadloaddecks t11 on t4.SEC_BAY_NAME = t11.Position and t9.NAME = t11.ULDIATAType and t11.PositionLocked = 'Y'
  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS; -- and t1.ID_BORT = :vID_BORT;

  if cWeight_10 is null then
    cWeight_10 := 0;
  end if;

  -- 11
  with tblDeadloadDecks as
  (
      select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@Position') Position, EXTRACTVALUE(value(b), '/row/@IsBulk') IsBulk,
            EXTRACTVALUE(value(b), '/row/@ULDIATAType') ULDIATAType, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
            EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked
      from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b
      where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
  )
  select LISTAGG(tt1.CMP_NAME || '/' || to_char(tt1.WEIGHT), ' ') within group (order by tt1.CMP_NAME)
  into cLoad_in_compartments_11
  from
  (
    select t3.CMP_NAME, sum(nvl(t11.NetWeight, 0)) WEIGHT -- LISTAGG(t3.CMP_NAME, '') within group (order by t3.CMP_NAME)
    from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                                  inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                                  inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                                  inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                                  inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                                  left join WB_REF_WS_AIR_SEC_BAY_TT t8 on t4.ID = t8.T_ID
                                  left join WB_REF_ULD_IATA t9 on t8.ULD_IATA_ID = t9.ID
                                  left join WB_REF_ULD_TYPES t10 on t9.TYPE_ID = t10.ID
                                  left join tblDeadloaddecks t11 on t4.SEC_BAY_NAME = t11.Position and t9.NAME = t11.ULDIATAType and t11.PositionLocked = 'Y'
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = :vID_BORT;
    group by t3.CMP_NAME
  ) tt1;
  -- distribution (начало)

  -- Информация по пассажирам (начало)
  vFcode :=  0; vCcode := 0; vYcode := 0; vPAX := 0; vPAX_curr := 0; vCabinBaggageF := 0; vCabinBaggageC := 0; vCabinBaggageY := 0;

  FOR PassDetails IN cPassDetails(P_ID)
  LOOP
     -- process data record
    -- DBMS_OUTPUT.PUT_LINE('Name = ' || PassDetails.code);
    if PassDetails.code = 'F' then
    begin
      vFcode := vFcode + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
                + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

      select to_number(nvl(PassDetails.Adult, 0)) * nvl(t1.ADULT, 0) + to_number(nvl(PassDetails.Male, 0)) * nvl(t1.ADULT, t1.MALE) + to_number(nvl(PassDetails.Female, 0)) * nvl(t1.ADULT, t1.FEMALE)
            + to_number(nvl(PassDetails.Child, 0)) * nvl(t1.CHILD, 0) + to_number(nvl(PassDetails.Infant, 0)) * nvl(t1.INFANT, 0)
      into vPAX_curr
      from WB_REF_AIRCO_PAX_WEIGHTS t1 inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_CLASS = t2.ID
                                      inner join (select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) date_from
                                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                  where t1.DATE_FROM <= sysdate
                                                  group by t1.ID_AC, t1.ID_CLASS) t3 on t1.ID_AC = t3.ID_AC and t1.ID_CLASS = t3.ID_CLASS and t1.date_from = t3.date_from
      where t1.ID_AC = vID_AC and t2.CLASS_CODE = 'F';

      vPAX := vPAX + nvl(vPAX_curr, 0);

      vCabinBaggageF := vCabinBaggageF + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;

    if PassDetails.code = 'C' then
    begin
      vCcode := vCcode + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
                + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

      select to_number(nvl(PassDetails.Adult, 0)) * nvl(t1.ADULT, 0) + to_number(nvl(PassDetails.Male, 0)) * nvl(t1.ADULT, t1.MALE) + to_number(nvl(PassDetails.Female, 0)) * nvl(t1.ADULT, t1.FEMALE)
            + to_number(nvl(PassDetails.Child, 0)) * nvl(t1.CHILD, 0) + to_number(nvl(PassDetails.Infant, 0)) * nvl(t1.INFANT, 0)
      into vPAX_curr
      from WB_REF_AIRCO_PAX_WEIGHTS t1 inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_CLASS = t2.ID
                                      inner join (select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) date_from
                                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                  where t1.DATE_FROM <= sysdate
                                                  group by t1.ID_AC, t1.ID_CLASS) t3 on t1.ID_AC = t3.ID_AC and t1.ID_CLASS = t3.ID_CLASS and t1.date_from = t3.date_from
      where t1.ID_AC = vID_AC and t2.CLASS_CODE = 'C';

      vPAX := vPAX + nvl(vPAX_curr, 0);

      vCabinBaggageC := vCabinBaggageC + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;

    if PassDetails.code = 'Y' then
    begin
      vYcode := vYcode + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
                + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

      select to_number(nvl(PassDetails.Adult, 0)) * nvl(t1.ADULT, 0) + to_number(nvl(PassDetails.Male, 0)) * nvl(t1.ADULT, t1.MALE) + to_number(nvl(PassDetails.Female, 0)) * nvl(t1.ADULT, t1.FEMALE)
            + to_number(nvl(PassDetails.Child, 0)) * nvl(t1.CHILD, 0) + to_number(nvl(PassDetails.Infant, 0)) * nvl(t1.INFANT, 0)
      into vPAX_curr
      from WB_REF_AIRCO_PAX_WEIGHTS t1 inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_CLASS = t2.ID
                                      inner join (select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) date_from
                                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                  where t1.DATE_FROM <= sysdate
                                                  group by t1.ID_AC, t1.ID_CLASS) t3 on t1.ID_AC = t3.ID_AC and t1.ID_CLASS = t3.ID_CLASS and t1.date_from = t3.date_from
      where t1.ID_AC = vID_AC and t2.CLASS_CODE = 'Y';

      vPAX := vPAX + nvl(vPAX_curr, 0);

      vCabinBaggageY := vCabinBaggageY + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;
  END LOOP;

  -- Подсчет TransitLoad (начало)
  vTransitLoad := 0; vTransitLoad_curr := 0; -- vCabinBaggageTrF := 0; vCabinBaggageTrC := 0; vCabinBaggageTrY := 0; vFcodeTr :=  0; vCcodeTr := 0; vYcodeTr := 0;

  FOR PassDetails IN cPassDetailsTr(P_ID)
  LOOP
     -- process data record
    -- DBMS_OUTPUT.PUT_LINE('Name = ' || PassDetails.code);
    if PassDetails.code = 'F' then
    begin
      -- vFcodeTr := vFcodeTr + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
      --          + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

      select to_number(nvl(PassDetails.Adult, 0)) * nvl(t1.ADULT, 0) + to_number(nvl(PassDetails.Male, 0)) * nvl(t1.ADULT, t1.MALE) + to_number(nvl(PassDetails.Female, 0)) * nvl(t1.ADULT, t1.FEMALE)
            + to_number(nvl(PassDetails.Child, 0)) * nvl(t1.CHILD, 0) + to_number(nvl(PassDetails.Infant, 0)) * nvl(t1.INFANT, 0)
      into vTransitLoad_curr
      from WB_REF_AIRCO_PAX_WEIGHTS t1 inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_CLASS = t2.ID
                                      inner join (select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) date_from
                                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                  where t1.DATE_FROM <= sysdate
                                                  group by t1.ID_AC, t1.ID_CLASS) t3 on t1.ID_AC = t3.ID_AC and t1.ID_CLASS = t3.ID_CLASS and t1.date_from = t3.date_from
      where t1.ID_AC = vID_AC and t2.CLASS_CODE = 'F';

      vTransitLoad := vTransitLoad + nvl(vTransitLoad_curr, 0);

      -- vCabinBaggageTrF := vCabinBaggageTrF + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;

    if PassDetails.code = 'C' then
    begin
      -- vCcodeTr := vCcodeTr + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
      --          + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

      select to_number(nvl(PassDetails.Adult, 0)) * nvl(t1.ADULT, 0) + to_number(nvl(PassDetails.Male, 0)) * nvl(t1.ADULT, t1.MALE) + to_number(nvl(PassDetails.Female, 0)) * nvl(t1.ADULT, t1.FEMALE)
            + to_number(nvl(PassDetails.Child, 0)) * nvl(t1.CHILD, 0) + to_number(nvl(PassDetails.Infant, 0)) * nvl(t1.INFANT, 0)
      into vTransitLoad_curr
      from WB_REF_AIRCO_PAX_WEIGHTS t1 inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_CLASS = t2.ID
                                      inner join (select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) date_from
                                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                  where t1.DATE_FROM <= sysdate
                                                  group by t1.ID_AC, t1.ID_CLASS) t3 on t1.ID_AC = t3.ID_AC and t1.ID_CLASS = t3.ID_CLASS and t1.date_from = t3.date_from
      where t1.ID_AC = vID_AC and t2.CLASS_CODE = 'C';

      vTransitLoad := vTransitLoad + nvl(vTransitLoad_curr, 0);

      -- vCabinBaggageTrC := vCabinBaggageTrC + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;

    if PassDetails.code = 'Y' then
    begin
      -- vYcodeTr := vYcodeTr + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
      --          + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

      select to_number(nvl(PassDetails.Adult, 0)) * nvl(t1.ADULT, 0) + to_number(nvl(PassDetails.Male, 0)) * nvl(t1.ADULT, t1.MALE) + to_number(nvl(PassDetails.Female, 0)) * nvl(t1.ADULT, t1.FEMALE)
            + to_number(nvl(PassDetails.Child, 0)) * nvl(t1.CHILD, 0) + to_number(nvl(PassDetails.Infant, 0)) * nvl(t1.INFANT, 0)
      into vTransitLoad_curr
      from WB_REF_AIRCO_PAX_WEIGHTS t1 inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_CLASS = t2.ID
                                      inner join (select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) date_from
                                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                  where t1.DATE_FROM <= sysdate
                                                  group by t1.ID_AC, t1.ID_CLASS) t3 on t1.ID_AC = t3.ID_AC and t1.ID_CLASS = t3.ID_CLASS and t1.date_from = t3.date_from
      where t1.ID_AC = vID_AC and t2.CLASS_CODE = 'Y';

      vTransitLoad := vTransitLoad + nvl(vTransitLoad_curr, 0);

      -- vCabinBaggageTrY := vCabinBaggageTrY + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;
  END LOOP;
  -- Подсчет TransitLoad (конец)

  -- 12 не окончательный подсчет, без vTransitLoad
  cPessenger_Cabin_bag_Weight_12 := vPAX + nvl(vCabinBaggageF, 0) + nvl(vCabinBaggageC, 0) + nvl(vCabinBaggageY, 0); -- + nvl(vTransitLoad, 0);

  select sum(nvl(t1.Male, 0)) male, sum(nvl(t1.Adult, 0)), sum(nvl(t1.Female, 0)), sum(nvl(t1.Child, 0)), sum(nvl(t1.Infant, 0))
  into cDistribution_13, cAdults_14, cFemale_14, cChd_15, cInf_16
  from
  (
    select EXTRACTVALUE(value(b), '/class/@code') code, EXTRACTVALUE(value(b), '/class/@Adult') Adult, EXTRACTVALUE(value(b), '/class/@Male') Male, EXTRACTVALUE(value(b), '/class/@Female') Female,
      EXTRACTVALUE(value(b), '/class/@Child') Child, EXTRACTVALUE(value(b), '/class/@Infant') Infant, EXTRACTVALUE(value(b), '/class/@CabinBaggage') CabinBaggage
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/On/class'))) b
    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'PassengersDetails'
  ) t1;

  cTotal_No_17 := cDistribution_13 + cAdults_14 + cFemale_14 + cChd_15 + cInf_16;

  cPass_13_14_15_16 := case when cAdults_14 = '0' then '' else cAdults_14 || '/' end
                  || case when cAdults_14 = '0' then cDistribution_13 || '/' else '' end
                  || case when cAdults_14 = '0' then cFemale_14 || '/' else '' end
                  || cChd_15 || '/'
                  || cInf_16;

  cCabin_Bag_18 := nvl(vCabinBaggageF, 0) + nvl(vCabinBaggageC, 0) + nvl(vCabinBaggageY, 0) + nvl(vTransitLoad, 0);

  cAct_class_serv_destinator_20 := ' ';

  -- 21
  with tblClassCodes as
  (
    select t1.ID ID, t1.CLASS_CODE, t1.DATE_FROM
    from WB_REF_AIRCO_CLASS_CODES t1 inner join (
                                                  select t1.CLASS_CODE, max(t1.DATE_FROM) DATE_FROM
                                                  from WB_REF_AIRCO_CLASS_CODES t1
                                                  where t1.ID_AC = vID_AC
                                                  group by t1.CLASS_CODE
                                                ) t2 on t1.CLASS_CODE = t2.CLASS_CODE and t1.DATE_FROM = t2.DATE_FROM
    where t1.ID_AC = vID_AC
  )
  select LISTAGG(to_char(tt1.pass), '/') within group (order by tt2.ID) pass
  into cTotal_number_seats_21
  from
  (
    select t1.code, sum(nvl(t1.Adult, 0) + nvl(t1.Male, 0) + nvl(t1.Female, 0) + nvl(t1.Child, 0)) pass -- + nvl(t1.Infant, 0)
    from
    (
      select EXTRACTVALUE(value(b), '/class/@code') code, EXTRACTVALUE(value(b), '/class/@Adult') Adult, EXTRACTVALUE(value(b), '/class/@Male') Male, EXTRACTVALUE(value(b), '/class/@Female') Female,
        EXTRACTVALUE(value(b), '/class/@Child') Child, EXTRACTVALUE(value(b), '/class/@Infant') Infant, EXTRACTVALUE(value(b), '/class/@CabinBaggage') CabinBaggage
      from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/On/class'))) b
      where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'PassengersDetails'
    ) t1
    group by t1.code
  ) tt1 inner join tblClassCodes tt2 on tt1.code = tt2.CLASS_CODE
  order by tt2.ID;

  -- Информация по багажу и грузу (начало)
  -- 24
  vFBag :=  0; vCBag := 0; vYBag := 0; vFCargo := 0; vCCargo := 0; vYCargo := 0; vFMail := 0; vCMail := 0; vYMail := 0; -- vTransitBag := 0;

  FOR DeadloadDetails IN cDeadloadDetails(P_ID)
  LOOP
    if (DeadloadDetails.TypeOfLoad = 'BF') and (DeadloadDetails.PositionLocked = 'Y') then
    begin
      vFBag := vFBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'BC') and (DeadloadDetails.PositionLocked = 'Y') then
    begin
      vCBag := vCBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'B') and (DeadloadDetails.PositionLocked = 'Y') then
    begin
      vYBag := vYBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'С') and (DeadloadDetails.PositionLocked = 'Y') then
    begin
      vYCargo := vYCargo + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'M') and (DeadloadDetails.PositionLocked = 'Y') then
    begin
      vYMail := vYMail + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'BT') and (DeadloadDetails.PositionLocked = 'Y') then
    begin
      vTransitLoad := vTransitLoad + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    /*
    if (DeadloadDetails.TypeOfLoad = 'BT') and (DeadloadDetails.PositionLocked = 'Y') then
    begin
      vTransitBag := vTransitBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;
    */
  END LOOP;

  cPessenger_Cabin_bag_Weight_12 := cPessenger_Cabin_bag_Weight_12 + vTransitLoad;

  -- Нужно ли nvl(vCabinBaggageF, 0) + nvl(vCabinBaggageC, 0) + nvl(vCabinBaggageY, 0) ???
  -- 24
  cTotal_Traffic_Load_24 := to_char(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad);

  -- Dry Operating Weight (начало)
  -- 25
  vDOI := 0; vCount := 0; vIdxTemp := 0; vCorrDryOperatingWeight := 0;

  /*
  select count(1)
  into vCount
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DOWData';

  if vCount > 0 then
  begin
    select EXTRACTVALUE(value(b), 'root/Start/@Weight'), to_number(EXTRACTVALUE(value(b), 'root/Start/@Idx'), '999999.9999') DryOperatingIndex,
      to_number(EXTRACTVALUE(value(b), '/root/OtherEq/@Idx'), '999999.9999') + to_number(EXTRACTVALUE(value(b), '/root/ULDs/@Idx'), '999999.9999') + to_number(EXTRACTVALUE(value(b), 'root/Start/@Idx'), '999999.9999'),
      to_number(EXTRACTVALUE(value(b), '/root/OtherEq/@Weight'), '999999.9999') + to_number(EXTRACTVALUE(value(b), '/root/ULDs/@Weight'), '999999.9999') + to_number(EXTRACTVALUE(value(b), 'root/Start/@Weight'), '999999.9999')
    into vDryOperatingWeight, vDryOperatingIndex, vIdxTemp, vCorrDryOperatingWeight
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root'))) b
    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DOWData' and rownum <= 1;

    vDOI := vDOI + nvl(vIdxTemp, 0); -- сложили StartIdx, ULDsIdx, OtherEqIdx
    vCorrDryOperatingWeight := nvl(vCorrDryOperatingWeight, 0);
  end;
  end if;
  */

  select count(1)
  into vCount
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DOWData';

  if vCount > 0 then
  begin
    -- DryOperating
    select t1.DOW, t1.DOI
    into vDryOperatingWeight, vDryOperatingIndex
    from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
          inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
    where t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT and t2.ID = vID_SL and rownum <= 1;

    vCorrDryOperatingWeight := vCorrDryOperatingWeight + vDryOperatingWeight;
    vDOI := vDOI + vDryOperatingIndex;

    -- Коды
    select to_number(EXTRACTVALUE(value(b), '/root/OtherEq/@Idx'), '999999.9999'), to_number(EXTRACTVALUE(value(b), '/root/OtherEq/@Weight'), '999999.9999'),
      to_number(EXTRACTVALUE(value(b), '/root/CrewCode/@id')), EXTRACTVALUE(value(b), '/root/CrewCode/@Name'),
      to_number(EXTRACTVALUE(value(b), '/root/PantryCode/@id')), EXTRACTVALUE(value(b), '/root/PantryCode/@Name'),
      to_number(EXTRACTVALUE(value(b), '/root/PotableWaterCode/@id')), EXTRACTVALUE(value(b), '/root/PotableWaterCode/@Name'),
      to_number(EXTRACTVALUE(value(b), '/root/CabinConfiguration/@id')), EXTRACTVALUE(value(b), '/root/CabinConfiguration/@Name')
    into vEqIdx, vEqWeight, vCC_ID, vCC_Name, vPT_ID, vPT_Name, vPW_ID, vPW_Name, vCabC_ID, vCabC_Name
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root'))) b
    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DOWData';

    vDOI := vDOI + nvl(vEqIdx, 0); -- добавили CrewCodes
    vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vEqWeight, 0);

    -- На основе этих кодов вытаскиваем веса и индексы из данных DOW
    SP_WB_REF_GET_DOW_REF(cXML_In, cAHMDOW);

    -- ULDCodes (начало)
    select count(1)
    into vCount2
    from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/CabinConfigurations/code'))) b
    where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vCabC_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vCabC_Name));

    if vCount2 > 0 then
    begin
      select to_number(EXTRACTVALUE(value(b), 'code/@ULD_Idx')), to_number(EXTRACTVALUE(value(b), 'code/@ULD_Weight'))
      into vIdxTemp, vWeightTemp
      from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/CabinConfigurations/code'))) b
      where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vCabC_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vCabC_Name)) and rownum <= 1;

      vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили ULDCodes
      vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);
    end;
    end if;
    -- ULDCodes (конец)

    -- CrewCodes (начало)
    select count(1)
    into vCount2
    from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/CrewCodes/code'))) b
    where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vCC_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vCC_Name))
      and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

    if vCount2 > 0 then
    begin
      select to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999'), to_number(EXTRACTVALUE(value(b), 'code/@Weight'))
      into vIdxTemp, vWeightTemp
      from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/CrewCodes/code'))) b
      where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vCC_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vCC_Name))
        and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

      vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили CrewCodes
      vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);

      if vCC_Name is not null then
      begin
        select to_char(nvl(t1.FC_NUMBER, 0)) || '/' || to_char(nvl(t1.CC_NUMBER, 0))
        into cCrew_6
        from WB_REF_WS_AIR_DOW_CR_CODES t1
        where t1.ID_AC = vID_AC  and t1.ID_WS = vID_WS and t1.CR_CODE_NAME = vCC_Name;
      end;
      else
      begin
        cCrew_6 := ' ';
      end;
      end if;
    end;
    end if;
    -- CrewCodes (конец)

    -- PantryCode (начало)
    select count(1)
    into vCount2
    from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/PantryCodes/code'))) b
    where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vPT_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vPT_Name))
      and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

    if vCount2 > 0 then
    begin
      select to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999'), to_number(EXTRACTVALUE(value(b), 'code/@Weight'))
      into vIdxTemp, vWeightTemp
      from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/PantryCodes/code'))) b
      where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vPT_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vPT_Name))
        and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

      vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили CrewCodes
      vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);
    end;
    end if;
    -- PantryCode (конец)

    -- PotableWaterCodes (начало)
    select count(1)
    into vCount2
    from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/PotableWaterCodes/code'))) b
    where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vPW_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vPW_Name))
      and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

    if vCount2 > 0 then
    begin
      select to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999'), to_number(EXTRACTVALUE(value(b), 'code/@Weight'))
      into vIdxTemp, vWeightTemp
      from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/PotableWaterCodes/code'))) b
      where ((to_number(EXTRACTVALUE(value(b), 'code/@id')) = vPW_ID) or (EXTRACTVALUE(value(b), '/code/@Name') = vPW_Name))
        and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

      vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили CrewCodes
      vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);
    end;
    end if;
    -- PotableWaterCodes (конец)

    -- SWA Codes (начало)
    select count(1)
    into vCount2
    from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/SWACodes/code'))) b
        inner join (
                    select distinct EXTRACTVALUE(value(b), '/code/@id') SWACode, EXTRACTVALUE(value(b), '/code/@Name') SWAName
                    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/SWACodes/code'))) b
                    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DOWData'
                    ) tt1 on (EXTRACTVALUE(value(b), '/code/@Name')) = tt1.SWAName;

    if vCount2 > 0 then
    begin
      select sum(to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999')), sum(to_number(EXTRACTVALUE(value(b), 'code/@Weight')))
      into vIdxTemp, vWeightTemp
      from table(XMLSequence(Extract(xmltype(cAHMDOW), '/root/SWACodes/code'))) b
          inner join (
                      select distinct EXTRACTVALUE(value(b), '/code/@id') SWACode, EXTRACTVALUE(value(b), '/code/@Name') SWAName
                      from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/SWACodes/code'))) b
                      where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DOWData'
                      ) tt1 on (EXTRACTVALUE(value(b), '/code/@Name')) = tt1.SWAName;

      vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили SWACodes
      vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);
    end;
    end if;
    -- SWA Codes (конец)
  end;
  end if;

  cDry_Operationg_Weight_25 := vCorrDryOperatingWeight;
  -- Dry Operating Weight (конец)

  -- 26
  cZero_Fuel_Weight_26 := nvl(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad + vDryOperatingWeight, 0);

  -- 27, 30, 33
  select t1.MAX_ZF_WEIGHT, t1.MAX_TO_WEIGHT, t1.MAX_LND_WEIGHT
  into cZero_Fuel_Weight_MAX_27, cMAX_Take_Off_Weight_30, cMAX_Landing_Weight_33
  from WB_REF_WS_AIR_MAX_WGHT_T t1
  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT;

  cZero_Fuel_Weight_MAX_27 := nvl(cZero_Fuel_Weight_MAX_27, 0);
  cMAX_Take_Off_Weight_30 := nvl(cMAX_Take_Off_Weight_30, 0);
  cMAX_Landing_Weight_33 := nvl(cMAX_Landing_Weight_33, 0);

  /*
  cZero_Fuel_Weight_MAX_27 := 117934; cMAX_Take_Off_Weight_30 := 179168; cMAX_Landing_Weight_33 := 136077;

  if vID_WS = 81 then -- 737-800
  begin
    cZero_Fuel_Weight_MAX_27 := 61688; cMAX_Take_Off_Weight_30 := 75000; cMAX_Landing_Weight_33 := 66360;
  end;
  end if;

  if vID_WS = 82 then
  begin
    cZero_Fuel_Weight_MAX_27 := 117934; cMAX_Take_Off_Weight_30 := 179168; cMAX_Landing_Weight_33 := 136077;
  end;
  end if;

  if vID_WS = 121 then -- 320-200
  begin
    cZero_Fuel_Weight_MAX_27 := 61000; cMAX_Take_Off_Weight_30 := 77000; cMAX_Landing_Weight_33 := 64500;
  end;
  end if;

  if vID_WS = 61 then -- 737-500
  begin
    cZero_Fuel_Weight_MAX_27 := 46720; cMAX_Take_Off_Weight_30 := 77000; cMAX_Landing_Weight_33 := 49895;
  end;
  end if;

  if vID_WS = 61 then -- 737-500
  begin
    cZero_Fuel_Weight_MAX_27 := 195044; cMAX_Take_Off_Weight_30 := 297556; cMAX_Landing_Weight_33 := 208652;
  end;
  end if;
  */

  -- 28, 31
  cTake_Off_Fuel_28 := 0; cTrip_Fuel_31 := 0;

  vCount := 0;
  select count(1)
  into vCount
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'FuelInfo';

  if vCount > 0 then
  begin
    select EXTRACTVALUE(value(b), '/fuel_info/@block_fuel'), EXTRACTVALUE(value(b), '/fuel_info/@taxi_fuel'), EXTRACTVALUE(value(b), '/fuel_info/@trip_fuel')
    into vBlockFuel, vTaxiFuel, cTrip_Fuel_31
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/fuel_info'))) b
    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'FuelInfo';

    cTake_Off_Fuel_28 := nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0);

    -- Получить Take-Off Fuel Idx
    vXMLParamIn := '<?xml version="1.0" ?>
<root><list>
                  <P_id_ac>' || to_char(vID_AC) || '</P_id_ac>
                  <P_id_ws>' || to_char(vID_WS) || '</P_id_ws>
                  <P_fuel>' || to_char(cTake_Off_Fuel_28) || '</P_fuel>
</list></root>';

    SP_WB__TAKE_OF_INDEX(vXMLParamIn, vXMLParamOut);

    select count(1)
    into vCount2
    from table(XMLSequence(Extract(xmltype(vXMLParamOut), '/root/list'))) b;

    if vCount2 > 0 then
    begin
      select EXTRACTVALUE(value(b), '/list/@y')
      into vTakeOffFuelIdx
      from table(XMLSequence(Extract(xmltype(vXMLParamOut), '/root/list'))) b;
    end;
    end if;
    -- <?xml version="1.0" ?><root><list y="-1,648"></list></root>

    -- Получить Landing Fuel Idx
    vXMLParamIn := '<?xml version="1.0" ?>
<root><list>
                  <P_id_ac>' || to_char(vID_AC) || '</P_id_ac>
                  <P_id_ws>' || to_char(vID_WS) || '</P_id_ws>
                  <P_fuel>' || to_char(cTake_Off_Fuel_28 - cTrip_Fuel_31) || '</P_fuel>
</list></root>';

    SP_WB__TAKE_OF_INDEX(vXMLParamIn, vXMLParamOut);

    select count(1)
    into vCount2
    from table(XMLSequence(Extract(xmltype(vXMLParamOut), '/root/list'))) b;

    if vCount2 > 0 then
    begin
      select EXTRACTVALUE(value(b), '/list/@y')
      into vLandingFuelIdx
      from table(XMLSequence(Extract(xmltype(vXMLParamOut), '/root/list'))) b;
    end;
    end if;
  end;
  end if;

  -- 29
  cTake_Off_Weight_29 := nvl(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad + vDryOperatingWeight + nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0), 0);

  -- 32
  cLanding_Weight_32 := nvl(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad + vDryOperatingWeight + nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0) - cTrip_Fuel_31, 0);

  -- 34 - пока непонятно
  cIndctr_for_max_weight_34_1 := 'L';
  cIndctr_for_max_weight_34_2 := ' ';
  cIndctr_for_max_weight_34_3 := ' ';

  -- 35 - позже
  cUnderload_before_LMC_35 := 0;

  if cIndctr_for_max_weight_34_1 = 'L' then
  begin
    cUnderload_before_LMC_35 := cZero_Fuel_Weight_MAX_27 - cZero_Fuel_Weight_26;
  end;
  end if;

  if cIndctr_for_max_weight_34_2 = 'L' then
  begin
    cUnderload_before_LMC_35 := cMAX_Take_Off_Weight_30 - cTake_Off_Weight_29;
  end;
  end if;

  if cIndctr_for_max_weight_34_3 = 'L' then
  begin
    cUnderload_before_LMC_35 := cMAX_Landing_Weight_33 - cLanding_Weight_32;
  end;
  end if;

  -- 36
  -- Balance Conditions (начало)
  vLI_ZFW_pass := 0; vLI_ZFW_bag := 0; vLI_ZFW_cb := 0; vLI_ZFW := 0;

  -- Из всех зон в SeatingDetails вытащить пассажиров, вычислить их массу, умножить на коэффициенты по зонам (начало)
  select sum((sd.adult * nvl(t1.ADULT, 0) + sd.male * nvl(t1.MALE, 0) + sd.female * nvl(t1.FEMALE, 0) + sd.child * nvl(t1.CHILD, 0) + sd.infant * nvl(t1.INFANT, 0)) * t4.INDEX_PER_WT_UNIT)
  into vLI_ZFW_pass
  from
  (
    select tt1.SEAT_ID, tt1.ClassCode, sum(case when tt1.OccupiedBy like '%ADULT%' then 1 else 0 end) adult, sum(case when tt1.OccupiedBy like '%MALE%' then 1 else 0 end) male,
      sum(case when tt1.OccupiedBy like '%FEMALE%' then 1 else 0 end) female, sum(case when tt1.OccupiedBy like '%CHILD%' then 1 else 0 end) child,
      sum(case when tt1.OccupiedBy like '%INFANT%' then 1 else 0 end) infant
    from
    (
      select EXTRACTVALUE(value(b), '/section/@Name') SEAT_ID, EXTRACTVALUE(value(c), '/seat/@Ident') Ident, EXTRACTVALUE(value(c), '/seat/@ClassCode') ClassCode, EXTRACTVALUE(value(c), '/seat/@OccupiedBy') OccupiedBy
      from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/section'))) b,
        table(XMLSequence( EXTRACT(value(b), '/section/row/seat'))) c
      where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'SeatingDetails'
    ) tt1
    group by tt1.SEAT_ID, tt1.ClassCode
  ) sd inner join WB_REF_AIRCO_CLASS_CODES t2 on sd.ClassCode = t2.CLASS_CODE
        inner join WB_REF_AIRCO_PAX_WEIGHTS t1 on t1.ID_CLASS = t2.ID
                                  inner join (select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) date_from
                                              from WB_REF_AIRCO_PAX_WEIGHTS t1
                                              where t1.DATE_FROM <= sysdate
                                              group by t1.ID_AC, t1.ID_CLASS) t3 on t1.ID_AC = t3.ID_AC and t1.ID_CLASS = t3.ID_CLASS and t1.date_from = t3.date_from
        inner join (
                      select t1.SECTION, t1.INDEX_PER_WT_UNIT
                      from WB_REF_WS_AIR_CABIN_CD t1
                      where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
                    ) t4 on sd.SEAT_ID = t4.SECTION
  where t1.ID_AC = vID_AC;
  -- Из всех зон в SeatingDetails вытащить пассажиров, вычислить их массу, умножить на коэффициенты по зонам (конец)

  -- Из всех зон в CabinBaggageByZone вытащить пассажиров, вычислить их массу, умножить на коэффициенты по зонам (начало)
  select sum(cb.CabinBaggage * t4.INDEX_PER_WT_UNIT)
  into vLI_ZFW_cb
  from
  (
    select EXTRACTVALUE(value(c), '/section/@Name') SEC_ID, EXTRACTVALUE(value(b), '/class/@code') ClassCode, EXTRACTVALUE(value(c), '/section/@CabinBaggage') CabinBaggage
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/class'))) b,
      table(XMLSequence( EXTRACT(value(b), '/class/section'))) c
    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'CabinBaggageByZone'
  ) cb
  inner join
  (
    select t1.SECTION, t1.INDEX_PER_WT_UNIT
    from WB_REF_WS_AIR_CABIN_CD t1
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT
  ) t4 on cb.SEC_ID = t4.SECTION;
  -- Из всех зон в CabinBaggageByZone вытащить пассажиров, вычислить их массу, умножить на коэффициенты по зонам (конец)

  -- Из DeadloadDetails вытаскиваем груз, умножаем на коэффициенты (начало)
  select sum(dd.NetWeight * t4.INDEX_PER_WT_UNIT)
  into vLI_ZFW_bag
  from WB_REF_WS_AIR_HLD_DECK t1
      inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
      inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
      inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
      inner join (
                  select dd.Position, sum(dd.NetWeight) NetWeight
                  from
                  (
                    select EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Position') Position
                    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b
                    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
                  ) dd
                  where dd.Position is not null
                  group by dd.Position
                ) dd on t4.SEC_BAY_NAME = dd.Position
  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS; -- and t1.ID_BORT = vID_BORT;
  -- Из DeadloadDetails вытаскиваем груз, умножаем на коэффициенты (конец)

  -- Получаем коэффициенты LI (начало)
  vLI_ZFW := vDOI + nvl(vLI_ZFW_pass, 0) + nvl(vLI_ZFW_bag, 0) + nvl(vLI_ZFW_cb, 0);
  vLI_TOW := vLI_ZFW + nvl(vTakeOffFuelIdx, 0);
  vLI_LAW := vLI_ZFW + nvl(vLandingFuelIdx, 0);
  -- Получаем коэффициенты LI (конец)

  -- Получаем коэффициенты MAC (начало)
  select t1.C_CONST, t1.K_CONST, t1.REF_ARM, t1.LEMAC_LERC, t1.LEN_MAC_RC
  into vC_CONST, vK_CONST, vREF_ARM, vLEMAC_LERC, vLEN_MAC_RC
  from WB_REF_WS_AIR_BAS_IND_FORM t1 inner join (
                                                  select ID_AC, ID_WS, ID_BORT, max(DATE_FROM) DATE_FROM
                                                  from WB_REF_WS_AIR_BAS_IND_FORM
                                                  where DATE_FROM <= sysdate
                                                  group by ID_AC, ID_WS, ID_BORT
                                                ) t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS and t1.DATE_FROM = t2.DATE_FROM -- and t1.ID_BORT = t2.ID_BORT
  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
  order by t1.ID_AC, t1.ID_WS, t1.ID_BORT;

  vC_CONST := nvl(vC_CONST, 0);
  vK_CONST := nvl(vK_CONST, 0);
  vREF_ARM := nvl(vREF_ARM, 0);
  vLEMAC_LERC := nvl(vLEMAC_LERC, 0);
  vLEN_MAC_RC := nvl(vLEN_MAC_RC, 0);

  select (vC_CONST * (vLI_ZFW - vK_CONST) / (case when cZero_Fuel_Weight_26 = 0 then 1 else cZero_Fuel_Weight_26 end) + vREF_ARM - vLEMAC_LERC) * 100 / vLEN_MAC_RC
  into vMAC_ZFW
  from dual;

  select (vC_CONST * (vLI_TOW - vK_CONST) / (case when cZero_Fuel_Weight_26 = 0 then 1 else cZero_Fuel_Weight_26 end) + vREF_ARM - vLEMAC_LERC) * 100 / vLEN_MAC_RC
  into vMAC_TOW
  from dual;

  select (vC_CONST * (vLI_LAW - vK_CONST) / (case when cZero_Fuel_Weight_26 = 0 then 1 else cZero_Fuel_Weight_26 end) + vREF_ARM - vLEMAC_LERC) * 100 / vLEN_MAC_RC
  into vMAC_LAW
  from dual;
  -- Получаем коэффициенты MAC (конец)

  cBlnce_and_Seat_Cnd_36_DOI := rtrim(to_char(nvl(vDOI, 0), 'FM99999990.99'), '.');
  cBlnce_and_Seat_Cnd_36_TOFI := rtrim(to_char(nvl(vTakeOffFuelIdx, 0), 'FM99999990.99'), '.');
  cBlnce_and_Seat_Cnd_36_LI_ZFW := rtrim(to_char(vLI_ZFW, 'FM99999990.99'), '.');
  cBlnce_and_Seat_Cnd_36_LI_TOW := rtrim(to_char(vLI_TOW, 'FM99999990.99'), '.');
  cBlnce_and_Seat_Cnd_36_LI_LAW := rtrim(to_char(vLI_LAW, 'FM99999990.99'), '.');
  cBlnce_and_Seat_Cnd_36_MAC_ZFW := rtrim(to_char(vMAC_ZFW, 'FM99999990.99'), '.'); -- 'FM999990.99'
  cBlnce_and_Seat_Cnd_36_MAC_TOW := rtrim(to_char(vMAC_TOW, 'FM99999990.99'), '.');
  cBlnce_and_Seat_Cnd_36_MAC_LAW := rtrim(to_char(vMAC_LAW, 'FM99999990.99'), '.'); -- vMAC_LAW

  /*
  cXML_out := cXML_out ||
  '<balance name="get_load_control_balance" result="ok">
    <Estimated DOI="0" TakeOfFuel="0" LI_ZFW="0" LI_TOW="0" LI_LAW="0" ZFW_MAC="0" TOW_MAC="0" LAW_MAC="0" />'
    || '<Actual DOI="' || rtrim(to_char(nvl(vDOI, 0), 'FM999990.99'), '.') || '"'
    || ' TakeOfFuel="' || rtrim(to_char(nvl(vTakeOffFuelIdx, 0), 'FM999990.99'), '.') || '"'
    || ' LI_ZFW="' || rtrim(to_char(vLI_ZFW, 'FM999990.99'), '.') || '"'
    || ' LI_TOW="' || rtrim(to_char(vLI_TOW, 'FM999990.99'), '.') || '"'
    || ' LI_LAW="' || rtrim(to_char(vLI_LAW, 'FM999990.99'), '.') || '"'
    || ' ZFW_MAC="' || rtrim(to_char(vMAC_ZFW, 'FM999990.99'), '.') || '"'
    || ' TOW_MAC="' || rtrim(to_char(vMAC_TOW, 'FM999990.99'), '.') || '"'
    || ' LAW_MAC="' || rtrim(to_char(vMAC_LAW, 'FM999990.99'), '.') || '" />'
    || '</balance>';
  */
  -- Balance Conditions (конец)

  -- Получаем XML
  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("loadsheet",
                              XMLATTRIBUTES(tt1.cFrom_1 "cFrom_1", tt1.cTo_2 "cTo_2", tt1.cFlight_3a "cFlight_3a", tt1.cIdentifier_3b "cIdentifier_3b", tt1.cA_C_Reg_4 "cA_C_Reg_4",
                                            tt1.cVersion_5 "cVersion_5", tt1.cCrew_6 "cCrew_6", tt1.cDate_7 "cDate_7", tt1.cTime_8 "cTime_8", tt1.cEd_No_9a "cEd_No_9a", tt1.cEd_No_Status_9b "cEd_No_Status_9b",
                                            tt1.cWeight_10 "cWeight_10", tt1.cLoad_in_compartments_11 "cLoad_in_compartments_11", tt1.cPessenger_Cabin_bag_Weight_12 "cPessenger_Cabin_bag_Weight_12",
                                            tt1.cDistribution_13 "cDistribution_13", tt1.cAdults_14 "cAdults_14", tt1.cFemale_14 "cFemale_14", tt1.cChd_15 "cChd_15", tt1.cInf_16 "cInf_16",
                                            tt1.cPass_13_14_15_16 "cPass_13_14_15_16", tt1.cTotal_No_17 "cTotal_No_17",
                                            tt1.cCabin_Bag_18 "cCabin_Bag_18", tt1.cPax_19 "cPax_19", tt1.cAct_class_serv_destinator_20 "cAct_class_serv_destinator_20", tt1.cTotal_number_seats_21 "cTotal_number_seats_21",
                                            tt1.cSOC_22 "cSOC_22", tt1.cBLKD_23 "cBLKD_23", tt1.cTotal_Traffic_Load_24 "cTotal_Traffic_Load_24", tt1.cDry_Operationg_Weight_25 "cDry_Operationg_Weight_25",
                                            tt1.cZero_Fuel_Weight_26 "cZero_Fuel_Weight_26", tt1.cZero_Fuel_Weight_MAX_27 "cZero_Fuel_Weight_MAX_27", tt1.cTake_Off_Fuel_28 "cTake_Off_Fuel_28",
                                            tt1.cTake_Off_Weight_29 "cTake_Off_Weight_29", tt1.cMAX_Take_Off_Weight_30 "cMAX_Take_Off_Weight_30", tt1.cTrip_Fuel_31 "cTrip_Fuel_31", tt1.cLanding_Weight_32 "cLanding_Weight_32",
                                            tt1.cMAX_Landing_Weight_33 "cMAX_Landing_Weight_33",
                                            tt1.cIndctr_for_max_weight_34_1 "cIndctr_for_max_weight_34_1", tt1.cIndctr_for_max_weight_34_2 "cIndctr_for_max_weight_34_2", tt1.cIndctr_for_max_weight_34_3 "cIndctr_for_max_weight_34_3",
                                            tt1.cUnderload_before_LMC_35 "cUnderload_before_LMC_35", tt1.cBlnce_and_Seat_Cnd_36_DOI "cBlnce_and_Seat_Cnd_36_DOI", tt1.cBlnce_and_Seat_Cnd_36_TOFI "cBlnce_and_Seat_Cnd_36_TOFI",
                                            tt1.cBlnce_and_Seat_Cnd_36_LI_ZFW "cBlnce_and_Seat_Cnd_36_LI_ZFW", tt1.cBlnce_and_Seat_Cnd_36_LI_TOW "cBlnce_and_Seat_Cnd_36_LI_TOW", tt1.cBlnce_and_Seat_Cnd_36_LI_LAW "cBlnce_and_Seat_Cnd_36_LI_LAW",
                                            tt1.cBlnce_and_Seat_Cnd_36_MAC_ZFW "cBlnce_and_Seat_Cnd_36_MAC_ZFW", tt1.cBlnce_and_Seat_Cnd_36_MAC_TOW "cBlnce_and_Seat_Cnd_36_MAC_TOW", tt1.cBlnce_and_Seat_Cnd_36_MAC_LAW "cBlnce_and_Seat_Cnd_36_MAC_LAW",
                                            tt1.cDest_37 "cDest_37", tt1.cSpecification_38 "cSpecification_38", tt1.cCL_Cpt_39 "cCL_Cpt_39", tt1.cPlus_Minus_40 "cPlus_Minus_40",
                                            tt1.cWeight_of_LMC_41 "cWeight_of_LMC_41", tt1.cLMC_total_Plus_Minus_42 "cLMC_total_Plus_Minus_42", tt1.cLMC_Total_Weight_43 "cLMC_Total_Weight_43",
                                            tt1.cAdj_44 "cAdj_44", tt1.cCaptains_Inform_Notes_45 "cCaptains_Inform_Notes_45", tt1.cLoadmessage_46 "cLoadmessage_46",
                                            tt1.cDeadload_breakdown_46a "cDeadload_breakdown_46a", tt1.cChecked_47 "cChecked_47", tt1.cApproved_48 "cApproved_48")
                            )
                )
  INTO cXML_data
  from
  (
    select cFrom_1, cTo_2, cFlight_3a, cIdentifier_3b, cA_C_Reg_4, cVersion_5, cCrew_6, cDate_7, cTime_8, cEd_No_9a, cEd_No_Status_9b, cWeight_10,
        cLoad_in_compartments_11, cPessenger_Cabin_bag_Weight_12, cDistribution_13, cAdults_14, cFemale_14, cChd_15, cInf_16,  cPass_13_14_15_16, cTotal_No_17,
        cCabin_Bag_18, cPax_19, cAct_class_serv_destinator_20, cTotal_number_seats_21, cSOC_22, cBLKD_23, cTotal_Traffic_Load_24, cDry_Operationg_Weight_25,
        cZero_Fuel_Weight_26, cZero_Fuel_Weight_MAX_27, cTake_Off_Fuel_28, cTake_Off_Weight_29, cMAX_Take_Off_Weight_30, cTrip_Fuel_31,
        cLanding_Weight_32, cMAX_Landing_Weight_33, cIndctr_for_max_weight_34_1, cIndctr_for_max_weight_34_2, cIndctr_for_max_weight_34_3, cUnderload_before_LMC_35,
        cBlnce_and_Seat_Cnd_36_DOI, cBlnce_and_Seat_Cnd_36_TOFI, cBlnce_and_Seat_Cnd_36_LI_ZFW, cBlnce_and_Seat_Cnd_36_LI_TOW, cBlnce_and_Seat_Cnd_36_LI_LAW,
        cBlnce_and_Seat_Cnd_36_MAC_ZFW, cBlnce_and_Seat_Cnd_36_MAC_TOW, cBlnce_and_Seat_Cnd_36_MAC_LAW,
        cDest_37, cSpecification_38, cCL_Cpt_39, cPlus_Minus_40, cWeight_of_LMC_41, cLMC_total_Plus_Minus_42, cLMC_Total_Weight_43, cAdj_44,
        cCaptains_Inform_Notes_45, cLoadmessage_46, cDeadload_breakdown_46a, cChecked_47, cApproved_48
    from dual
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_loadsheet" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_GET_LOADSHEET;
/

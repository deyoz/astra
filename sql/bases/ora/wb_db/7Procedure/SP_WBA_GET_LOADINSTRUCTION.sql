create or replace PROCEDURE SP_WBA_GET_LOADINSTRUCTION
(cXML_in in clob, cXML_out out clob)
AS
-- получение данных на основе вычислений
cXML_data XMLType; P_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number; vID_SL number;
vCount number; vCount2 number; vXMLParamIn clob; vXMLParamOut clob; vBulk number; vREC_COUNT number;

cAirCoCode varchar(10) := ' ';

-- Все поля
cFrom_1 varchar(40) := ' '; cTo_2 varchar(40) := ' '; cFlight_3 varchar(40) := ' '; cA_C_Reg_4 varchar(40) := ' ';
cVersion_5 varchar(40) := ' '; cGate_6 varchar(40) := ' '; cStand_7 varchar(40);  cDate_8 varchar(40) := ' '; cTime_9 varchar(40) := ' '; cEd_No_10 varchar(2) := '0';
cPlannedJoiningLoad_11 varchar(1000) := ' '; cJoiningSpecs_12 varchar(100) := ' '; cTransitSpecs_13 varchar(100) := ' '; cReloads_14 varchar(100) := ' '; cCPT_15 varchar(1000);

cInf_16 varchar(40) := ' '; cTotal_No_17 varchar(40) := ' ';
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

-- Курсор для секций
CURSOR cCMPs (cP_ID NUMBER) IS
  select case
            when tt1.DECK_NAME = 'UPPER' then 'UD'
            when tt1.DECK_NAME = 'MAIN' then 'MD'
            when tt1.DECK_NAME = 'LOWER' then 'LD'
            else ' '
          end DECK_NAME,
          tt1.CMP_NAME, tt1.BA_FWD, tt1.SEC_BAY_NAME, tt1.LR, tt1.IATA_ID, tt1.IATA_TYPE, tt1.BEG_SER_NUM, tt1.Tare_Weight, tt1.MAX_WEIGHT, tt1.AP, tt1.NetWeight
  from
  (
    select t5.ID DECK_ID, t5.NAME DECK_NAME, t3.CMP_NAME, t4.BA_FWD, t4.SEC_BAY_NAME,
      case
        when nvl(t4.LA_FROM, 0) > 0 and nvl(t4.LA_TO, 0) > 0 then 1
        when nvl(t4.LA_FROM, 0) < 0 and nvl(t4.LA_TO, 0) < 0 then -1
        else 2
      end LR,
      t9.ID IATA_ID, t9.NAME IATA_TYPE,
      t3.MAX_WEIGHT,
      t11.IATA AP,
      t12.BEG_SER_NUM,
      nvl(t12.Tare_Weight, 0) Tare_Weight,
      sum(nvl(t11.NetWeight, 0)) NetWeight
    from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                                  inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                                  inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                                  inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                                  inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                                  inner join WB_REF_WS_AIR_SEC_BAY_TT t8 on t4.ID = t8.T_ID
                                  left join WB_REF_ULD_IATA t9 on t8.ULD_IATA_ID = t9.ID
                                  left join WB_REF_ULD_TYPES t10 on t9.TYPE_ID = t10.ID
                                  inner join WB_SHED t13 on t13.ID = cP_ID and t1.ID_AC = t13.ID_AC and t1.ID_WS = t13.ID_WS and t1.ID_BORT = t13.ID_BORT
                                  join (
                                          select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@Position') Position, EXTRACTVALUE(value(b), '/row/@IsBulk') IsBulk,
                                                EXTRACTVALUE(value(b), '/row/@ULDIATAType') ULDIATAType, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
                                                EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked, t2.IATA
                                          from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b inner join WB_REF_AIRPORTS t2 on EXTRACTVALUE(value(b), '/row/@DestId') = t2.ID
                                          where t1.ELEM_ID = cP_ID and t1.DATA_NAME = 'DeadloadDetails'
                                        ) t11 on t4.SEC_BAY_NAME = t11.Position and t9.NAME = t11.ULDIATAType and t11.PositionLocked = 'Y' and t11.Finalized = 'Y'
                                  left join (
                                              select t1.*
                                              from WB_REF_AIRCO_ULD t1 inner join (select t.ULD_IATA_ID, max(t.BY_DEFAULT) BY_DEFAULT from WB_REF_AIRCO_ULD t group by t.ULD_IATA_ID) t2 on t1.ULD_IATA_ID = t2.ULD_IATA_ID and t1.BY_DEFAULT = t2.BY_DEFAULT
                                            ) t12 on t9.ID = t12.ULD_IATA_ID
    group by t5.ID, t5.NAME, t3.CMP_NAME, t11.IATA, t4.BA_FWD,
      case
        when nvl(t4.LA_FROM, 0) > 0 and nvl(t4.LA_TO, 0) > 0 then 1
        when nvl(t4.LA_FROM, 0) < 0 and nvl(t4.LA_TO, 0) < 0 then -1
        else 2
      end,
      t9.ID, t9.NAME, t12.BEG_SER_NUM, nvl(t12.Tare_Weight, 0),
      t4.SEC_BAY_NAME,
      t3.MAX_WEIGHT
  ) tt1
  where tt1.NetWeight > 0;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  -- 3, 4*, 6, 8, 9
  -- cA_C_Reg_4 - пока еще не определить!!!
  select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.NR, to_char(t1.S_DTL_1, 'DDMONYY', 'nls_date_language=English'), to_char(t1.S_DTL_1, 'HH24MI'), 'Stand', 'NIL', substr(t1.NR, 1, 2) -- , t1.GATE, t1.ID_SL -- будущее конфигурация
  into vID_AC, vID_WS, vID_BORT, cFlight_3, cDate_8, cTime_9, cStand_7, cJoiningSpecs_12, cAirCoCode -- , cGate_6, vID_SL
  from WB_SHED t1
  where t1.ID = P_ID and rownum <= 1;

  vBulk := 0;

  select count(1)
  into vREC_COUNT
  from WB_REF_WS_AIR_TYPE t1 inner join WB_REF_WS_TYPE_OF_LOADING t2 on t1.ID_WS_TYPE_OF_LOADING = t2.ID
  where (t1.ID_AC = vID_AC) and (t1.ID_WS = vID_WS) and (t2.NAME = 'Bulk');

  if vREC_COUNT > 0 then
  begin
    vBulk := 1;
  end;
  end if;

  -- 1, 2
  select nvl(t2.IATA, t2.AP),  nvl(t3.IATA, t3.AP)
  into cFrom_1, cTo_2
  from WB_SHED t1 inner join WB_REF_AIRPORTS t2 on t1.ID_AP_1 = t2.ID
                        inner join WB_REF_AIRPORTS t3 on t1.ID_AP_2 = t3.ID
  where t1.ID = P_ID and rownum <= 1
  order by t1.ID;

  -- 5
  select t3.TABLE_NAME
  into cVersion_5
  from WB_SHED t1 inner join WB_REF_WS_AIR_S_L_C_ADV t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS -- and t1.ID_SL = t2.IDN -- на будущее конфигурация -- and t1.ID_BORT = t2.ID_BORT
                inner join WB_REF_WS_AIR_S_L_C_IDN t3 on t2.IDN = t3.ID
  where t1.ID = P_ID and rownum <= 1
  order by t2.ID_WS, t2.ADV_ID;

  -- 7
  select 'Stand'
  into cStand_7
  from dual;

  -- 10 EdNo
  select trim(to_char(count(t1.ELEM_ID) + 1, '09'))
  into cEd_No_10
  from WB_CALCS_XML t1
  where t1.DATA_NAME = 'LoadInsruction' and t1.ELEM_ID = P_ID;

  -- 11 пассажиры и багаж. КАк учитывать признаки Finalized и PositionLocked
  with t_Load as
  (
    select tt1.row_num, tt1.IATA, tt1."F" P_F, tt1."C" P_C, tt1."Y" P_Y, tt2."C" B_C, tt2."M" B_M, tt2."B" B_B
    from
    (
      select t1.DestId, t1.IATA, sum(case when t1.CODE = 'F' then nvl(t1.adult, 0) + nvl(t1.male, 0) + nvl(t1.female, 0) + nvl(t1.child, 0) + nvl(t1.infant, 0) else 0 end) "F",
                  sum(case when t1.CODE = 'C' then nvl(t1.adult, 0) + nvl(t1.male, 0) + nvl(t1.female, 0) + nvl(t1.child, 0) + nvl(t1.infant, 0) else 0 end) "C",
                  sum(case when t1.CODE = 'Y' then nvl(t1.adult, 0) + nvl(t1.male, 0) + nvl(t1.female, 0) + nvl(t1.child, 0) + nvl(t1.infant, 0) else 0 end) "Y",
                  min(t1.row_num) row_num
      from
      (
        select rownum row_num, EXTRACTVALUE(value(c), '/class/@code') code, EXTRACTVALUE(value(c), '/class/@Adult') Adult, EXTRACTVALUE(value(c), '/class/@Male') Male, EXTRACTVALUE(value(c), '/class/@Female') Female,
          EXTRACTVALUE(value(c), '/class/@Child') Child, EXTRACTVALUE(value(c), '/class/@Infant') Infant, EXTRACTVALUE(value(c), '/class/@CabinBaggage') CabinBaggage,
          EXTRACTVALUE(value(b), '/ARR/@id') Destid, t2.IATA
        from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR'))) b inner join WB_REF_AIRPORTS t2 on EXTRACTVALUE(value(b), '/ARR/@id') = t2.ID,
          table(XMLSequence( EXTRACT(value(b), '/ARR/On/class'))) c
        where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'PassengersDetails'
      ) t1
      group by t1.DestId, t1.IATA
    ) tt1
    inner join
    (
      select t1.DestId, t1.IATA, sum(case when t1.TYPEOFLOAD = 'C' then to_number(t1.NETWEIGHT) else 0 end) "C",
                  sum(case when t1.TYPEOFLOAD = 'M' then to_number(t1.NETWEIGHT) else 0 end) "M",
                  sum(case when t1.TYPEOFLOAD = 'B' then to_number(t1.NETWEIGHT) else 0 end) "B"
      from
      (
          select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
                EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked, EXTRACTVALUE(value(b), '/row/@DestId') DestId, t2.IATA
          from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b inner join WB_REF_AIRPORTS t2 on EXTRACTVALUE(value(b), '/row/@DestId') = t2.ID
          where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
      ) t1
      group by t1.DestId, t1.IATA
    ) tt2 on tt1.DestId = tt2.DestId
  )
  select LISTAGG(t1.IATA || '     F    ' || to_char(t1.P_F) || '     C    ' || to_char(t1.P_C) || '    Y     ' || to_char(t1.P_Y)
                  || '    C     ' || to_char(t1.B_C) || '    M    ' || to_char(t1.B_M) || '    B    ' || to_char(t1.B_B), '
') within group (order by t1.row_num) TT
  into cPlannedJoiningLoad_11
  from t_load t1;

  -- 12, 13, 14
  select 'NIL', 'NIL', ' '
  into cJoiningSpecs_12, cTransitSpecs_13, cReloads_14
  from dual;

  -- Получаем XML
  cXML_out := '<?xml version="1.0" ?><root>';

  with t_CPTs as
  (
    select case
              when tt1.DECK_NAME = 'UPPER' then 'UD'
              when tt1.DECK_NAME = 'MAIN' then 'MD'
              when tt1.DECK_NAME = 'LOWER' then 'LD'
              else ' '
            end DECK_NAME,
            tt1.CMP_NAME, tt1.BA_FWD, tt1.SEC_BAY_NAME, tt1.LR, tt1.IATA_ID, tt1.IATA_TYPE, tt1.BEG_SER_NUM, tt1.Tare_Weight, tt1.MAX_WEIGHT, tt1.AP, tt1.NetWeight,
            tt1.NetWeight_B, tt1.NetWeight_C, tt1.NetWeight_M
    from
    (
      -- Compartments
      select t5.ID DECK_ID, t5.NAME DECK_NAME, t3.CMP_NAME, t4.BA_FWD, t4.SEC_BAY_NAME,
        case
          when nvl(t4.LA_FROM, 0) > 0 and nvl(t4.LA_TO, 0) > 0 then 1
          when nvl(t4.LA_FROM, 0) < 0 and nvl(t4.LA_TO, 0) < 0 then -1
          else 2
        end LR,
        t9.ID IATA_ID, t9.NAME IATA_TYPE,
        t3.MAX_WEIGHT,
        t11.IATA AP,
        t12.BEG_SER_NUM,
        nvl(t12.Tare_Weight, 0) Tare_Weight,
        sum(nvl(t11.NetWeight, 0)) NetWeight,
        sum(case when t11.TypeOfLoad = 'B' then to_number(t11.NetWeight) else 0 end) NetWeight_B,
        sum(case when t11.TypeOfLoad = 'С' then to_number(t11.NetWeight) else 0 end) NetWeight_C,
        sum(case when t11.TypeOfLoad = 'M' then to_number(t11.NetWeight) else 0 end) NetWeight_M
      from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                                    inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                                    inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                                    inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                                    inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                                    inner join WB_REF_WS_AIR_SEC_BAY_TT t8 on t4.ID = t8.T_ID
                                    left join WB_REF_ULD_IATA t9 on t8.ULD_IATA_ID = t9.ID
                                    left join WB_REF_ULD_TYPES t10 on t9.TYPE_ID = t10.ID
                                    inner join WB_SHED t13 on t13.ID = P_ID and t1.ID_AC = t13.ID_AC and t1.ID_WS = t13.ID_WS -- and t1.ID_BORT = t13.ID_BORT
                                    join (
                                            select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@Position') Position, EXTRACTVALUE(value(b), '/row/@IsBulk') IsBulk,
                                                  EXTRACTVALUE(value(b), '/row/@ULDIATAType') ULDIATAType, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
                                                  EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked, t2.IATA, t2.ID ID_AP
                                            from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b inner join WB_REF_AIRPORTS t2 on EXTRACTVALUE(value(b), '/row/@DestId') = t2.ID
                                            where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
                                          ) t11 on t13.ID_AP_2 = t11.ID_AP
                                                    and t4.SEC_BAY_NAME = t11.Position and t9.NAME = t11.ULDIATAType and t11.PositionLocked = 'Y' and t11.Finalized = 'Y'
                                    left join (
                                                select t1.*
                                                from WB_REF_AIRCO_ULD t1 inner join (select t.ULD_IATA_ID, max(t.BY_DEFAULT) BY_DEFAULT from WB_REF_AIRCO_ULD t group by t.ULD_IATA_ID) t2 on t1.ULD_IATA_ID = t2.ULD_IATA_ID and t1.BY_DEFAULT = t2.BY_DEFAULT
                                              ) t12 on t9.ID = t12.ULD_IATA_ID
      group by t5.ID, t5.NAME, t3.CMP_NAME, t11.IATA, t4.BA_FWD,
        case
          when nvl(t4.LA_FROM, 0) > 0 and nvl(t4.LA_TO, 0) > 0 then 1
          when nvl(t4.LA_FROM, 0) < 0 and nvl(t4.LA_TO, 0) < 0 then -1
          else 2
        end,
        t9.ID, t9.NAME, t12.BEG_SER_NUM, nvl(t12.Tare_Weight, 0),
        t4.SEC_BAY_NAME,
        t3.MAX_WEIGHT

      -- Bulk
      union all
      select t5.ID DECK_ID, t5.NAME DECK_NAME, t3.CMP_NAME, t4.BA_FWD, t4.SEC_BAY_NAME,
        case
          when nvl(t4.LA_FROM, 0) > 0 and nvl(t4.LA_TO, 0) > 0 then 1
          when nvl(t4.LA_FROM, 0) < 0 and nvl(t4.LA_TO, 0) < 0 then -1
          else 2
        end LR,
        0 IATA_ID, ' ' IATA_TYPE,
        t3.MAX_WEIGHT,
        t11.IATA AP,
        ' ' BEG_SER_NUM,
        0 Tare_Weight,
        sum(nvl(t11.NetWeight, 0)) NetWeight,
        sum(case when t11.TypeOfLoad = 'B' then to_number(t11.NetWeight) else 0 end) NetWeight_B,
        sum(case when t11.TypeOfLoad = 'С' then to_number(t11.NetWeight) else 0 end) NetWeight_C,
        sum(case when t11.TypeOfLoad = 'M' then to_number(t11.NetWeight) else 0 end) NetWeight_M
      from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                                    inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                                    inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                                    inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                                    inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID and t7.NAME = 'Bulk'
                                    inner join WB_SHED t13 on t13.ID = P_ID and t1.ID_AC = t13.ID_AC and t1.ID_WS = t13.ID_WS -- and t1.ID_BORT = t13.ID_BORT
                                    left join (
                                            select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@Position') Position, EXTRACTVALUE(value(b), '/row/@IsBulk') IsBulk,
                                                  EXTRACTVALUE(value(b), '/row/@ULDIATAType') ULDIATAType, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
                                                  EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked, t2.IATA, t2.ID ID_AP
                                            from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b inner join WB_REF_AIRPORTS t2 on EXTRACTVALUE(value(b), '/row/@DestId') = t2.ID
                                            where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
                                          ) t11 on t13.ID_AP_2 = t11.ID_AP
                                                    and t4.SEC_BAY_NAME = t11.Position and t11.Position is not null -- для Bulk берем, если есть позиция
                                                    -- and t11.PositionLocked = 'Y' and t11.Finalized = 'Y' -- and t9.NAME = t11.ULDIATAType
      group by t5.ID, t5.NAME, t3.CMP_NAME, t11.IATA, t4.BA_FWD,
        case
          when nvl(t4.LA_FROM, 0) > 0 and nvl(t4.LA_TO, 0) > 0 then 1
          when nvl(t4.LA_FROM, 0) < 0 and nvl(t4.LA_TO, 0) < 0 then -1
          else 2
        end,
        t4.SEC_BAY_NAME,
        t3.MAX_WEIGHT
    ) tt1
    where (vBulk = 1) -- если Bulk, то все позиции
      or (tt1.NetWeight > 0) -- иначе только с весами
  )
  select XMLAGG(
                  XMLELEMENT("loadsheet",
                              XMLATTRIBUTES(
                                              tt1.cFrom_1 "cFrom_1", tt1.cTo_2 "cTo_2", tt1.cFlight_3 "cFlight_3", tt1.cA_C_Reg_4 "cA_C_Reg_4",
                                              tt1.cVersion_5 "cVersion_5", tt1.cGate_6 "cGate_6", tt1.cStand_7 "cStand_7", tt1.cDate_8 "cDate_8", tt1.cTime_9 "cTime_9", tt1.cEd_No_10 "cEd_No_10",
                                              tt1.cPlannedJoiningLoad_11 "cPlannedJoiningLoad_11",
                                              tt1.cJoiningSpecs_12 "cJoiningSpecs_12", tt1.cTransitSpecs_13 "cTransitSpecs_13", tt1.cReloads_14 "cReloads_14"),

                                              ( -- Следующий вложенный элемент - секции
                                                SELECT XMLAGG(
                                                              XMLELEMENT("cmp",
                                                                          XMLATTRIBUTES(
                                                                                        t1.DECK_NAME as "deck_name",
                                                                                        t1.CMP_NAME as "cmp_name",
                                                                                        t1.BA_FWD as "ba_fwd",
                                                                                        t1.SEC_BAY_NAME as "sec_bay_name",
                                                                                        t1.LR as "lr",
                                                                                        t1.IATA_ID as "iata_id",
                                                                                        t1.IATA_TYPE as "iata_type",
                                                                                        t1.BEG_SER_NUM as "beg_ser_num",
                                                                                        t1.Tare_Weight as "tare_weight",
                                                                                        t1.MAX_WEIGHT as "max_weight",
                                                                                        t1.AP as "ap",
                                                                                        t1.NetWeight as "netweight",
                                                                                        t1.SEC_BAY_NAME_R as "sec_bay_name_r",
                                                                                        t1.IATA_TYPE_R as "iata_type_r",
                                                                                        t1.BEG_SER_NUM_R as "beg_ser_num_r",
                                                                                        t1.Tare_Weight_R as "tare_weight_r",
                                                                                        t1.NetWeight_R as "netweight_r",
                                                                                        t1.AP_R as "ap_r",
                                                                                        t1.onload as "onload",
                                                                                        t1.specs as "specs",
                                                                                        t1.report as "report",
                                                                                        t1.onload_r as "onload_r",
                                                                                        t1.specs_r as "specs_r",
                                                                                        t1.report_r as "report_r",
                                                                                        t1.bLR as "blr",
                                                                                        case when t1.NetWeight_B = 0 then ' ' else to_char(t1.NetWeight_B) end as "netweight_b",
                                                                                        case when t1.NetWeight_C = 0 then ' ' else to_char(t1.NetWeight_C) end as "netweight_c",
                                                                                        case when t1.NetWeight_M = 0 then ' ' else to_char(t1.NetWeight_M) end as "netweight_m",
                                                                                        case when t1.NetWeight_B_R = 0 then ' ' else to_char(t1.NetWeight_B_R) end as "netweight_b_r",
                                                                                        case when t1.NetWeight_C_R = 0 then ' ' else to_char(t1.NetWeight_C_R) end as "netweight_c_r",
                                                                                        case when t1.NetWeight_M_R = 0 then ' ' else to_char(t1.NetWeight_M_R) end as "netweight_m_r"
                                                                                        )
                                                                        )
                                                                        ORDER BY t1.DECK_NAME, t1.BA_FWD desc
                                                            )
                                                from (
                                                      -- только левые и, если есть, правые
                                                      select t1.*, t2.SEC_BAY_NAME SEC_BAY_NAME_R, t2.IATA_TYPE IATA_TYPE_R, t2.BEG_SER_NUM BEG_SER_NUM_R, t2.Tare_Weight Tare_Weight_R, t2.NetWeight NetWeight_R, t2.AP AP_R,
                                                        ':ONLOAD:' onload, ':SPECS:' specs, ':REPORT:' report,
                                                        case when t2.SEC_BAY_NAME is not null then ':ONLOAD:' else ' ' end onload_r,
                                                        case when t2.SEC_BAY_NAME is not null then ':SPECS:' else ' ' end specs_r,
                                                        case when t2.SEC_BAY_NAME is not null then ':REPORT:' else ' ' end report_r,
                                                        case when t2.SEC_BAY_NAME is not null then 1 else 0 end bLR,
                                                        t2.NetWeight_B NetWeight_B_R, t2.NetWeight_C NetWeight_C_R, t2.NetWeight_M NetWeight_M_R
                                                      from t_CPTs t1 left join t_CPTs t2 on t1.AP = t2.AP and t1.DECK_NAME = t2.DECK_NAME and t1.CMP_NAME = t2.CMP_NAME and t1.BA_FWD = t2.BA_FWD and t2.LR = 1
                                                      where t1.LR = -1
                                                      union all
                                                      -- только средние
                                                      select t1.*, ' ' SEC_BAY_NAME_R, ' ' IATA_TYPE_R, ' ' BEG_SER_NUM_R, 0 Tare_Weight_R, 0 NetWeight_R, ' ' AP_R,
                                                        ':ONLOAD:' onload, ':SPECS:' specs, ':REPORT:' report,
                                                        ' ' onload_r, ' ' specs_r, ' ' report_r,
                                                        0 bLR,
                                                        0 NetWeight_B, 0 NetWeight_C_R, 0 NetWeight_M_R
                                                      from t_CPTs t1
                                                      where t1.LR = 2
                                                      union all
                                                      -- только правые, если нет левых
                                                      select t1.DECK_NAME, t1.CMP_NAME, t1.BA_FWD, ' ' SEC_BAY_NAME, t1.LR, 0 IATA_ID, ' ' IATA_TYPE, ' ' BEG_SER_NUM, 0 Tare_Weight, 0 MAX_WEIGHT, ' ' AP, 0 NetWeight,
                                                        0 NetWeight_B, 0 NetWeight_C, 0 NetWeight_M,
                                                        t1.SEC_BAY_NAME SEC_BAY_NAME_R, t1.IATA_TYPE IATA_TYPE_R, t1.BEG_SER_NUM BEG_SER_NUM_R, t1.Tare_Weight Tare_Weight_R, t1.NetWeight NetWeight_R, t1.AP AP_R,
                                                        ' ' onload, ' ' specs, ' ' report,
                                                        ':ONLOAD:' onload, ':SPECS:' specs, ':REPORT:' report,
                                                        0 bLR,
                                                        t1.NetWeight_B NetWeight_B_R, t1.NetWeight_C NetWeight_C_R, t1.NetWeight_M NetWeight_M_R
                                                      from t_CPTs t1 left join t_CPTs t2 on t1.AP = t2.AP and t1.DECK_NAME = t2.DECK_NAME and t1.CMP_NAME = t2.CMP_NAME and t1.BA_FWD = t2.BA_FWD and t2.LR = -1
                                                      where (t1.LR = 1) and nvl(t2.LR, 0) = 0
                                                      union all
                                                      select ' ' DECK_NAME, ' ' CMP_NAME, 0 BA_FWD, ' ' SEC_BAY_NAME, 0 LR, 0 IATA_ID, ' ' IATA_TYPE, ' ' BEG_SER_NUM, 0 Tare_Weight, 0 MAX_WEIGHT, ' ' AP, 0 NetWeight,
                                                        0 NetWeight_B, 0 NetWeight_C, 0 NetWeight_M,
                                                        ' ' SEC_BAY_NAME_R, ' ' IATA_TYPE_R, ' ' BEG_SER_NUM_R, 0Tare_Weight_R, 0 NetWeight_R, ' ' AP_R,
                                                        ' ' onload, ' ' specs, ' ' report,
                                                        ':ONLOAD:' onload, ':SPECS:' specs, ':REPORT:' report,
                                                        0 bLR,
                                                        0 NetWeight_B_R, 0 NetWeight_C_R, 0 NetWeight_M_R
                                                      from dual
                                                      where (select count(1) from t_CPTs) = 0
                                                    ) t1
                                              ) as node

                                            -- здесь использовать оператор with для cCMPs(секции)
                                            /*
                                              tt1.cCPT_15 "cCPT_15", tt1.cInf_16 "cInf_16",
                                              tt1.cPass_13_14_15_16 "cTransitSpecs_13", tt1.cTotal_No_17 "cTotal_No_17",
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
                                              tt1.cDeadload_breakdown_46a "cDeadload_breakdown_46a", tt1.cChecked_47 "cChecked_47", tt1.cApproved_48 "cApproved_48"
                                            */
                            )
                )
  INTO cXML_data
  from
  (
    select cFrom_1, cTo_2, cFlight_3, cA_C_Reg_4, cVersion_5, cGate_6, cStand_7, cDate_8, cTime_9, cEd_No_10,
        cPlannedJoiningLoad_11, cJoiningSpecs_12, cTransitSpecs_13, cReloads_14
    from dual
  ) tt1;

  if cXML_data is not NULL then
  begin
    select replace(cXML_data, '" "', '""') into cXML_out from dual;
    -- cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_loadsheet" result="ok">' || cXML_data.getClobVal() || '</root>';
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_loadinstruction" result="ok">' || cXML_out || '</root>';
  end;
  end if;

  commit;
END SP_WBA_GET_LOADINSTRUCTION;
/

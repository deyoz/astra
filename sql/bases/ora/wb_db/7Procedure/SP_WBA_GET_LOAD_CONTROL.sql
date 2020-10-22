create or replace PROCEDURE SP_WBA_GET_LOAD_CONTROL
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; vID_AC number; cXML_SeatingConditions XMLType; vID_WS number; vID_BORT number; vID_SL number; vID number;
vCount number; vCount2 number; vCount3 number; vXMLParamIn clob; vXMLParamOut clob; cXML_distribution XMLType; cXML_zfw XMLType; cXML_tow XMLType; cXML_ldw XMLType; cXML_mac XMLType; cXML_itl XMLType;

 -- Пассажиры
vFcode number; vCcode number; vYcode number; vCode_Total number;
vFcodeTr number; vCcodeTr number; vYcodeTr number;

 -- Багаж
vFBag number; vCBag number; vYBag number;

-- Груз
vFCargo number; vCCargo number; vYCargo number;

-- Почта
vFMail number; vCMail number; vYMail number;

-- Веса
vPAX number; vPAX_curr number; vBAG number; vCargo number; vMail number; vTransiLoad number; vTotalTrafficLoad number; vAllowedTrafficLoad number; vDryOperatingWeight number; vCorrDryOperatingWeight number;
vZeroFuelWeight number; vWeightTemp number;
vTakeOffFuel number; vTakeOffWeight number; vTripFuel number; vLandingWeight number; vUnderloadBeforLMC number;
vAdultWeight number; vMaleWeight number; vFamaleWeight number; vChildWeight number; vInfantWeight number; vTransitLoad number; vTransitLoad_curr number; vBlockFuel number; vTaxiFuel number;
vCabinBaggageF number; vCabinBaggageC number; vCabinBaggageY number; vCabinBaggageTrF number; vCabinBaggageTrC number; vCabinBaggageTrY number; vTransitBag number; vTakeOffFuelIdx number; vLandingFuelIdx number;
vDryOperatingIndex number;
vMTOW number; vMLAW number; vMZFW number; vDOI number;
vTOW number; vLAW number; vZFW number;
vLI_ZFW number; vLI_TOW number; vLI_LAW number; vLI_ZFW_pass number; vLI_ZFW_bag number; vLI_ZFW_cb number; vIdxTemp number;
vMAC_ZFW number; vMAC_TOW number; vMAC_LAW number;
vC_CONST number; vK_CONST number; vREF_ARM number; vLEMAC_LERC number; vLEN_MAC_RC number;
vminweight number; vmaxweight number; vminweight_curr number; vmaxweight_curr number;
vEqWeight number; vEqIdx number;
vULDWeight number; vULDIdx number;
vCC_ID number; vCC_Name varchar(100);
vPT_ID number; vPT_Name varchar(100);
vPW_ID number; vPW_Name varchar(100);
cAHMDOW clob;
vCabC_ID number; vCabC_Name varchar(100);

temp_var number := 0; CurrWeight number := 0; CurrIDX number := 0; cIdx1 number := 0; cIdx2 number := 0; bZFW number := 0; bTOW number := 0; bLAW number := 0;

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
    where (t1.ELEM_ID = cP_ID) and (t1.DATA_NAME = 'DeadloadDetails') and (EXTRACTVALUE(value(b), '/row/@Position') is not null);

-- Курсор ZFW : Для проверки выхода за границы диапазона
CURSOR cZFWBounds (cID_AC NUMBER, cID_WS NUMBER) IS
  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = cID_AC and t1.ID_WS = cID_WS and t1.CH_TYPE = 'ZFW' and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select t3.WEIGHT, t3.INDX, t3.PROC_MAC, t3.WEIGHT WEIGHT_SORT
  from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                  inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                  inner join tblIDN t4 on t2.IDN = t4.IDN
  where t1.ID_AC = cID_AC and t1.ID_WS = cID_WS and t1.CH_TYPE = 'ZFW' -- and t1.ID_BORT = vID_BORT
  union all
  select t3.WEIGHT, t3.INDX, 100 - t3.PROC_MAC PROC_MAC, 1000000 - t3.WEIGHT WEIGHT_SORT
  from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                  inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                  inner join tblIDN t4 on t2.IDN = t4.IDN
  where t1.ID_AC = cID_AC and t1.ID_WS = cID_WS and t1.CH_TYPE = 'ZFW' -- and t1.ID_BORT = vID_BORT
  order by WEIGHT_SORT;

-- Курсор TOW : Для проверки выхода за границы диапазона
CURSOR cBounds (cID_AC NUMBER, cID_WS NUMBER, cTYPE varchar) IS
  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = cID_AC and t1.ID_WS = cID_WS and t1.CH_TYPE = cTYPE and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select t3.WEIGHT, t3.INDX, t3.PROC_MAC, t3.WEIGHT WEIGHT_SORT
  from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                  inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                  inner join tblIDN t4 on t2.IDN = t4.IDN
  where t1.ID_AC = cID_AC and t1.ID_WS = cID_WS and t1.CH_TYPE = cTYPE -- and t1.ID_BORT = vID_BORT
  union all
  select t3.WEIGHT, t3.INDX, 100 - t3.PROC_MAC PROC_MAC, 1000000 - t3.WEIGHT WEIGHT_SORT
  from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                  inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                  inner join tblIDN t4 on t2.IDN = t4.IDN
  where t1.ID_AC = cID_AC and t1.ID_WS = cID_WS and t1.CH_TYPE = cTYPE -- and t1.ID_BORT = vID_BORT
  order by WEIGHT_SORT;
BEGIN
  vPAX := 0; vBAG := 0; vCargo := 0; vMail := 0; vTransiLoad := 0; vTotalTrafficLoad := 0; vAllowedTrafficLoad := 0; vDryOperatingWeight := 0; vZeroFuelWeight := 0; vDryOperatingIndex := 0;
  vTakeOffFuel := 0; vTakeOffWeight := 0; vTripFuel := 0; vLandingWeight := 0; vUnderloadBeforLMC := 0;

  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC
  into vID_AC
  from WB_SHED t1 -- WB_REF_WS_SCHED_TEMP
  where t1.ID = P_ID;

  select t1.ID_WS
  into vID_WS
  from WB_SHED t1
  where t1.ID = P_ID;

  select t1.ID_BORT
  into vID_BORT
  from WB_SHED t1
  where t1.ID = P_ID;

  /*
  select t1.ID_SL
  into vID_SL
  from WB_SHED t1
  where t1.ID = P_ID;
  */

  cXML_out := ''; -- начальное обнуление

  -- Информация по пассажирам (начало)
  vFcode :=  0; vCcode := 0; vYcode := 0; vCode_Total := 0; vPAX := 0; vPAX_curr := 0; vCabinBaggageF := 0; vCabinBaggageC := 0; vCabinBaggageY := 0;

  FOR PassDetails IN cPassDetails(P_ID)
  LOOP
     -- process data record
    -- DBMS_OUTPUT.PUT_LINE('Name = ' || PassDetails.code);
    if PassDetails.code = 'F' then
    begin
      vFcode := vFcode + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
                + to_number(nvl(PassDetails.Child, 0)); --  + to_number(nvl(PassDetails.Infant, 0)); -- инфантов не включаем в кол-во пассажиров

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
                + to_number(nvl(PassDetails.Child, 0)); -- + to_number(nvl(PassDetails.Infant, 0)); -- инфантов не включаем в кол-во пассажиров

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
                + to_number(nvl(PassDetails.Child, 0)); -- + to_number(nvl(PassDetails.Infant, 0)); -- инфантов не включаем в кол-во пассажиров

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
      -- DBMS_OUTPUT.PUT_LINE('vPAX_curr = ' || vPAX_curr || ' Male = ' || PassDetails.Male || ' MaleWeight = ' || t1.MALE);
    end;
    end if;
  END LOOP;

  vCode_Total := vFcode + vCcode + vYcode;

  -- Подсчет TransitLoad (начало)
  vTransitLoad := 0; vTransitLoad_curr := 0; vCabinBaggageTrF := 0; vCabinBaggageTrC := 0; vCabinBaggageTrY := 0; vFcodeTr :=  0; vCcodeTr := 0; vYcodeTr := 0;

  FOR PassDetails IN cPassDetailsTr(P_ID)
  LOOP
     -- process data record
    -- DBMS_OUTPUT.PUT_LINE('Name = ' || PassDetails.code);
    if PassDetails.code = 'F' then
    begin
      vFcodeTr := vFcodeTr + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
                + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

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

      vCabinBaggageTrF := vCabinBaggageTrF + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;

    if PassDetails.code = 'C' then
    begin
      vCcodeTr := vCcodeTr + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
                + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

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

      vCabinBaggageTrC := vCabinBaggageTrC + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;

    if PassDetails.code = 'Y' then
    begin
      vYcodeTr := vYcodeTr + to_number(nvl(PassDetails.Adult, 0)) + to_number(nvl(PassDetails.Male, 0)) + to_number(nvl(PassDetails.Female, 0))
                + to_number(nvl(PassDetails.Child, 0)) + to_number(nvl(PassDetails.Infant, 0));

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

      vCabinBaggageTrY := vCabinBaggageTrY + to_number(nvl(PassDetails.CabinBaggage, 0));
    end;
    end if;
  END LOOP;
  -- Подсчет TransitLoad (конец)

  cXML_out := cXML_out || '<pass name="get_load_control_pass" result="ok">';

--  if (vFcode != 0) or (vFcodeTr != 0) then
    cXML_out := cXML_out || '<Class Code="F" Booked="0" CheckIn="' || to_char(vFcode) || '" Transit="' || to_char(vFcodeTr) || '"/>';
--  end if;

--  if (vCcode != 0) or (vCcodeTr != 0) then
    cXML_out := cXML_out || '<Class Code="C" Booked="0" CheckIn="' || to_char(vCcode) || '" Transit="' || to_char(vCcodeTr) || '"/>';
--  end if;

--  if (vYcode != 0) or (vYcodeTr != 0) then
    cXML_out := cXML_out || '<Class Code="Y" Booked="0" CheckIn="' || to_char(vYcode) || '" Transit="' || to_char(vYcodeTr) || '"/>';
--  end if;

  cXML_out := cXML_out || '</pass>';
  -- Информация по пассажирам (конец)

  -- Информация по багажу и грузу (начало)
  vFBag :=  0; vCBag := 0; vYBag := 0; vFCargo := 0; vCCargo := 0; vYCargo := 0; vFMail := 0; vCMail := 0; vYMail := 0; vTransitBag := 0;

  FOR DeadloadDetails IN cDeadloadDetails(P_ID)
  LOOP
    if (DeadloadDetails.TypeOfLoad = 'BF') -- and (DeadloadDetails.PositionLocked = 'Y')
    then
    begin
      vFBag := vFBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'BC') -- and (DeadloadDetails.PositionLocked = 'Y')
    then
    begin
      vCBag := vCBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'B') -- and (DeadloadDetails.PositionLocked = 'Y')
    then
    begin
      vYBag := vYBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'С') -- and (DeadloadDetails.PositionLocked = 'Y')
    then
    begin
      vYCargo := vYCargo + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'M') -- and (DeadloadDetails.PositionLocked = 'Y')
    then
    begin
      vYMail := vYMail + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'BT') -- and (DeadloadDetails.PositionLocked = 'Y')
    then
    begin
      vTransitLoad := vTransitLoad + to_number(DeadloadDetails.NetWeight);
    end;
    end if;

    if (DeadloadDetails.TypeOfLoad = 'BT') -- and (DeadloadDetails.PositionLocked = 'Y')
    then
    begin
      vTransitBag := vTransitBag + to_number(DeadloadDetails.NetWeight);
    end;
    end if;
  END LOOP;

  cXML_out := cXML_out || '<bagg name="get_load_control_bagg" result="ok">';

--  if (vFBag != 0) or (vCabinBaggageTrF != 0) then
    cXML_out := cXML_out || '<Class Code="F" Booked="0" CheckIn="' || to_char(vFBag) || '" Transit="' || to_char(0) || '"/>'; -- vCabinBaggageTrF
--  end if;

--  if (vCBag != 0) or (vCabinBaggageTrC != 0) then
    cXML_out := cXML_out || '<Class Code="C" Booked="0" CheckIn="' || to_char(vCBag) || '" Transit="' || to_char(0) || '"/>'; -- vCabinBaggageTrC
--  end if;

--  if (vYBag != 0) or (vCabinBaggageTrY != 0) then
    cXML_out := cXML_out || '<Class Code="Y" Booked="0" CheckIn="' || to_char(vYBag) || '" Transit="' || to_char(vTransitBag) || '"/>'; -- vCabinBaggageTrY
--  end if;

  cXML_out := cXML_out || '</bagg>';
  -- Информация по багажу (конец)

  -- Dry Operating Weight (начало)
  vDOI := 0; vCount := 0; vIdxTemp := 0; vCorrDryOperatingWeight := 0;

  select count(1)
  into vCount
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DOWData';

  if vCount > 0 then
  begin
    -- DryOperating
    vDryOperatingWeight := 0;
    vDryOperatingIndex := 0;

    select count(1)
    into vCount3
    from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
          inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
    where t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT -- and t2.ID = vID_SL
      and rownum <= 1;

    if vCount3 > 0 then
    begin
      select t1.DOW, t1.DOI
      into vDryOperatingWeight, vDryOperatingIndex
      from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
            inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
      where t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT -- and t2.ID = vID_SL
        and rownum <= 1;
    end;
    end if;

    vCorrDryOperatingWeight := vCorrDryOperatingWeight + vDryOperatingWeight;
    vDOI := vDOI + vDryOperatingIndex;

    --vCorrDryOperatingWeight := vCorrDryOperatingWeight + vULDWeight;
    --vDOI := vDOI + vULDIdx;

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
    SP_WBA_GET_DOW_REF(cXML_In, cAHMDOW);

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

  /*
  select count(1)
  into vCount
  from WB_TMP_XML t1
  where t1.ELEM_ID = P_ID and t1.TABLENAME = 'AHMDOW';

  if vCount > 0 then
  begin
    -- CrewCodes
    select sum(to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999')), sum(to_number(EXTRACTVALUE(value(b), 'code/@Weight'), '999999.9999'))
    into vIdxTemp, vWeightTemp
    from WB_TMP_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/CrewCodes/code'))) b
    where t1.ELEM_ID = P_ID and t1.TABLENAME = 'AHMDOW' and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

    vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили CrewCodes
    vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);

    -- SWACodes
    select sum(to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999')), sum(to_number(EXTRACTVALUE(value(b), 'code/@Weight'), '999999.9999'))
    into vIdxTemp, vWeightTemp
    from WB_TMP_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/SWACodes/code'))) b
    where t1.ELEM_ID = P_ID and t1.TABLENAME = 'AHMDOW';

    vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили SWACodes
    vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);

    -- PantryCodes
    select sum(to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999')), sum(to_number(EXTRACTVALUE(value(b), 'code/@Weight'), '999999.9999'))
    into vIdxTemp, vWeightTemp
    from WB_TMP_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/PantryCodes/code'))) b
    where t1.ELEM_ID = P_ID and t1.TABLENAME = 'AHMDOW' and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

    vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили PantryCodes

    -- PotableWaterCodes
    select sum(to_number(EXTRACTVALUE(value(b), 'code/@Idx'), '999999.9999')), sum(to_number(EXTRACTVALUE(value(b), 'code/@Weight'), '999999.9999'))
    into vIdxTemp, vWeightTemp
    from WB_TMP_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/PotableWaterCodes/code'))) b
    where t1.ELEM_ID = P_ID and t1.TABLENAME = 'AHMDOW' and (EXTRACTVALUE(value(b), 'code/@Included') != 'Y');

    vDOI := vDOI + nvl(vIdxTemp, 0); -- добавили PotableWaterCodes
    vCorrDryOperatingWeight := vCorrDryOperatingWeight + nvl(vWeightTemp, 0);
  end;
  end if;
  */
  -- Dry Operating Weight (конец)

  -- Fuel (начало)
  vCount := 0;
  select count(1)
  into vCount
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'FuelInfo';

  if vCount > 0 then
  begin
    select EXTRACTVALUE(value(b), '/fuel_info/@block_fuel'), EXTRACTVALUE(value(b), '/fuel_info/@taxi_fuel'), EXTRACTVALUE(value(b), '/fuel_info/@trip_fuel')
    into vBlockFuel, vTaxiFuel, vTripFuel
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/fuel_info'))) b
    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'FuelInfo';

    vTakeOffFuel := nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0);

    -- Получить Take-Off Fuel Idx
    vXMLParamIn := '<?xml version="1.0" ?>
<root><list>
                  <P_id_ac>' || to_char(vID_AC) || '</P_id_ac>
                  <P_id_ws>' || to_char(vID_WS) || '</P_id_ws>
                  <P_fuel>' || to_char(vTakeOffFuel) || '</P_fuel>
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
                  <P_fuel>' || to_char(vTakeOffFuel - vTripFuel) || '</P_fuel>
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
  -- Fuel (конец)

  -- Получить SeatingConditions (начало)
  select XMLAGG(
                XMLELEMENT("seating",
                            XMLATTRIBUTES('get_load_control_seating' as "name", 'ok' as "result"),
                            -- Набор вложенных элементов list
                            (
                              SELECT XMLAGG(
                                              XMLELEMENT(
                                                          "list",
                                                          XMLATTRIBUTES(tt1.CABIN_SECTION "Seat", 0 "Estimated", nvl(tt2.cnt, 0) "Actual", tt1.cnt_max "Maximum")
                                                        ) order by tt1.CABIN_SECTION
                                            )
                              from
                              (
                                select t1.CABIN_SECTION, count(1) cnt_max
                                from
                                (
                                  select t1.CABIN_SECTION, t1.ROW_NUMBER, t3.NAME Seat_Ident, LISTAGG(t5.NAME, '') within group (order by t5.NAME) Codes
                                  from WB_REF_WS_AIR_S_L_PLAN t1 inner join WB_REF_WS_AIR_S_L_P_S t2 on t1.ID = t2.PLAN_ID
                                                                    inner join WB_REF_WS_AIR_S_L_C_ADV t7 on t1.ID_AC = t7.ID_AC and t1.ID_WS = t7.ID_WS and t2.ADV_ID = t7.ADV_ID
                                                                    inner join WB_REF_WS_AIR_S_L_P_S_P t4 on t2.ID = t4.S_SEAT_ID
                                                                    inner join WB_REF_WS_SEATS_NAMES t3 on t2.SEAT_ID = t3.ID
                                                                    inner join WB_REF_WS_SEATS_PARAMS t5 on t4.PARAM_ID = t5.ID
                                                                    inner join WB_REF_WS_AIR_SL_CI_T t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t7.ID = t6.ADV_ID and (t1.ROW_NUMBER >= t6.FIRST_ROW) and (t1.ROW_NUMBER <= t6.LAST_ROW)
                                  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT -- and t7.IDN = vID_SL
                                  group by t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t3.NAME
                                  having (LISTAGG(t5.NAME, '') within group (order by t5.NAME)) not like '%Y%'
                                ) t1
                                group by t1.CABIN_SECTION
                              ) tt1
                              left join
                              -- Максимальное кол-во мест
                              (
                                select t1.SEAT_ID, count(t1.OccupiedBy) cnt
                                from
                                (
                                  select EXTRACTVALUE(value(b), '/section/@Name') SEAT_ID, EXTRACTVALUE(value(c), '/seat/@Ident') Ident, EXTRACTVALUE(value(c), '/seat/@OccupiedBy') OccupiedBy
                                  from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/section'))) b,
                                    table(XMLSequence( EXTRACT(value(b), '/section/row/seat'))) c
                                  where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'SeatingDetails'
                                ) t1
                                -- where t1.OccupiedBy not in ('AVAILABLE', 'BLOCKED');
                                where t1.OccupiedBy like ('%PAX%')
                                group by t1.SEAT_ID
                              ) tt2 on tt1.CABIN_SECTION = tt2.SEAT_ID
                            )
                          )
                )
  INTO cXML_SeatingConditions
  from dual;

  SYS.DBMS_LOB.APPEND(cXML_out, cXML_SeatingConditions.GetCLOBVal());
  -- Получить SeatingConditions (конец)

  -- vAllowedTrafficLoad (начало)
  select t1.MAX_ZF_WEIGHT, t1.MAX_TO_WEIGHT, t1.MAX_LND_WEIGHT
  into vMZFW, vMTOW, vMLAW
  from WB_REF_WS_AIR_MAX_WGHT_T t1
  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT;

  vMZFW := nvl(vMZFW, 0);
  vMTOW := nvl(vMTOW, 0);
  vMLAW := nvl(vMLAW, 0);

  -- Временно ввели величины для расчета Allowed Traffic Load, пока их нет в базе данных
  /*
  vMTOW := 179168; vMLAW := 136077; vMZFW := 117934;

  if vID_WS = 81 then -- 737-800
  begin
    vMTOW := 75000; vMLAW := 66360; vMZFW := 61688;
  end;
  end if;

  if vID_WS = 82 then
  begin
    vMTOW := 179168; vMLAW := 136077; vMZFW := 117934;
  end;
  end if;

  if vID_WS = 121 then -- 320-200
  begin
    vMTOW := 77000; vMLAW := 64500; vMZFW := 61000;
  end;
  end if;

  if vID_WS = 61 then -- 737-500
  begin
    vMTOW := 55000; vMLAW := 49895; vMZFW := 46720;
  end;
  end if;

  if vID_WS = 61 then -- 737-500
  begin
    vMTOW := 297556; vMLAW := 208652; vMZFW :=195044;
  end;
  end if;
  */

  select least(vMZFW + nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0) /*Take off Fuel*/, vMTOW, vMLAW + vTripFuel) - (vDryOperatingWeight + nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0)/*Take off Fuel*/)
  into vAllowedTrafficLoad
  from dual;

  -- min( 61688 - 11850, 75000, 24360) - (42000 + 11850)
  -- vAllowedTrafficLoad (конец)

  -- distribution (начало)
  with tblDeadloadDecks as
(
    select t1.ELEM_ID, EXTRACTVALUE(value(b), '/row/@TypeOfLoad') TypeOfLoad, EXTRACTVALUE(value(b), '/row/@Position') Position, EXTRACTVALUE(value(b), '/row/@IsBulk') IsBulk,
          EXTRACTVALUE(value(b), '/row/@ULDIATAType') ULDIATAType, EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Finalized') Finalized,
          EXTRACTVALUE(value(b), '/row/@PositionLocked') PositionLocked --, t1.XML_VALUE
    from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b
    where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
)

  select XMLAGG(
                  XMLELEMENT("distribution",
                              XMLATTRIBUTES('get_load_control_distribution' "name", 'ok' "result"),
                                            -- Вложенный элемент list
                                            (SELECT XMLAGG(
                                                            XMLELEMENT("list",
                                                                        XMLATTRIBUTES(t2.CMP_NAME "position", 0 "Estimated", t2.NetWeight as "Actual", t2.MAX_WEIGHT as "Maximum")
                                                                      ) ORDER BY t2.CMP_NAME
                                                        )
                                              from
                                              (
                                                select t3.CMP_NAME, t3.MAX_WEIGHT, sum(nvl(t11.NetWeight, 0)) NetWeight
                                                from WB_REF_WS_AIR_HLD_DECK t1 inner join (
                                                                                            select t1.ID, max(t1.DATE_WRITE) DATE_WRITE
                                                                                            from WB_REF_WS_AIR_HLD_DECK t1
                                                                                            where sysdate >= t1.DATE_WRITE
                                                                                            group by t1.ID
                                                                                           ) t12 on t1.ID = t12.ID
                                                                              inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                                                                              inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                                                                              inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                                                                              inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                                                                              inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                                                                              left join WB_REF_WS_AIR_SEC_BAY_TT t8 on t4.ID = t8.T_ID
                                                                              left join WB_REF_ULD_IATA t9 on t8.ULD_IATA_ID = t9.ID
                                                                              left join WB_REF_ULD_TYPES t10 on t9.TYPE_ID = t10.ID
                                                                              left join tblDeadloaddecks t11 on t4.SEC_BAY_NAME = t11.Position and t11.PositionLocked = 'Y'
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
                                                group by t3.CMP_NAME, t3.MAX_WEIGHT
                                              ) t2
                                            ) as node
                            )
                )
  INTO cXML_distribution
  from dual;

  if cXML_distribution is not null then
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_distribution.GetCLOBVal());
  end if;
/*
  cXML_out := cXML_out || '<distribution name="get_load_control_distribution" result="ok">';
  cXML_out := cXML_out || '<list position="0" Estimated="5" Actual="1" Maximum="12" />';
  cXML_out := cXML_out || '<list position="1" Estimated="15" Actual="3" Maximum="42" />';
  cXML_out := cXML_out || '<list position="2" Estimated="20" Actual="7" Maximum="42" />';
  cXML_out := cXML_out || '<list position="3" Estimated="15" Actual="12" Maximum="36" />';
  cXML_out := cXML_out || '</distribution>';
*/
  -- distribution (конец)

  vZFW := nvl(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad + vDryOperatingWeight, 0);
  vTOW := nvl(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad + vDryOperatingWeight + nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0), 0);
  vLAW := nvl(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad + vDryOperatingWeight + nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0) - vTripFuel, 0);

  -- Информация по весам (начало)
  cXML_out := cXML_out || '<wheit name="get_load_control_weight" result="ok">';
  cXML_out := cXML_out || '<list load="Allowed Traffic Load" Estimated="0" Actual="' || to_char(vAllowedTrafficLoad) || '" Maximum="" />';
  cXML_out := cXML_out || '<list load="PASSENGER/CABIN BAG" Estimated="" Actual="' || to_char(vPAX + nvl(vCabinBaggageF, 0) + nvl(vCabinBaggageC, 0) + nvl(vCabinBaggageY, 0)) || '" Maximum=""/>';
  cXML_out := cXML_out || '<list load="BAGGAGE" Estimated="0" Actual="' || to_char(vFBag + vCBag + vYBag) || '" Maximum=""/>';
  cXML_out := cXML_out || '<list load="Cargo" Estimated="0" Actual="' || to_char(vFCargo + vCCargo + vYCargo) || '" Maximum=""/>';
  cXML_out := cXML_out || '<list load="Mail" Estimated="0" Actual="' || to_char(vFMail + vCMail + vYMail) || '" Maximum=""/>';
  cXML_out := cXML_out || ' <list load="Transit Load" Estimated="0" Actual="' || to_char(vTransitLoad) || '" Maximum="" />';
  cXML_out := cXML_out || '<list load="Total Traffic Load" Estimated="0" Actual="' || to_char(vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad) || '" Maximum="" />';
  cXML_out := cXML_out || '<list load="" Estimated="" Actual="" Maximum="" />';
  cXML_out := cXML_out || '<list load="Dry Operating Weight" Estimated="0" Actual="' || to_char(vCorrDryOperatingWeight) || '" Maximum="" />';
  cXML_out := cXML_out || '<list load="Zero fuel Weight" Estimated="0" Actual="' || to_char(vZFW) || '" Maximum="' || to_char(vMZFW) || '" />';
  cXML_out := cXML_out || '<list load="Take off Fuel" Estimated="0" Actual="' || to_char(nvl(vBlockFuel, 0) - nvl(vTaxiFuel, 0)) || '" Maximum="" />';
  cXML_out := cXML_out || '<list load="Take off Weight" Estimated="0" Actual="' || to_char(vTOW) || '" Maximum="' || to_char(vMTOW) || '" />';
  cXML_out := cXML_out || '<list load="Trip Fuel" Estimated="0" Actual="' || to_char(vTripFuel) || '" Maximum="" />';
  cXML_out := cXML_out || '<list load="Landing Weight" Estimated="0" Actual="' || to_char(vLAW) || '" Maximum="' || to_char(vMLAW) || '" />';
  cXML_out := cXML_out || '<list load="" Estimated="" Actual="" Maximum="" />';
  cXML_out := cXML_out || '<list load="Underload Befor LMC" Estimated="0" Actual="' || to_char(vAllowedTrafficLoad - (vPAX + vFBag + vCBag + vYBag + vFCargo + vCCargo + vYCargo + vFMail + vCMail + vYMail + vTransitLoad)) || '" Maximum="" />';
  cXML_out := cXML_out || '</wheit>';
  -- Информация по весам (конец)

  -- Balance Conditions (начало)
  vLI_ZFW_pass := 0; vLI_ZFW_bag := 0; vLI_ZFW_cb := 0; vLI_ZFW := 0;

  -- Из всех зон в SeatingDetails вытащить пассажиров, вычислить их массу, умножить на коэффициенты по зонам (начало)
  select sum((sd.adult * nvl(t1.ADULT, 0) + sd.male * nvl(t1.MALE, 0) + sd.female * nvl(t1.FEMALE, 0) + sd.child * nvl(t1.CHILD, 0) + sd.infant * nvl(t1.INFANT, 0)) * t4.INDEX_PER_WT_UNIT)
  -- t4.INDEX_PER_WT_UNIT отрицательный, и LIZFW может быть отрицательным
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
                      from WB_REF_WS_AIR_CABIN_CD t1 inner join WB_REF_WS_AIR_S_L_C_ADV t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS -- and t1.ADV_ID = t2.ADV_ID
                      where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT -- Дима сказал, что если бортов нет, то так и надо. В общем, не знаю, убирать это условие или нет
                                                                    -- and t2.IDN = vID_SL
                    ) t4 on sd.SEAT_ID = t4.SECTION
  where t1.ID_AC = vID_AC;
  -- Из всех зон в SeatingDetails вытащить пассажиров, вычислить их массу, умножить на коэффициенты по зонам (конец)

  temp_var := vLI_ZFW_pass;

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
    from WB_REF_WS_AIR_CABIN_CD t1 inner join WB_REF_WS_AIR_S_L_C_ADV t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS -- and t1.ADV_ID = t2.ADV_ID
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT -- and t2.IDN = vID_SL
  ) t4 on cb.SEC_ID = t4.SECTION;
  -- Из всех зон в CabinBaggageByZone вытащить пассажиров, вычислить их массу, умножить на коэффициенты по зонам (конец)

  -- Из DeadloadDetails вытаскиваем груз, умножаем на коэффициенты (начало)
  select sum(dd.NetWeight * t4.INDEX_PER_WT_UNIT)
  into vLI_ZFW_bag
  from WB_REF_WS_AIR_HLD_DECK t1 inner join (
                                              select t1.ID, max(t1.DATE_WRITE) DATE_WRITE
                                              from WB_REF_WS_AIR_HLD_DECK t1
                                              where sysdate >= t1.DATE_WRITE
                                              group by t1.ID
                                             ) t12 on t1.ID = t12.ID
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
                                                ) t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS and t1.ID_BORT = t2.ID_BORT and t1.DATE_FROM = t2.DATE_FROM
  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
  order by t1.ID_AC, t1.ID_WS, t1.ID_BORT;

  vC_CONST := nvl(vC_CONST, 0);
  vK_CONST := nvl(vK_CONST, 0);
  vREF_ARM := nvl(vREF_ARM, 0);
  vLEMAC_LERC := nvl(vLEMAC_LERC, 0);
  vLEN_MAC_RC := nvl(vLEN_MAC_RC, 0);

  select (vC_CONST * (vLI_ZFW - vK_CONST) / (case when vZFW = 0 then 1 else vZFW end) + vREF_ARM - vLEMAC_LERC) * 100 / vLEN_MAC_RC
  into vMAC_ZFW
  from dual;

  select (vC_CONST * (vLI_TOW - vK_CONST) / (case when vTOW = 0 then 1 else vTOW end) + vREF_ARM - vLEMAC_LERC) * 100 / vLEN_MAC_RC -- vZFW
  into vMAC_TOW
  from dual;

  select (vC_CONST * (vLI_LAW - vK_CONST) / (case when vLAW = 0 then 1 else vLAW end) + vREF_ARM - vLEMAC_LERC) * 100 / vLEN_MAC_RC -- vZFW
  into vMAC_LAW
  from dual;
  -- Получаем коэффициенты MAC (конец)

  -- ZFW: Определение выхода за границы диапазона (начало)
  CurrWeight := 0;
  -- vLI_ZFW
  cIdx1 := -1000;
  cIdx2 := -1000;

  FOR ZFWBounds IN cZFWBounds(vID_AC, vID_WS)
  LOOP
     -- ищем соседние записи для определения отрезков, в которые попадает центровка
    if ((vZFW >= CurrWeight) and (vZFW <= ZFWBounds.WEIGHT))
    or ((vZFW >= ZFWBounds.WEIGHT) and (vZFW <= CurrWeight)) then
    begin
      -- отрезок между точками (x1 = CurrIDX, y1 = CurrWeight) и (x2 = ZFWBounds.INDX, y2 = ZFWBounds.WEIGHT)
      -- нужно найти индекс для веса vZFW
      if cIdx1 = -1000 then -- первую границу еще не вычисляли
      begin
        cIdx1 := CurrIDX + (ZFWBounds.INDX - CurrIDX) * (vZFW - CurrWeight) / (ZFWBounds.WEIGHT - CurrWeight);
      end;
      else -- вторая граница
      begin
        cIdx2 := CurrIDX + (ZFWBounds.INDX - CurrIDX) * (vZFW - CurrWeight) / (ZFWBounds.WEIGHT - CurrWeight);
      end;
      end if;
    end;
    end if;

    CurrWeight := ZFWBounds.WEIGHT;
    CurrIDX := ZFWBounds.INDX;
  END LOOP;

  bZFW := 0;
  if (cIdx1 != -1000) or (cIdx2 != -1000) then -- есть обе границы
  begin
    if (vLI_ZFW >= cIdx1) and (vLI_ZFW <= cIdx2) then
      begin
        bZFW := 1;
      end;
      else
      if (vLI_ZFW >= cIdx1) and (vLI_ZFW <= cIdx2) then
        begin
          bZFW := 1;
        end;
      end if;
    end if;
  end;
  end if;
  -- ZFW: Определение выхода за границы диапазона (конец)

  -- TOW: Определение выхода за границы диапазона (начало)
  CurrWeight := 0;
  -- vLI_TOW
  cIdx1 := -1000;
  cIdx2 := -1000;

  FOR Bounds IN cBounds(vID_AC, vID_WS, 'TOW')
  LOOP
     -- ищем соседние записи для определения отрезков, в которые попадает центровка
    if ((vTOW >= CurrWeight) and (vTOW <= Bounds.WEIGHT))
    or ((vTOW >= Bounds.WEIGHT) and (vTOW <= CurrWeight)) then
    begin
      -- отрезок между точками (x1 = CurrIDX, y1 = CurrWeight) и (x2 = ZFWBounds.INDX, y2 = ZFWBounds.WEIGHT)
      -- нужно найти индекс для веса vZFW
      if cIdx1 = -1000 then -- первую границу еще не вычисляли
      begin
        cIdx1 := CurrIDX + (Bounds.INDX - CurrIDX) * (vTOW - CurrWeight) / (Bounds.WEIGHT - CurrWeight);
      end;
      else -- вторая граница
      begin
        cIdx2 := CurrIDX + (Bounds.INDX - CurrIDX) * (vTOW - CurrWeight) / (Bounds.WEIGHT - CurrWeight);
      end;
      end if;
    end;
    end if;

    CurrWeight := Bounds.WEIGHT;
    CurrIDX := Bounds.INDX;
  END LOOP;

  bTOW := 0;
  if (cIdx1 != -1000) or (cIdx2 != -1000) then -- есть обе границы
  begin
    if (vLI_TOW >= cIdx1) and (vLI_TOW <= cIdx2) then
      begin
        bTOW := 1;
      end;
      else
      if (vLI_TOW >= cIdx1) and (vLI_TOW <= cIdx2) then
        begin
          bTOW := 1;
        end;
      end if;
    end if;
  end;
  end if;
  -- TOW: Определение выхода за границы диапазона (конец)

  -- LAW: Определение выхода за границы диапазона (начало)
-- t3.WEIGHT, t3.INDX, t3.PROC_MAC, t3.WEIGHT WEIGHT_SORT
  CurrWeight := 0;
  -- vLI_LAW
  cIdx1 := -1000;
  cIdx2 := -1000;

  FOR Bounds IN cBounds(vID_AC, vID_WS, 'LDW')
  LOOP
     -- ищем соседние записи для определения отрезков, в которые попадает центровка
    if ((vLAW >= CurrWeight) and (vLAW <= Bounds.WEIGHT))
    or ((vLAW >= Bounds.WEIGHT) and (vLAW <= CurrWeight)) then
    begin
      -- отрезок между точками (x1 = CurrIDX, y1 = CurrWeight) и (x2 = ZFWBounds.INDX, y2 = ZFWBounds.WEIGHT)
      -- нужно найти индекс для веса vLAW
      if cIdx1 = -1000 then -- первую границу еще не вычисляли
      begin
        cIdx1 := CurrIDX + (Bounds.INDX - CurrIDX) * (vLAW - CurrWeight) / (Bounds.WEIGHT - CurrWeight);
      end;
      else -- вторая граница
      begin
        cIdx2 := CurrIDX + (Bounds.INDX - CurrIDX) * (vLAW - CurrWeight) / (Bounds.WEIGHT - CurrWeight);
      end;
      end if;
    end;
    end if;

    CurrWeight := Bounds.WEIGHT;
    CurrIDX := Bounds.INDX;
  END LOOP;

  bLAW := 0;
  if (cIdx1 != -1000) or (cIdx2 != -1000) then -- есть обе границы
  begin
    if (vLI_LAW >= cIdx1) and (vLI_LAW <= cIdx2) then
      begin
        bLAW := 1;
      end;
      else
      if (vLI_LAW >= cIdx1) and (vLI_LAW <= cIdx2) then
        begin
          bLAW := 1;
        end;
      end if;
    end if;
  end;
  end if;
  -- LAW: Определение выхода за границы диапазона (конец)

  cXML_out := cXML_out ||
  '<balance name="get_load_control_balance" result="ok">
    <Estimated DOI="0" TakeOfFuel="0" LI_ZFW="0" LI_TOW="0" LI_LAW="0" ZFW_MAC="0" TOW_MAC="0" LAW_MAC="0" />'
    || '<Actual DOI="' || rtrim(to_char(vDOI,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- to_char(nvl(vDOI, 0), 'FM999990.99')
    || ' TakeOfFuel="' || rtrim(to_char(vTakeOffFuelIdx,'9990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- to_char(nvl(vTakeOffFuelIdx, 0), 'FM999990.99')
    || ' LI_ZFW="' || rtrim(to_char(vLI_ZFW,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- rtrim(to_char(nvl(vLI_ZFW, 0), 'FM999990.99'), '.')
    || ' LI_TOW="' || rtrim(to_char(vLI_TOW,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- to_char(vLI_TOW, 'FM999990.99') -- 9G990D00 -- 999G990D00
    || ' LI_LAW="' || rtrim(to_char(vLI_LAW,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- to_char(vLI_LAW, '999990.99'), '.')
    || ' ZFW_MAC="' || rtrim(to_char(vMAC_ZFW,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- to_char(vMAC_ZFW, '999990.99'), '.')
    || ' TOW_MAC="' || rtrim(to_char(vMAC_TOW,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- to_char(vMAC_TOW, '999990.99'), '.')
    || ' LAW_MAC="' || rtrim(to_char(vMAC_LAW,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"' -- to_char(vMAC_LAW, '999990.99'), '.')
    || ' TEMP_VAR="' || rtrim(to_char(temp_var,'9G990D00', 'NLS_NUMERIC_CHARACTERS = '',.'''), '.') || '"'
    || ' ZFW_Bounds="' || to_char(bZFW) || '"'
    || ' TOW_Bounds="' || to_char(bTOW) || '"'

    || ' vC_CONST="' || to_char(vC_CONST) || '"'
    || ' vLI_ZFW="' || to_char(vLI_ZFW) || '"'
    || ' vK_CONST="' || to_char(vK_CONST) || '"'
    || ' vZFW="' || to_char(vZFW) || '"'
    || ' vREF_ARM="' || to_char(vREF_ARM) || '"'
    || ' vLEMAC_LERC="' || to_char(vLEMAC_LERC) || '"'
    || ' vLEN_MAC_RC="' || to_char(vLEN_MAC_RC) || '"'

    || ' LAW_Bounds="' || to_char(bLAW) || '"/>'
    || '</balance>';
  -- Balance Conditions (конец)

/*
  select (vC_CONST * (vLI_ZFW - vK_CONST) / (case when vZFW = 0 then 1 else vZFW end) + vREF_ARM - vLEMAC_LERC) * 100 / vLEN_MAC_RC
  into vMAC_ZFW
  from dual;

*/

  -- Коэффициенты для колодцев (начало)
  -- cXML_zfw XMLType; cXML_tow XMLType; cXML_law XMLType;
  -- ZFW
  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'ZFW' and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select XMLAGG(
                  XMLELEMENT("zfw",
                              XMLATTRIBUTES('get_load_control_zfw' "name", 'ok' "result"),
                                            -- Вложенный элемент list
                                            (SELECT XMLAGG(
                                                            XMLELEMENT("list",
                                                                        XMLATTRIBUTES(t2.INDX as "index", t2.WEIGHT "Weight")
                                                                      ) ORDER BY t2.WEIGHT_SORT
                                                        )
                                              from
                                              (
                                                select t3.WEIGHT, t3.INDX, t3.PROC_MAC, t3.WEIGHT WEIGHT_SORT
                                                from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                                                                inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                                                                inner join tblIDN t4 on t2.IDN = t4.IDN
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'ZFW' -- and t1.ID_BORT = vID_BORT
                                                union all
                                                select t3.WEIGHT, t3.INDX, 100 - t3.PROC_MAC PROC_MAC, 1000000 - t3.WEIGHT WEIGHT_SORT
                                                from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                                                                inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                                                                inner join tblIDN t4 on t2.IDN = t4.IDN
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'ZFW' -- and t1.ID_BORT = vID_BORT
                                              ) t2
                                            ) as node
                            )
                )
  INTO cXML_zfw
  from dual;

  if cXML_zfw is not null then
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_zfw.GetCLOBVal());
  end if;

  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'ZFW' and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select min(t1.WEIGHT) minweight, max(t1.WEIGHT) maxweight
  into vminweight_curr, vmaxweight_curr
  from
  (
    select t3.WEIGHT
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                    inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                    inner join tblIDN t4 on t2.IDN = t4.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'ZFW' -- and t1.ID_BORT = vID_BORT
    union all
    select t3.WEIGHT
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                    inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                    inner join tblIDN t4 on t2.IDN = t4.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'ZFW' -- and t1.ID_BORT = vID_BORT
  ) t1;

  vminweight := nvl(vminweight_curr, 0);
  vmaxweight := nvl(vmaxweight_curr, 0);

  -- TOW
  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'TOW' and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select XMLAGG(
                  XMLELEMENT("tow",
                              XMLATTRIBUTES('get_load_control_tow' "name", 'ok' "result"),
                                            -- Вложенный элемент list
                                            (SELECT XMLAGG(
                                                            XMLELEMENT("list",
                                                                        XMLATTRIBUTES(t2.INDX as "index", t2.WEIGHT "Weight")
                                                                      ) ORDER BY t2.WEIGHT_SORT -- t2.PROC_MAC
                                                        )
                                              from
                                              (
                                                select t3.WEIGHT, t3.INDX, t3.PROC_MAC, t3.WEIGHT WEIGHT_SORT
                                                from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                                                                inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                                                                inner join tblIDN t4 on t2.IDN = t4.IDN
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'TOW' -- and t1.ID_BORT = vID_BORT
                                                union all
                                                select t3.WEIGHT, t3.INDX, 100 - t3.PROC_MAC PROC_MAC, 1000000 - t3.WEIGHT WEIGHT_SORT
                                                from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                                                                inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                                                                inner join tblIDN t4 on t2.IDN = t4.IDN
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'TOW' -- and t1.ID_BORT = vID_BORT
                                              ) t2
                                            ) as node
                            )
                )
  INTO cXML_tow
  from dual;

  if cXML_tow is not null then
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_tow.GetCLOBVal());
  end if;

  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'TOW' and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select min(t1.WEIGHT) minweight, max(t1.WEIGHT) maxweight
  into vminweight_curr, vmaxweight_curr
  from
  (
    select t3.WEIGHT
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                    inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                    inner join tblIDN t4 on t2.IDN = t4.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'TOW' -- and t1.ID_BORT = vID_BORT
    union all
    select t3.WEIGHT
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                    inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                    inner join tblIDN t4 on t2.IDN = t4.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'TOW' -- and t1.ID_BORT = vID_BORT
  ) t1;

  vminweight := least(nvl(vminweight_curr, 0), vminweight);
  vmaxweight := GREATEST(nvl(vmaxweight_curr, 0), vmaxweight);

  -- LAW
  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'LDW' and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select XMLAGG(
                  XMLELEMENT("law",
                              XMLATTRIBUTES('get_load_control_law' "name", 'ok' "result"),
                                            -- Вложенный элемент list
                                            (SELECT XMLAGG(
                                                            XMLELEMENT("list",
                                                                        XMLATTRIBUTES(t2.INDX as "index", t2.WEIGHT "Weight")
                                                                      ) ORDER BY t2.WEIGHT_SORT
                                                          )
                                              from
                                              (
                                                select t3.WEIGHT, t3.INDX, t3.PROC_MAC, t3.WEIGHT WEIGHT_SORT
                                                from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                                                                inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                                                                inner join tblIDN t4 on t2.IDN = t4.IDN
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'LDW' -- and t1.ID_BORT = vID_BORT
                                                union all
                                                select t3.WEIGHT, t3.INDX, 100 - t3.PROC_MAC PROC_MAC, 1000000 - t3.WEIGHT WEIGHT_SORT
                                                from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                                                                inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                                                                inner join tblIDN t4 on t2.IDN = t4.IDN
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'LDW' --  and t1.ID_BORT = vID_BORT
                                              ) t2
                                            ) as node
                            )
                )
  INTO cXML_ldw
  from dual;

  if cXML_ldw is not null then
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_ldw.GetCLOBVal());
  end if;

  -- Ideal Trim Line (Begin)
  with tblIDN as
  (
    select IDN
    from
    (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'Ideal_Trim_Line' -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
    )
    where rownum <= 1
  )
  select XMLAGG(
                  XMLELEMENT("itl",
                              XMLATTRIBUTES('get_ideal_trim_line' "name", 'ok' "result"),
                                            -- Вложенный элемент list
                                            (SELECT XMLAGG(
                                                            XMLELEMENT("list",
                                                                        XMLATTRIBUTES(t2.INDX as "index", t2.WEIGHT "Weight")
                                                                      ) ORDER BY t2.PROC_MAC
                                                        )
                                              from
                                              (
                                                select t3.WEIGHT, t3.INDX, t3.PROC_MAC
                                                from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                                                                inner join WB_REF_WS_AIR_GR_CH_ITL_L t3 on t2.ID = t3.ID_CH
                                                                                inner join tblIDN t4 on t2.IDN = t4.IDN
                                                where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'Ideal_Trim_Line' -- and t1.ID_BORT = vID_BORT
                                              ) t2
                                            ) as node
                            )
                )
  INTO cXML_itl
  from dual;

  if cXML_itl is not null then
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_itl.GetCLOBVal());
  end if;
  -- Ideal Trim Line (End)

  with tblIDN as
  (
    select t2.IDN
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'LDW' and rownum <= 1 -- and t1.ID_BORT = vID_BORT
    order by t2.TABLE_NAME
  )
  select min(t1.WEIGHT) minweight, max(t1.WEIGHT) maxweight
  into vminweight_curr, vmaxweight_curr
  from
  (
    select t3.WEIGHT
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                    inner join WB_REF_WS_AIR_GR_CH_F_L t3 on t2.ID = t3.ID_CH
                                    inner join tblIDN t4 on t2.IDN = t4.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'LDW' -- and t1.ID_BORT = vID_BORT
    union all
    select t3.WEIGHT
    from WB_REF_WS_AIR_GR_CH_IDN t1 inner join WB_REF_WS_AIR_GR_CH_ADV t2 on t1.ID = t2.IDN
                                    inner join WB_REF_WS_AIR_GR_CH_A_L t3 on t2.ID = t3.ID_CH
                                    inner join tblIDN t4 on t2.IDN = t4.IDN
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.CH_TYPE = 'LDW' -- and t1.ID_BORT = vID_BORT
  ) t1;

  vminweight := least(nvl(vminweight_curr, 0), vminweight);
  vmaxweight := GREATEST(nvl(vmaxweight_curr, 0), vmaxweight);

  /*
  cXML_out := cXML_out ||
  '<zfw name="get_load_control_zfw" result="ok">
    <list index="30.08" Weight="35000" />
    <list index="13.19" Weight="60250" />
    <list index="13.45" Weight="61688" />
    <list index="76.98" Weight="61688" />
    <list index="61.18" Weight="39470" />
    <list index="55.73" Weight="35000" />
  </zfw>
  <tow name="get_load_control_tow" result="ok">
    <list index="30.08" Weight="35000" />
    <list index="13.19" Weight="60250" />
    <list index="14.15" Weight="65543" />
    <list index="23.31" Weight="75000" />
    <list index="84.19" Weight="75000" />
    <list index="87.52" Weight="73055" />
    <list index="44.48" Weight="35000" />
  </tow>
  <law name="get_load_control_law" result="ok">
    <list index="30.08" Weight="35000" />
    <list index="13.19" Weight="60250" />
    <list index="14.11" Weight="65317" />
    <list index="37.04" Weight="66360" />
    <list index="84.24" Weight="66360" />
    <list index="65.5" Weight="40000" />
    <list index="59.43" Weight="35000" />
  </law>';
  */
  -- Коэффициенты для колодцев (конец)

  -- Линии САХ (начало)
  vC_CONST := nvl(vC_CONST, 0);
  vK_CONST := nvl(vK_CONST, 0);
  vREF_ARM := nvl(vREF_ARM, 0);
  vLEMAC_LERC := nvl(vLEMAC_LERC, 0);
  vLEN_MAC_RC := nvl(vLEN_MAC_RC, 0);

-- ((B5*('737-800'!$B$7))/100)+'737-800'!$B$8
-- ((G5*('737-800'!$B$7))/100)+'737-800'!$B$8

-- A3*((C3-('737-800'!$B$3))/'737-800'!$B$5)+'737-800'!$B$4
-- F3*((H3-('737-800'!$B$3))/'737-800'!$B$5)+'737-800'!$B$4

  select XMLAGG(
                  XMLELEMENT("mac",
                              XMLATTRIBUTES('get_load_control_mac' "name", 'ok' "result"),
                                            -- Вложенный элемент list
                                            (SELECT XMLAGG(
                                                            XMLELEMENT("list",
                                                                        XMLATTRIBUTES(t2.MAC as "macn", t2.x1 "x1", t2.y1 "y1", t2.x2 "x2", t2.y2 "y2")
                                                                      ) ORDER BY t2.MAC
                                                        )
                                              from
                                              (
                                                select vminweight Y1, t1.MAC, (vminweight * ((((t1.MAC * vLEN_MAC_RC / 100) + vLEMAC_LERC)  -  vREF_ARM  ) / vC_CONST) + vK_CONST) X1,
                                                      vmaxweight Y2, (vmaxweight * ((((t1.MAC * vLEN_MAC_RC / 100) + vLEMAC_LERC)  -  vREF_ARM  ) / vC_CONST) + vK_CONST) X2
                                                from
                                                (
                                                  select rownum MAC
                                                  from all_objects
                                                  where rownum <= 45
                                                ) t1
                                              ) t2
                                            ) as node
                            )
                )
  INTO cXML_mac
  from dual;

  if cXML_mac is not null then
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_mac.GetCLOBVal());
  end if;

/*
<mac name="get_load_control_mac" result="ok">
	<list macn="10" x1="29.38" y1="35000" x2="9.30" y2="80000"/>
	<list macn="11" x1="30" y1="35000" x2="12" y2="80000"/>
  </mac>
*/
  -- Линии САХ (конец)

  -- корневые теги
  cXML_out := '<root name="get_load_control" elem_id="' || to_char(P_ID) || '" data="' || to_char(sysdate) || '" time="" result="ok">' || cXML_out || '</root>';
  cXML_out := '<?xml version="1.0" ?>' || cXML_out;

  -- commit;
END SP_WBA_GET_LOAD_CONTROL;
/

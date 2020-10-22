create or replace procedure SP_WB_SAVE_FLIGHT
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number := -1;
P_AC_ID number := -1;
REC_COUNT number := 0;
REC_COUNT_2 number := 0;
P_AC_TYPE_ID number := -1;
P_AC_REG_ID number := -1;
P_CONFIG_ID number := -1;
P_ID_SL number := -1;
P_ID_OPER number := -1;
P_NR varchar(20) := '';
P_TEMP_ID number := -1;
TABLEVAR varchar(40):='PassengersDetails';
STD date;
ETD date;
cnt number := 0;
--ID_PORT_1 number := 0;
--ID_PORT_2 number := 0;
ICAO_CODE varchar(10) := '';

CURSOR cPorts IS
  select t1.ID, t3.ID AP1, t4.ID AP2
  from
  (
    select rownum id, EXTRACTVALUE(value(b), '/airport/@id') PORT
    from table(XMLSequence(Extract(xmltype(cXML_in), 'root[1]/legs[1]/airport'))) b
  ) t1
  inner join
  (
    select rownum id, EXTRACTVALUE(value(b), '/airport/@id') PORT
    from table(XMLSequence(Extract(xmltype(cXML_in), 'root[1]/legs[1]/airport'))) b
  ) t2 on t1.ID + 1 = t2.ID
  inner join WB_REF_AIRPORTS t3 on t1.PORT in (t3.IATA, t3.AP)
  inner join WB_REF_AIRPORTS t4 on t2.PORT in (t4.IATA, t4.AP)
  order by t1.ID;
begin
  -- select replace(cXML_in, '"set_query_save_set_flight"', '"get_query_save_set_flight"') into cXML_out from dual;

  -- 1 - Получить все входные параметры
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/aircompany[1]/@id')),
    to_number(extractValue(xmltype(cXML_in),'/root[1]/ac_type[1]/@id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/ac_reg[1]/@id')),
    to_number(extractValue(xmltype(cXML_in),'/root[1]/config[1]/@id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/nr[1]/@id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/nr[1]/@numb')),
    to_date(extractValue(xmltype(cXML_in),'/root[1]/std[1]/@value'), 'DD.MM.YYYY HH24:MI:SS'), to_date(extractValue(xmltype(cXML_in),'/root[1]/etd[1]/@value'), 'DD.MM.YYYY HH24:MI:SS')
  into P_ELEM_ID, P_AC_ID, P_AC_TYPE_ID, P_AC_REG_ID, P_CONFIG_ID, P_ID_OPER, P_NR, STD, ETD
  from dual;

  -- Код оператора
  select t1.ICAO_CODE
  into ICAO_CODE
  from WB_REF_AIRCOMPANY_ADV_INFO t1
  where t1.ID = 262 and rownum <= 1;

  select count(t1.ID)
  into REC_COUNT
  from WB_SCHED t1
  where t1.ID = P_ELEM_ID;

  if REC_COUNT > 0 then
  -- 2 - Если рейс существует - обновляем
  begin
    select t1.ID_SL
    into P_ID_SL
    from WB_SCHED t1
    where t1.ID = P_ELEM_ID;

    if P_ID_SL != P_CONFIG_ID then
    begin
      SP_WB_DELETE_CALCS(cXML_in, cXML_out); -- удаляем расчтеы для данного рейса
    end;
    end if;

    update WB_SCHED t1
    set t1.ID_AC = P_AC_ID, t1.ID_WS = P_AC_TYPE_ID, t1.ID_BORT = P_AC_REG_ID, t1.ID_SL = P_CONFIG_ID, t1.NR = ICAO_CODE || P_NR, t1.STD = STD, t1.ETD = ETD
    where t1.ID = P_ELEM_ID;
  end;
  else
  -- 3 - Если новый рейс - добавляем
  begin
    insert into WB_SCHED (ID, ID_AC, ID_WS, ID_BORT, ID_SL, NR, STD, ETD)
    select P_ELEM_ID, P_AC_ID, P_AC_TYPE_ID, P_AC_REG_ID, P_CONFIG_ID, ICAO_CODE || P_NR, STD, ETD
    from dual;
  end;
  end if;

  -- 4 - обновляем данные по участкам
  select count(t1.ID)
  into REC_COUNT
  from WB_SCHED_MRSHR t1
  where t1.ELEM_ID = P_ELEM_ID;

  if REC_COUNT > 0 then
  begin
    -- признак обновления
    update WB_SCHED_MRSHR t1
    set t1.SIGN = 0
    where t1.ELEM_ID = P_ELEM_ID;

    FOR Ports IN cPorts
    LOOP
      select count(t1.ID)
      into REC_COUNT_2
      from WB_SCHED_MRSHR t1
      where t1.ELEM_ID = P_ELEM_ID and t1.ID_AP1 = Ports.AP1 and t1.ID_AP2 = Ports.AP2;

      -- Нет нужного участка
      if REC_COUNT_2 = 0 then
      begin
        insert into WB_SCHED_MRSHR (ID_AP1, ID_AP2, ELEM_ID, SIGN)
        select Ports.AP1, Ports.AP2, P_ELEM_ID, 1 -- признак обновления
        from dual;
      end;
      else
      begin
        update WB_SCHED_MRSHR t1
        set t1.SIGN = 1 -- признак обновления
        where t1.ELEM_ID = P_ELEM_ID and t1.ID_AP1 = Ports.AP1 and t1.ID_AP2 = Ports.AP2;
      end;
      end if;
    END LOOP;

    -- Удаление всех остальных участков
    delete WB_SCHED_MRSHR t1
    where t1.ELEM_ID = P_ELEM_ID and t1.SIGN != 1;

    update WB_SCHED_MRSHR t1
    set t1.SIGN = 0
    where t1.ELEM_ID = P_ELEM_ID;
  end;
  else
  begin
    FOR Ports IN cPorts
    LOOP
      insert into WB_SCHED_MRSHR (ID_AP1, ID_AP2, ELEM_ID)
      select Ports.AP1, Ports.AP2, P_ELEM_ID
      from dual;
    END LOOP;
  end;
  end if;

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
-->
<root name="get_answer_save_set_flight" result="ok">
</root>';

  commit;
end SP_WB_SAVE_FLIGHT;
/

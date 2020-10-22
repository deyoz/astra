create or replace procedure SP_WBA_SET_FLIGHT
(cXML_in in clob,
   cXML_out out clob)
as
-- Сохранение рейса в центровке
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
TABLEVAR varchar(40) := 'PassengersDetails';
STD date;
ETD date;
cnt number := 0;
--ID_PORT_1 number := 0;
--ID_PORT_2 number := 0;
ICAO_CODE varchar(10) := '';
ID_AP1 number := 0;
ID_AP2 number := 0;
begin
  -- select replace(cXML_in, '"set_query_set_flight"', '"get_query_set_flight"') into cXML_out from dual;
/*
'<?xml version="1.0"?>
<root name="get_query_save_set_flight" elem_id="0">
  <aircompany id="242"/>
  <ac_type id="41"/><ac_reg id="%"/>
  <config id="%"/><nr id="262" numb=""/>
  <std value="01.01.2001 16:48"/>
  <etd value="01.01.2001 16:48"/>
  <legs><airport id="VKO"/><airport id="SVO"/></legs></root>';
*/
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
  where t1.ID_AC = P_AC_ID and rownum <= 1;

  select count(t1.ID)
  into REC_COUNT
  from WB_SHED t1
  where t1.ID = P_ELEM_ID;

  if REC_COUNT > 0 then
  -- 2 - Если рейс существует - обновляем
  begin
    /* на будущее - значение конфигурации в таблице расписания
    select t1.ID_SL
    into P_ID_SL
    from WB_SHED t1
    where t1.ID = P_ELEM_ID;

    if P_ID_SL != P_CONFIG_ID then
    begin
      SP_WB_DELETE_CALCS(cXML_in, cXML_out); -- удаляем расчтеы для данного рейса
    end;
    end if;
    */

    select t3.ID AP1, t4.ID AP2
    into ID_AP1, ID_AP2
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
    where rownum <= 1
    order by t1.ID;

    update WB_SHED t1
    set t1.ID_AC = P_AC_ID, t1.ID_WS = P_AC_TYPE_ID, t1.ID_BORT = P_AC_REG_ID, t1.NR = ICAO_CODE || P_NR, t1.S_DTL_1 = STD, t1.E_DTL_1 = ETD, -- , t1.ID_SL = P_CONFIG_ID -- на будущее - значение конфигурации в таблице расписания
      t1.ID_AP_1 = ID_AP1, t1.ID_AP_2 = ID_AP2
    where t1.ID = P_ELEM_ID;
  end;
  else
  -- 3 - Если новый рейс - добавляем
  begin
    insert into WB_SHED (ID, ID_AC, ID_WS, ID_BORT, NR, S_DTL_1, E_DTL_1, ID_AP_1, ID_AP_2, MVL_TYPE, U_NAME, U_IP, U_HOST_NAME, DATE_WRITE) -- , ID_SL -- на будущее - значение конфигурации в таблице расписания
    select nvl(P_ELEM_ID, 0), nvl(P_AC_ID, 0), nvl(P_AC_REG_ID, 0), nvl(P_CONFIG_ID, 0), nvl(ICAO_CODE, '') || nvl(P_NR, ''), nvl(STD, sysdate), nvl(ETD, sysdate), nvl(ID_AP1, 0), nvl(ID_AP2, 0),
      0, 'pdvl', '192.168.10.2', 'PSH-ПК', sysdate -- , P_AC_TYPE_ID
    from dual;
  end;
  end if;

/*
<!--
    Ответ.
-->
*/

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<root name="get_query_save_set_flight" result="ok">
</root>';

  commit;
end SP_WBA_SET_FLIGHT;
/

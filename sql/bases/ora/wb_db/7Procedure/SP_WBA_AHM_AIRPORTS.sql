create or replace PROCEDURE SP_WBA_AHM_AIRPORTS(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с единицами измерения по борту
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  cXML_out := '';

  select XMLAGG(
                  XMLELEMENT("airport",
                              XMLATTRIBUTES(tt1.ID "id", tt1.en_name "en_name", tt1.ru_name "ru_name", tt1.en_full_name "en_full_name", tt1.ru_full_name "ru_full_name", tt1.en_name "Code")
                            ) order by tt1.en_name
                )
  INTO cXML_data
  from
  (
    --select t1.ID, t1.IATA en_name, trim(t1.AP) ru_name, trim(t1.NAME_ENG_FULL) en_full_name, trim(t1.NAME_RUS_FULL) ru_full_name
    --from WB_REF_AIRPORTS t1

    select t5.ID, t5.IATA en_name, trim(t5.AP) ru_name, trim(t5.NAME_ENG_FULL) en_full_name, trim(t5.NAME_RUS_FULL) ru_full_name, t5.IATA
    from WB_SHED t1 inner join WB_REF_WS_AIR_TYPE t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS -- WB_SHED
                    inner join WB_REF_WS_TYPES t3 on t2.ID_WS = t3.ID
                    left join WB_REF_AIRCO_WS_BORTS t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.ID_BORT = t4.ID
                    inner join WB_REF_AIRPORTS t5 on t1.ID_AP_2 = t5.ID
    where t1.ID = P_ID

  ) tt1;

  if cXML_Data is not null then
  begin
    cXML_out := cXML_data.getClobVal();
  end;
  else
  begin
    cXML_out := '';
  end;
  end if;
END SP_WBA_AHM_AIRPORTS;
/

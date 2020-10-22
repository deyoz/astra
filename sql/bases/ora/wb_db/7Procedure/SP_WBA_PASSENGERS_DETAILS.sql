create or replace PROCEDURE SP_WBA_PASSENGERS_DETAILS(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с единицами измерения по борту
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; Units varchar2(100);
TABLEVAR varchar(40):='PassengersDetails';
REC_COUNT number:=0;
REC_COUNT2 number:=0;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  cXML_out := '';

  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM или из заглушки
  select count(t1.ELEM_ID)
  into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
  begin
    select t1.XML_DATA
    INTO cXML_out
    from
    (
      select t1.XML_DATA
      from WB_CALCS_XML t1
      where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
    ) t1
    where rownum <= 1;
  end;
  end if;

  if (length(cXML_out) = 0) or (cXML_out is null) then
  begin
    with tblARRs as
    (
      select t1.ID, t1.ID_AP_1, t1.ID_AP_2, 'русский' RU_ID, t6.CLASS_CODE
      from WB_SHED t1 inner join WB_REF_AIRPORTS t3 on t1.ID_AP_1 = t3.ID
                      inner join WB_REF_AIRPORTS t4 on t1.ID_AP_2 = t4.ID
                      inner join WB_REF_WS_AIR_S_L_C_ADV t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS -- and t1.ID_SL = t5.IDN -- компоновка - на будущее
                      inner join WB_REF_WS_AIR_SL_CI_T t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t5.ID = t6.ADV_ID -- and (t1.ROW_NUMBER >= t6.FIRST_ROW) and (t1.ROW_NUMBER <= t6.LAST_ROW)
      where t1.ID = P_ID
    )
    select XMLAGG(
                  XMLELEMENT("block",
                              XMLATTRIBUTES('get_passengers_details' as "name",
                                            P_ID as "elem_id",
                                            'ok' as "result"),
                              -- Набор вложенных элементов ARR
                              (select XMLAGG(
                                              XMLELEMENT("ARR",
                                                          XMLATTRIBUTES(t1.RU_ID as "ru_id",
                                                                        t1.ID_AP_2 as "id"),
                                                          -- Вложенный элемент On
                                                          (SELECT XMLAGG(
                                                                          XMLELEMENT("On",
                                                                                      XMLATTRIBUTES(t2.Weight as "Weight"),
                                                                                                    -- Следующий вложенный элемент
                                                                                                    (SELECT XMLAGG(
                                                                                                                    XMLELEMENT("class",
                                                                                                                                XMLATTRIBUTES(t3.CLASS_CODE as "code",
                                                                                                                                              '0' as "Male",
                                                                                                                                              '0' as "Female",
                                                                                                                                              '0' as "Child",
                                                                                                                                              '0' as "Infant",
                                                                                                                                              '0' as "CabinBaggage"
                                                                                                                                              )
                                                                                                                              ) ORDER BY t3.CLASS_CODE
                                                                                                                  )
                                                                                                      from
                                                                                                      (
                                                                                                        select CLASS_CODE
                                                                                                        from tblARRs
                                                                                                        WHERE ID_AP_2 = t1.ID_AP_2
                                                                                                      ) t3
                                                                                                    ) as node
                                                                                    ) ORDER BY t2.Weight
                                                                      )
                                                            from
                                                            (
                                                              select ' ' Weight from dual
                                                            ) t2
                                                          ) as node,
                                                          -- Вложенный элемент Tr
                                                          (SELECT XMLAGG(
                                                                          XMLELEMENT("Tr",
                                                                                      XMLATTRIBUTES(t2.Weight as "Weight"),
                                                                                                    -- Следующий вложенный элемент
                                                                                                    (SELECT XMLAGG(
                                                                                                                    XMLELEMENT("class",
                                                                                                                                XMLATTRIBUTES(t3.CLASS_CODE as "code",
                                                                                                                                              '0' as "Male",
                                                                                                                                              '0' as "Female",
                                                                                                                                              '0' as "Child",
                                                                                                                                              '0' as "Infant",
                                                                                                                                              '0' as "CabinBaggage"
                                                                                                                                              )
                                                                                                                              ) ORDER BY t3.CLASS_CODE
                                                                                                                  )
                                                                                                      from
                                                                                                      (
                                                                                                        select CLASS_CODE
                                                                                                        from tblARRs
                                                                                                        WHERE ID_AP_2 = t1.ID_AP_2
                                                                                                      ) t3
                                                                                                    ) as node
                                                                                    ) ORDER BY t2.Weight
                                                                      )
                                                            from
                                                            (
                                                              select '0' Weight from dual
                                                            ) t2
                                                          ) as node
                                                        ) ORDER BY t1.ID
                                            )
                                --INTO cXML_data
                                from
                                (
                                  select min(t.ID) ID, t.ID_AP_2, t.RU_ID
                                  from tblARRs t
                                  group by t.ID_AP_2, t.RU_ID
                                ) t1
                              ),
                              -- Вложенный элемент Total
                              (SELECT XMLAGG(
                                              XMLELEMENT("Total",
                                                          XMLATTRIBUTES(t2.Weight as "Weight"),
                                                                        -- Следующий вложенный элемент
                                                                        (SELECT XMLAGG(
                                                                                        XMLELEMENT("class",
                                                                                                    XMLATTRIBUTES(t3.CLASS_CODE as "code",
                                                                                                                  '0' as "Male",
                                                                                                                  '0' as "Female",
                                                                                                                  '0' as "Child",
                                                                                                                  '0' as "Infant",
                                                                                                                  '0' as "CabinBaggage"
                                                                                                                  )
                                                                                                  ) ORDER BY t3.CLASS_CODE
                                                                                      )
                                                                          from
                                                                          (
                                                                            select distinct CLASS_CODE
                                                                            from tblARRs
                                                                          ) t3
                                                                        ) as node
                                                        ) ORDER BY t2.Weight
                                          )
                                from
                                (
                                  select '0' Weight from dual
                                ) t2
                              ) as node
                            )
                  )
    INTO cXML_data
    from dual;

    if cXML_data is not NULL then
    begin
      select replace(cXML_data, 'ru_id=" "', 'ru_id=""') into cXML_out from dual;
      select replace(cXML_out, 'Weight=" "', 'Weight=""') into cXML_out from dual;
      select replace(cXML_out, 'Male=" "', 'Male=""') into cXML_out from dual;
      select replace(cXML_out, 'Female=" "', 'Female=""') into cXML_out from dual;
      select replace(cXML_out, 'Child=" "', 'Child=""') into cXML_out from dual;
      select replace(cXML_out, 'Infant=" "', 'Infant=""') into cXML_out from dual;
      select replace(cXML_out, 'CabinBaggage=" "', 'CabinBaggage=""') into cXML_out from dual;
    end;
    else
    begin
      cXML_out := '';
    end;
    end if;
  end;
  end if;
END SP_WBA_PASSENGERS_DETAILS;
/

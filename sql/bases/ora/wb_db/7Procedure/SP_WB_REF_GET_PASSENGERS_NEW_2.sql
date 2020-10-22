create or replace PROCEDURE SP_WB_REF_GET_PASSENGERS_NEW_2(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; P_IDN number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  cXML_out := '<?xml version="1.0"  encoding="utf-8"?>';

  with tblARRs as
  (
    select t2.ELEM_ID, t2.ID_AP1, t2.ID_AP2, ' ' RU_ID, t6.CLASS_CODE
    from WB_REF_WS_SCHED_TEMP t1 inner join WB_REF_SCHED_MRSHR_TEMP t2 on t1.ID = t2.ELEM_ID
                                inner join WB_REF_AIRPORTS t3 on t2.ID_AP1 = t3.ID
                                inner join WB_REF_AIRPORTS t4 on t2.ID_AP2 = t4.ID
                                inner join WB_REF_WS_AIR_S_L_C_ADV t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS and t1.ID_SL = t5.ID
                                inner join WB_REF_WS_AIR_SL_CI_T t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t5.ID = t6.ADV_ID -- and (t1.ROW_NUMBER >= t6.FIRST_ROW) and (t1.ROW_NUMBER <= t6.LAST_ROW)
    where t2.ELEM_ID = P_ID
  )

  select XMLAGG(
                XMLELEMENT("root",
                            XMLATTRIBUTES('get_passengers_details' as "name",
                                          P_ID as "elem_id",
                                          'ok' as "result"),
                            -- Набор вложенных элементов ARR
                            (select XMLAGG(
                                            XMLELEMENT("ARR",
                                                        XMLATTRIBUTES(t1.RU_ID as "ru_id",
                                                                      t1.ID_AP1 as "id"),
                                                        -- Вложенный элемент On
                                                        (SELECT XMLAGG(
                                                                        XMLELEMENT("On",
                                                                                    XMLATTRIBUTES(t2.Weight as "Weight"),
                                                                                                  -- Следующий вложенный элемент
                                                                                                  (SELECT XMLAGG(
                                                                                                                  XMLELEMENT("class",
                                                                                                                              XMLATTRIBUTES(t3.CLASS_CODE as "code",
                                                                                                                                            ' ' as "Male",
                                                                                                                                            ' ' as "Female",
                                                                                                                                            ' ' as "Child",
                                                                                                                                            ' ' as "Infant",
                                                                                                                                            ' ' as "CabinBaggage"
                                                                                                                                            )
                                                                                                                            ) ORDER BY t3.CLASS_CODE
                                                                                                                )
                                                                                                    from
                                                                                                    (
                                                                                                      select CLASS_CODE
                                                                                                      from tblARRs
                                                                                                      WHERE ID_AP1 = t1.ID_AP1
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
                                                                                                                                            ' ' as "Male",
                                                                                                                                            ' ' as "Female",
                                                                                                                                            ' ' as "Child",
                                                                                                                                            ' ' as "Infant",
                                                                                                                                            ' ' as "CabinBaggage"
                                                                                                                                            )
                                                                                                                            ) ORDER BY t3.CLASS_CODE
                                                                                                                )
                                                                                                    from
                                                                                                    (
                                                                                                      select CLASS_CODE
                                                                                                      from tblARRs
                                                                                                      WHERE ID_AP1 = t1.ID_AP1
                                                                                                    ) t3
                                                                                                  ) as node
                                                                                  ) ORDER BY t2.Weight
                                                                    )
                                                          from
                                                          (
                                                            select ' ' Weight from dual
                                                          ) t2
                                                        ) as node
                                                      ) -- ORDER BY t1.CABIN_SECTION
                                          )
                              --INTO cXML_data
                              from
                              (
                                select t.ID_AP1, t.RU_ID
                                from tblARRs t
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
                                                                                                                ' ' as "Male",
                                                                                                                ' ' as "Female",
                                                                                                                ' ' as "Child",
                                                                                                                ' ' as "Infant",
                                                                                                                ' ' as "CabinBaggage"
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
                                select ' ' Weight from dual
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
    cXML_out := '<?xml version="1.0" encoding="utf-8"?>' || cXML_out;
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_passengers_details" result="ok"></root>';
  end;
  end if;

END SP_WB_REF_GET_PASSENGERS_NEW_2;
/

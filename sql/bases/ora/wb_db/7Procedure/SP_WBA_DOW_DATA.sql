create or replace PROCEDURE SP_WBA_DOW_DATA(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с единицами измерения по борту
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number;
REC_COUNT number := 0;
REC_COUNT2 number := 0;
TABLEVAR varchar(40) := 'DOWData';
P_ID_AC number := 0;
P_ID_WS number := 0;
P_ID_BORT number := 0;
P_ID_SL number := 0;
WCC_ID number := 0;
WCC_CODE_NAME varchar(100) := '';
CR_ID number := 0;
CR_CODE_NAME varchar(100) := '';
PC_ID number := 0;
PT_CODE_NAME varchar(100) := '';
PW_CODE_ID number := 0;
PW_CODE_NAME varchar(100) := '';
cXML_Temp clob;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  cXML_out := '';

  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM
  select count(t1.ELEM_ID)
  into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
  begin
    select t1.XML_DATA
    INTO cXML_temp
    from
    (
      select t1.XML_DATA
      from WB_CALCS_XML t1
      where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
    ) t1
    where rownum <= 1;

    if length(cXML_temp) > 0 then
      SP_WBA_RECALC_DOW_DATA(cXML_temp, cXML_out); -- пересобираем XML с учетом изменений
    else
      cXML_out := '';
    end if;
  end;
  end if;

  -- if cXML_out = '' then
  if (length(cXML_out) = 0) or (cXML_out is null) then
  begin
    select t1.ID_AC, t1.ID_WS, t1.ID_BORT-- , t1.ID_SL -- компоновка - на будущее
    INTO P_ID_AC, P_ID_WS, P_ID_BORT -- , P_ID_SL
    from WB_SHED t1
    where t1.ID = P_ID and rownum <= 1;

    -- CabinConfiguration
    select XMLAGG(
                    XMLELEMENT("CabinConfiguration",
                                XMLATTRIBUTES(t1.ID "id",
                                              t1.TABLE_NAME "Name")
                              ) ORDER BY t1.ID
                  )
    into cXML_data
    from
    (
      select t2.ID, t2.TABLE_NAME
      from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
      where t1.ID_BORT = P_ID_BORT and rownum <= 1 -- and t2.ID = P_ID_SL -- компоновка - на будущее
      union all
      select 0 ID, ' ' TABLE_NAME
      from dual
    ) t1
    where rownum <= 1
    order by t1.ID desc;

    if cXML_data is not null then
    begin
      cXML_out := cXML_Data.GetCLOBVal();
    end;
    end if;

    -- Значения кодов
    select count(t1.ID)
    into REC_COUNT2
    from WB_REF_WS_AIR_WCC t1 left join WB_REF_WS_AIR_DOW_CR_CODES t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t3.BY_DEFAULT = 1
                            left join WB_REF_WS_AIR_DOW_PT_CODES t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t4.BY_DEFAULT = 1
                            left join WB_REF_WS_AIR_DOW_PW_CODES t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS and t5.BY_DEFAULT = 1
    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS;

    if REC_COUNT2 > 0 then
    begin
      select t1.ID wcc_id, t1.CODE_NAME, t1.CREW_CODE_ID, t3.CR_CODE_NAME, t1.PANTRY_CODE_ID, t4.PT_CODE_NAME, t1.PORTABLE_WATER_CODE_ID, t5.PW_CODE_NAME
      into WCC_ID, WCC_CODE_NAME, CR_ID, CR_CODE_NAME, PC_ID, PT_CODE_NAME, PW_CODE_ID, PW_CODE_NAME
      from WB_REF_WS_AIR_WCC t1 left join WB_REF_WS_AIR_DOW_CR_CODES t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t3.BY_DEFAULT = 1
                              left join WB_REF_WS_AIR_DOW_PT_CODES t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t4.BY_DEFAULT = 1
                              left join WB_REF_WS_AIR_DOW_PW_CODES t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS and t5.BY_DEFAULT = 1
      where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS and rownum <= 1;
    end;
    else
    begin
      select 0 wcc_id, ' ' CODE_NAME, 0 CREW_CODE_ID, ' ' CR_CODE_NAME, 0 PANTRY_CODE_ID, ' ' PT_CODE_NAME, 0 PORTABLE_WATER_CODE_ID, ' ' PW_CODE_NAME
      into WCC_ID, WCC_CODE_NAME, CR_ID, CR_CODE_NAME, PC_ID, PT_CODE_NAME, PW_CODE_ID, PW_CODE_NAME
      from dual;
    end;
    end if;

    -- WeightConfigurationCode
    select XMLAGG(
                    XMLELEMENT("WeightConfigurationCode",
                                XMLATTRIBUTES(t1.WCC_ID "id",
                                              t1.WCC_CODE_NAME "Name")
                              )
                  )
    into cXML_data
    from
    (
      select WCC_ID, WCC_CODE_NAME
      from dual
    ) t1;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
    end;
    end if;

    -- Captain
    select XMLAGG(
                    XMLELEMENT("Captain",
                                XMLATTRIBUTES(' ' "Name")
                              )
                  )
    into cXML_Data
    from dual;

    select replace(cXML_Data, '" "', '""')
    into cXML_Temp
    from dual;

    if cXML_Temp is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Temp);
    end;
    end if;

    -- CrewCode
    select XMLAGG(
                    XMLELEMENT("CrewCode",
                                XMLATTRIBUTES(t1.CR_ID "id",
                                              t1.CR_CODE_NAME "Name",
                                              'N' "IsPrint")
                              )
                  )
    into cXML_Data
    from
    (
      select CR_ID, CR_CODE_NAME
      from dual
    ) t1;

    if cXML_Data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Data.GetCLOBVal());
    end;
    end if;

    -- XCR
    select XMLAGG(
                    XMLELEMENT("XCR",
                                XMLATTRIBUTES('0' "Weight",
                                              '0.0' "Idx",
                                              ' ' "Remarks",
                                              'N' "IsPrint")
                              )
                  )
    into cXML_Data
    from dual;

    select replace(cXML_Data, '" "', '""')
    into cXML_Temp
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Temp);
    end;
    end if;

    -- DHC
    select XMLAGG(
                    XMLELEMENT("DHC",
                                XMLATTRIBUTES('0' "Weight",
                                              '0.0' "Idx",
                                              ' ' "Remarks",
                                              'N' "IsPrint")
                              )
                  )
    into cXML_Data
    from dual;

    select replace(cXML_Data, '" "', '""')
    into cXML_Temp
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Temp);
    end;
    end if;

    -- PantryCode
    select XMLAGG(
                    XMLELEMENT("PantryCode",
                                XMLATTRIBUTES(t1.PC_ID "id",
                                              t1.PT_CODE_NAME "Name",
                                              'N' "IsPrint")
                              )
                  )
    into cXML_Data
    from
    (
      select PC_ID, PT_CODE_NAME
      from dual
    ) t1;

    if cXML_Data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Data.GetCLOBVal());
    end;
    end if;

    -- PotableWaterCode
    select XMLAGG(
                    XMLELEMENT("PotableWaterCode",
                                XMLATTRIBUTES(t1.PW_CODE_ID "id",
                                              t1.PW_CODE_NAME "Name",
                                              'N' "IsPrint")
                              )
                  )
    into cXML_Data
    from
    (
      select PW_CODE_ID, PW_CODE_NAME
      from dual
    ) t1;

    if cXML_Data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Data.GetCLOBVal());
    end;
    end if;

    -- SWACodes
    select XMLAGG(
                    XMLELEMENT("SWACodes",
                                -- Набор вложенных элементов
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.ID "id",
                                                                            t1.CODE_NAME_1 "Name",
                                                                            'N' "IsPrint")
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t1.ID, t1.CODE_NAME_1
                                    from WB_REF_WS_AIR_DOW_SWA_CODES t1
                                    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                  ) t1
                                )
                              )
                  )
    into cXML_Data
    from dual;

    if cXML_Data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Data.GetCLOBVal());
    end;
    end if;

    -- OtherEq
    select XMLAGG(
                    XMLELEMENT("OtherEq",
                                XMLATTRIBUTES('0' "Weight",
                                              '0.0' "Idx",
                                              ' ' "Remarks",
                                              'N' "IsPrint")
                              )
                  )
    into cXML_Data
    from dual;

    select replace(cXML_Data, '" "', '""')
    into cXML_Temp
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_Temp);
    end;
    end if;

    -- ULDs
    if REC_COUNT > 0 then -- если есть запись в БД
    begin
      select t1.XML_DATA
      INTO cXML_temp
      from
      (
        select t1.XML_DATA
        from WB_CALCS_XML t1
        where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
      ) t1
      where rownum <= 1;

      if cXML_temp is not null then
      begin
        select XMLAGG(
                        XMLELEMENT("ULDs",
                                    XMLATTRIBUTES(t1.NetWeight "Weight",
                                                  t1.Idx "Idx",
                                                  ' ' "Remarks",
                                                  'N' "IsPrint")
                                  )
                      )
        into cXML_Data
        from
        (
          select sum(dd.NetWeight) NetWeight, to_char(sum(dd.NetWeight * t4.INDEX_PER_WT_UNIT), 'FM999990.99') Idx
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
                            from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_DATA), '/root/row'))) b
                            where t1.ELEM_ID = P_ID and t1.DATA_NAME = 'DeadloadDetails'
                          ) dd
                          where dd.Position is not null
                          group by dd.Position
                        ) dd on t4.SEC_BAY_NAME = dd.Position
          where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
        ) t1;

        select replace(cXML_Data, '" "', '""')
        into cXML_Temp
        from dual;
      end;
      else
      begin
        cXML_Temp := '<ULDs Weight="0" Idx="0" IsPrint="N"></ULDs>';
      end;
      end if;

      if cXML_data is not NULL then
      begin
        SYS.DBMS_LOB.APPEND(cXML_out, cXML_Temp);
      end;
      end if;
    end;
    end if;
  end;
  end if;
END SP_WBA_DOW_DATA;
/

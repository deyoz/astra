create or replace procedure SP_WB_RECALC_DOW_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number := -1;
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'DOWData';
vWCC varchar(100) := '';
vWCC_ID number;
vCREW_CODE_ID number;
vPT_CODE_ID number;
vPW_CODE_ID number;
vCR_CODE_NAME varchar(100);
vPT_CODE_NAME varchar(100);
vPW_CODE_NAME varchar(100);
P_ID_AC number;
P_ID_WS number;
P_ID_BORT number;
P_ID_SL number;
cXML_data XMLType;
WeightConfigurationCode varchar(100);
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  if P_ELEM_ID is not null then
  begin
    select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.ID_SL -- на будущее - компоновка
    INTO P_ID_AC, P_ID_WS, P_ID_BORT, P_ID_SL
    from WB_SCHED t1
    where t1.ID = P_ELEM_ID and rownum <= 1;

    -- Здесь нужно вычислить задаваемый WeightConfigurationCode (начало)
    -- Получили значения кодов.
    select EXTRACTVALUE(value(b), '/root/CrewCode/@id') CREW_CODE_ID, EXTRACTVALUE(value(b), '/root/CrewCode/@Name') CR_CODE_NAME,
        EXTRACTVALUE(value(b), '/root/PantryCode/@id') PT_CODE_ID, EXTRACTVALUE(value(b), '/root/PantryCode/@Name') PT_CODE_NAME,
        EXTRACTVALUE(value(b), '/root/PotableWaterCode/@id') PW_CODE_ID, EXTRACTVALUE(value(b), '/root/PotableWaterCode/@Name') PW_CODE_NAME
    into vCREW_CODE_ID, vCR_CODE_NAME, vPT_CODE_ID, vPT_CODE_NAME, vPW_CODE_ID, vPW_CODE_NAME
    from table(XMLSequence(Extract(xmltype(cXML_in), '/root'))) b
    where rownum <= 1;

    select count(1)
    into REC_COUNT
    from WB_REF_WS_AIR_WCC t1 left join WB_REF_WS_AIR_DOW_CR_CODES t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t1.CREW_CODE_ID = t3.ID
                            left join WB_REF_WS_AIR_DOW_PT_CODES t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.PANTRY_CODE_ID = t4.ID
                            left join WB_REF_WS_AIR_DOW_PW_CODES t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS and t1.PORTABLE_WATER_CODE_ID = t5.ID
    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
      and ((t1.CREW_CODE_ID = vCREW_CODE_ID) or (t3.CR_CODE_NAME = vCR_CODE_NAME))
      and ((t1.PANTRY_CODE_ID = vPT_CODE_ID) or (t4.PT_CODE_NAME = vPT_CODE_NAME))
      and ((t1.PORTABLE_WATER_CODE_ID = vPW_CODE_ID) or (t5.PW_CODE_NAME = vPW_CODE_NAME));

    -- Если есть значение, берем его, если нет, то пустая строка
    if REC_COUNT > 0 then
    begin
      select t1.ID, t1.CODE_NAME
      into vWCC_ID, vWCC
      from WB_REF_WS_AIR_WCC t1 left join WB_REF_WS_AIR_DOW_CR_CODES t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t1.CREW_CODE_ID = t3.ID
                              left join WB_REF_WS_AIR_DOW_PT_CODES t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.PANTRY_CODE_ID = t4.ID
                              left join WB_REF_WS_AIR_DOW_PW_CODES t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS and t1.PORTABLE_WATER_CODE_ID = t5.ID
      where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
        and ((t1.CREW_CODE_ID = vCREW_CODE_ID) or (t3.CR_CODE_NAME = vCR_CODE_NAME))
        and ((t1.PANTRY_CODE_ID = vPT_CODE_ID) or (t4.PT_CODE_NAME = vPT_CODE_NAME))
        and ((t1.PORTABLE_WATER_CODE_ID = vPW_CODE_ID) or (t5.PW_CODE_NAME = vPW_CODE_NAME))
        and rownum <= 1;

      if vWCC_ID is null then
      begin
        vWCC_ID := 0;
        vWCC := ' ';
      end;
      end if;
    end;
    end if;

    -- Пересобрать XML
    with t_Dowdata as
    (
        select EXTRACTVALUE(value(b), '/root/CabinConfiguration/@id') CONFIG_ID, EXTRACTVALUE(value(b), '/root/CabinConfiguration/@Name') CONFIG_NAME, -- CabinConfiguration
            EXTRACTVALUE(value(b), '/root/Captain/@Name') CaptainName, -- Captain
            EXTRACTVALUE(value(b), '/root/CrewCode/@id') CREW_CODE_ID, EXTRACTVALUE(value(b), '/root/CrewCode/@Name') CR_CODE_NAME, EXTRACTVALUE(value(b), '/root/CrewCode/@IsPrint') CR_CODE_IsPrint, -- CrewCode
            EXTRACTVALUE(value(b), '/root/XCR/@Weight') XCR_Weight, EXTRACTVALUE(value(b), '/root/XCR/@Idx') XCR_Idx,
            EXTRACTVALUE(value(b), '/root/XCR/@Remarks') XCR_Remarks, EXTRACTVALUE(value(b), '/root/XCR/@IsPrint') XCR_IsPrint, -- XCR
            EXTRACTVALUE(value(b), '/root/PantryCode/@id') PT_CODE_ID, EXTRACTVALUE(value(b), '/root/PantryCode/@Name') PT_CODE_NAME, EXTRACTVALUE(value(b), '/root/PantryCode/@IsPrint') PT_CODE_IsPrint, -- PantryCode
            EXTRACTVALUE(value(b), '/root/PotableWaterCode/@id') PW_CODE_ID, EXTRACTVALUE(value(b), '/root/PotableWaterCode/@Name') PW_CODE_NAME, EXTRACTVALUE(value(b), '/root/PotableWaterCode/@IsPrint') PW_CODE_IsPrint, -- PotableWaterCode
            EXTRACTVALUE(value(b), '/root/OtherEq/@Weight') OtherEq_Weight, EXTRACTVALUE(value(b), '/root/OtherEq/@Idx') OtherEq_Idx,
            EXTRACTVALUE(value(b), '/root/OtherEq/@Remarks') OtherEq_Remarks, EXTRACTVALUE(value(b), '/root/OtherEq/@IsPrint') OtherEq_IsPrint, -- OtherEq
            EXTRACTVALUE(value(b), '/root/ULDs/@Weight') ULDs_Weight, EXTRACTVALUE(value(b), '/root/ULDs/@Idx') ULDs_Idx,
            EXTRACTVALUE(value(b), '/root/ULDs/@Remarks') ULDs_Remarks, EXTRACTVALUE(value(b), '/root/ULDs/@IsPrint') ULDs_IsPrint -- ULDs
        from table(XMLSequence(Extract(xmltype(cXML_in), '/root'))) b
        where rownum <= 1
    )
    select XMLELEMENT("root",
                      XMLATTRIBUTES('get_dow_data' "name", P_ELEM_ID "elem_id", 'ok' "result"),
                      -- CabinConfiguration
                      XMLELEMENT(
                                  "CabinConfiguration",
                                  XMLATTRIBUTES(nvl(t1.CONFIG_ID, 0) "id", t1.CONFIG_NAME "Name")
                                ),
                      -- Captain
                      XMLELEMENT(
                                  "Captain",
                                  XMLATTRIBUTES(t1.CaptainName "Name")
                                ),
                      -- CrewCode
                      XMLELEMENT(
                                  "CrewCode",
                                  XMLATTRIBUTES(nvl(t1.CREW_CODE_ID, 0) "id", t1.CR_CODE_NAME "Name", t1.CR_CODE_IsPrint "IsPrint")
                                ),
                      -- XCR
                      XMLELEMENT(
                                  "XCR",
                                  XMLATTRIBUTES(t1.XCR_Weight "Weight", t1.XCR_Idx "Idx", t1.XCR_Remarks "Remarks", t1.XCR_IsPrint "IsPrint")
                                ),
                      -- PantryCode
                      XMLELEMENT(
                                  "PantryCode",
                                  XMLATTRIBUTES(nvl(t1.PT_CODE_ID, 0) "id", t1.PT_CODE_NAME "Name", t1.PT_CODE_IsPrint "IsPrint")
                                ),
                      -- PotableWaterCode
                      XMLELEMENT(
                                  "PotableWaterCode",
                                  XMLATTRIBUTES(nvl(t1.PW_CODE_ID, 0) "id", t1.PW_CODE_NAME "Name", t1.PW_CODE_IsPrint "IsPrint")
                                ),
                      -- OtherEq
                      XMLELEMENT(
                                  "OtherEq",
                                  XMLATTRIBUTES(t1.OtherEq_Weight "Weight", t1.OtherEq_Idx "Idx", t1.OtherEq_Remarks "Remarks", t1.OtherEq_IsPrint "IsPrint")
                                ),
                      -- ULDs
                      XMLELEMENT(
                                  "ULDs",
                                  XMLATTRIBUTES(t1.ULDs_Weight "Weight", t1.ULDs_Idx "Idx", t1.ULDs_Remarks "Remarks", t1.ULDs_IsPrint "IsPrint")
                                ),
                      -- WeightConfigurationCode
                      XMLELEMENT(
                                  "WeightConfigurationCode",
                                  XMLATTRIBUTES(nvl(vWCC_ID, 0) "id", vWCC "Name")
                                ),
                      -- SWACodes
                      XMLELEMENT(
                                  "SWACodes",
                                    -- Набор вложенных элементов
                                    (
                                      select XMLAGG(
                                                      XMLELEMENT("code",
                                                                  XMLATTRIBUTES(t1.SWACode "id",
                                                                                t1.SWAName "Name",
                                                                                t1.SWA_IsPrint "IsPrint")
                                                                ) ORDER BY t1.SWACode
                                                    )
                                      from
                                      (
                                        select EXTRACTVALUE(value(b), '/code/@id') SWACode, EXTRACTVALUE(value(b), '/code/@Name') SWAName, EXTRACTVALUE(value(b), '/code/@IsPrint') SWA_IsPrint
                                        from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/SWACodes/code'))) b
                                        where t1.ELEM_ID = P_ELEM_ID and t1.DATA_NAME = 'DOWData'
                                      ) t1
                                    )
                                 )
                  )
    INTO cXML_data
    from
    (
      select *
      from t_Dowdata
    ) t1;


    if cXML_Data is not null then
    begin
      select replace(cXML_Data, '" "', '""')
      into cXML_out
      from dual;

      cXML_out := '<?xml version="1.0" encoding="utf-8"?>' || cXML_out;
    end;
    end if;
    -- Здесь нужно вычислить задаваемый WeightConfigurationCode (конец)
  end;
  else
  begin
    cXML_out := cXML_in;
  end;
  end if;

  commit;
end SP_WB_RECALC_DOW_DATA;
/

create or replace PROCEDURE SP_WB_REF_GET_DOW_REF(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_Data XMLType;
P_ELEM_ID number := -1;
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'AHMDOW';
P_ID_AC number := 0; -- авиакомпания
P_ID_WS number := 0; -- тип ВС
P_ID_BORT number := 0; -- борт
P_ID_SL number := 0; -- конфигурация
ULD_Weight number := 0;
-- ULD_Idx varchar(20) := '';
ULD_Idx number := 0;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select count(t1.ID) into REC_COUNT
  from WB_SCHED t1
  where t1.ID = P_ELEM_ID;

  -- Если есть рейс
  if REC_COUNT > 0 then
  begin
    select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.ID_SL
    INTO P_ID_AC, P_ID_WS, P_ID_BORT, P_ID_SL
    from WB_SCHED t1
    where t1.ID = P_ELEM_ID and rownum <= 1;

/*
select t1.ULD_TYPE_ID, t1.ULD_IATA_ID, t1.TARE_WEIGHT, t3.NAME
from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_TYPES t2 on t1.ULD_TYPE_ID = t2.ID
                         inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
where t1.ID_AC = 242;
*/

    -- Считаем ULDs
    select sum(t5.TARE_WEIGHT) NetWeight, sum(t5.TARE_WEIGHT * t4.INDEX_PER_WT_UNIT) Idx -- to_char(sum(t5.TARE_WEIGHT * t4.INDEX_PER_WT_UNIT), 'FM999990.99') Idx
    into ULD_Weight, ULD_Idx
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
                    select * -- dd.ULDTypeID, dd.ULDIATAType --, sum(dd.NetWeight) NetWeight
                    from
                    (
                      select EXTRACTVALUE(value(b), '/row/@NetWeight') NetWeight, EXTRACTVALUE(value(b), '/row/@Position') Position, EXTRACTVALUE(value(b), '/row/@DOW') DOW,
                            EXTRACTVALUE(value(b), '/row/@ULDTypeID') ULDTypeID, EXTRACTVALUE(value(b), '/row/@ULDIATAType') ULDIATAType
                      from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/row'))) b
                      where t1.ELEM_ID = P_ELEM_ID and t1.DATA_NAME = 'DeadloadDetails'
                    ) dd
                    where (dd.Position is not null) and (dd.DOW = 'Y')
                    --group by dd.ULDTypeID
                  ) dd on t4.SEC_BAY_NAME = dd.Position -- and t4.SEC_BAY_NAME = dd.ULDTypeID
        inner join (
                    select distinct t3.NAME, t1.TARE_WEIGHT
                    from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_TYPES t2 on t1.ULD_TYPE_ID = t2.ID
                                             inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
                                             inner join (
                                                          /*
                                                          select t3.NAME, max(t1.DATE_WRITE) DATE_WRITE
                                                          from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_TYPES t2 on t1.ULD_TYPE_ID = t2.ID
                                                                                   inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
                                                            where t1.ID_AC = P_ID_AC
                                                          group by t3.NAME
                                                          */
                                                          select t1.ULD_IATA_ID, t3.NAME, 1 BY_DEFAULT, max(t1.DATE_WRITE) DATE_WRITE
                                                          from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
                                                          where t1.BY_DEFAULT = 1 and t1.ID_AC = P_ID_AC
                                                          group by t1.ULD_IATA_ID, t3.NAME, t1.DATE_WRITE
                                                          union all
                                                          select t1.ULD_IATA_ID, t3.NAME, 0 BY_DEFAULT, max(t1.DATE_WRITE) DATE_WRITE
                                                          from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
                                                                                  inner join (
                                                                                              select t1.ULD_IATA_ID, max(t1.BY_DEFAULT) BY_DEFAULT
                                                                                              from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
                                                                                              group by t1.ULD_IATA_ID
                                                                                              having max(t1.BY_DEFAULT) != 1
                                                                                              ) t2 on t1.ULD_IATA_ID = t2.ULD_IATA_ID
                                                          where t1.BY_DEFAULT != 1 and t1.ID_AC = P_ID_AC
                                                          group by t1.ULD_IATA_ID, t3.NAME, t1.DATE_WRITE
                                                        ) t4 on t3.NAME = t4.NAME and t1.DATE_WRITE = t4.DATE_WRITE
                    where t1.ID_AC = P_ID_AC
                  ) t5 on dd.ULDIATAType = t5.NAME
    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS;

    ULD_Weight := nvl(ULD_Weight, 0);
    ULD_Idx := nvl(ULD_Idx, 0);

    -- Конфигурация
    select XMLAGG(
                    XMLELEMENT("CabinConfigurations",
                                -- Набор вложенных элементов
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.ID "id",
                                                                            t1.TABLE_NAME "Name",
                                                                            t1.DOW "Dow",
                                                                            t1.DOI "Doi",
                                                                            t1.IsBasic "IsBasic",
                                                                            ULD_Weight "ULD_Weight",
                                                                            ULD_Idx "ULD_Idx")
                                                                            -- to_char(ULD_Idx, 'FM999990,09') "ULD_Idx") -- 'FM999990.09'
                                                                            -- to_char(ULD_Idx,'999990D00', 'NLS_NUMERIC_CHARACTERS = '',.''') "ULD_Idx")
                                                                            -- ULD_Idx "ULD_Idx")
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t2.ID, t2.TABLE_NAME, t1.DOW, t1.DOI, case when t3.CH_BASIC_WEIGHT = 1 then 'Y' else 'N' end IsBasic
                                    from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
                                                                inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
                                    where t1.ID_AC = t2.ID_AC and t1.ID_BORT = P_ID_BORT
                                  ) t1
                                )
                              )
                  )
    into cXML_data
    from dual;

    cXML_out := cXML_Data.GetCLOBVal();

    -- Start
    select XMLAGG(
                    XMLELEMENT("Start",
                                XMLATTRIBUTES(t1.DOW "Weight",
                                              t1.DOI "Idx",
                                              t1.IsBasic "IsBasic")
                              )
                  )
    into cXML_Data
    from (
          select t2.ID, t1.DOW, t1.DOI, case when t3.CH_BASIC_WEIGHT = 1 then 'Y' else 'N' end IsBasic
          from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
                inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
          where t1.ID_BORT = P_ID_BORT and t2.ID = P_ID_SL
        ) t1;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
    end;
    end if;

    -- CrewCodes
    select XMLAGG(
                    XMLELEMENT("CrewCodes",
                                -- Набор вложенных элементов
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.ID "id",
                                                                            t1.CR_CODE_NAME "Name",
                                                                            t1.WEIHGT_DIFF "Weight",
                                                                            t1.INDEX_DIFF "Idx",
                                                                            t1.FC_NUMBER "FlightCrew",
                                                                            t1.CC_NUMBER "CabinCrew",
                                                                            t1.Included "Included",
                                                                            t1.REMARKS "Remarks"
                                                                            )
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t1.ID, t1.CR_CODE_NAME, t1.WEIHGT_DIFF, to_char(t1.INDEX_DIFF, 'FM999990.99') INDEX_DIFF, t1.FC_NUMBER, t1.CC_NUMBER,
                                      case when t1.BY_DEFAULT = 1 then 'Y' else 'N' end Included, case when t1.REMARKS = 'EMPTY_STRING' then ' ' else t1.REMARKS end REMARKS
                                    from WB_REF_WS_AIR_DOW_CR_CODES t1
                                    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                  ) t1
                                )
                              )
                  )
    into cXML_data
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
    end;
    end if;

    -- PantryCodes
    select XMLAGG(
                    XMLELEMENT("PantryCodes",
                                -- Набор вложенных элементов
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.ID "id",
                                                                            t1.PT_CODE_NAME "Name",
                                                                            t1.WEIHGT_DIFF "Weight",
                                                                            t1.TOTAL_WEIGHT "TotalWeight",
                                                                            t1.INDEX_DIFF "Idx",
                                                                            t1.Included "Included",
                                                                            t1.REMARKS "Remarks"
                                                                            )
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t1.ID, t1.PT_CODE_NAME, t1.WEIHGT_DIFF, to_char(t1.INDEX_DIFF, 'FM999990.99') INDEX_DIFF,
                                      case when t1.BY_DEFAULT = 1 then 'Y' else 'N' end Included, case when t1.REMARKS = 'EMPTY_STRING' then ' ' else t1.REMARKS end REMARKS,
                                      to_char(t1.TOTAL_WEIGHT, 'FM999990.99') TOTAL_WEIGHT
                                    from WB_REF_WS_AIR_DOW_PT_CODES t1
                                    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                  ) t1
                                )
                              )
                  )
    into cXML_data
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
    end;
    end if;

    -- PotableWaterCodes
    select XMLAGG(
                    XMLELEMENT("PotableWaterCodes",
                                -- Набор вложенных элементов
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.ID "id",
                                                                            t1.PW_CODE_NAME "Name",
                                                                            t1.WEIHGT_DIFF "Weight",
                                                                            t1.TOTAL_WEIGHT "TotalWeight",
                                                                            t1.INDEX_DIFF "Idx",
                                                                            t1.Included "Included",
                                                                            t1.REMARKS "Remarks"
                                                                            )
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t1.ID, t1.PW_CODE_NAME, t1.WEIHGT_DIFF, to_char(t1.INDEX_DIFF, 'FM999990.99') INDEX_DIFF,
                                      case when t1.BY_DEFAULT = 1 then 'Y' else 'N' end Included, case when t1.REMARKS = 'EMPTY_STRING' then ' ' else t1.REMARKS end REMARKS,
                                      to_char(t1.TOTAL_WEIGHT, 'FM999990.99') TOTAL_WEIGHT
                                    from WB_REF_WS_AIR_DOW_PW_CODES t1
                                    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                  ) t1
                                )
                              )
                  )
    into cXML_data
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
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
                                                                            t1.WEIGHT "Weight",
                                                                            t1.INDEX_UNIT "Idx",
                                                                            t1.Included "Included",
                                                                            t1.REMARKS "Remarks"
                                                                            )
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t1.ID, t1.CODE_NAME_1, t1.WEIGHT, to_char(t1.INDEX_UNIT, 'FM999990.99') INDEX_UNIT,
                                      'N' Included, t1.REMARKS
                                    from WB_REF_WS_AIR_DOW_SWA_CODES t1
                                    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                  ) t1
                                )
                              )
                  )
    into cXML_data
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
    end;
    end if;

    -- WeightConfigurationCodes
    select XMLAGG(
                    XMLELEMENT("WeightConfigurationCodes",
                                -- Набор вложенных элементов
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.wcc_id "wcc_id",
                                                                            t1.CODE_NAME "Name",
                                                                            t1.CREW_CODE_ID "crew_code_id",
                                                                            t1.CR_CODE_NAME "CrewCodeName",
                                                                            t1.PANTRY_CODE_ID "pantry_code_id",
                                                                            t1.PT_CODE_NAME "PantryCodeName",
                                                                            t1.PORTABLE_WATER_CODE_ID "portable_water_code_id",
                                                                            t1.PW_CODE_NAME "PotableWaterCodeName",
                                                                            'Domestic' "SWACodeNames0",
                                                                            'Domestic' "SWACodeNames1",
                                                                            'Domestic' "SWACodeNames2"
                                                                            ),
                                                              -- SWA Codes
                                                              (
                                                                select XMLAGG(
                                                                                XMLELEMENT("swacodes",
                                                                                            XMLATTRIBUTES(t1.ADJ_CODE_ID "id",
                                                                                                          t1.CODE_NAME_1 "Name"
                                                                                                          )
                                                                                          ) ORDER BY t1.ADJ_CODE_ID
                                                                              )
                                                                from
                                                                (
                                                                  select t1.ADJ_CODE_ID, t2.CODE_NAME_1
                                                                  from WB_REF_WS_AIR_WCC_AC t1 inner join WB_REF_WS_AIR_DOW_SWA_CODES t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS and t1.ADJ_CODE_ID = t2.ID
                                                                  where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                                                ) t1
                                                              )
                                                            ) ORDER BY t1.wcc_id
                                                )
                                  from
                                  (
                                    select t1.ID wcc_id, t1.CODE_NAME,
                                        case when t3.ID is null then t6.ID else t3.ID end CREW_CODE_ID,
                                        case when t3.ID is null then t6.CR_CODE_NAME else t3.CR_CODE_NAME end CR_CODE_NAME,
                                        case when t4.ID is null then t7.ID else t4.ID end PANTRY_CODE_ID,
                                        case when t4.ID is null then t7.PT_CODE_NAME else t4.PT_CODE_NAME end PT_CODE_NAME,
                                        case when t5.ID is null then t8.ID else t5.ID end PORTABLE_WATER_CODE_ID,
                                        case when t5.ID is null then t8.PW_CODE_NAME else t5.PW_CODE_NAME end PW_CODE_NAME
                                    from WB_REF_WS_AIR_WCC t1 left join WB_REF_WS_AIR_DOW_CR_CODES t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t1.CREW_CODE_ID = t3.ID
                                                            left join WB_REF_WS_AIR_DOW_PT_CODES t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.PANTRY_CODE_ID = t4.ID
                                                            left join WB_REF_WS_AIR_DOW_PW_CODES t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS and t1.PORTABLE_WATER_CODE_ID = t5.ID
                                                            left join WB_REF_WS_AIR_DOW_CR_CODES t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and (t3.ID is null) and t6.BY_DEFAULT = 1
                                                            left join WB_REF_WS_AIR_DOW_PT_CODES t7 on t1.ID_AC = t7.ID_AC and t1.ID_WS = t7.ID_WS and (t4.ID is null) and t7.BY_DEFAULT = 1
                                                            left join WB_REF_WS_AIR_DOW_PW_CODES t8 on t1.ID_AC = t8.ID_AC and t1.ID_WS = t8.ID_WS and (t5.ID is null) and t8.BY_DEFAULT = 1
                                    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                  ) t1
                                )
                              )
                  )
    into cXML_data
    from dual;

    if cXML_data is not NULL then
    begin
      SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
    end;
    end if;

    -- В конце оборачиваем в root
    if cXML_out is not null then
    begin
      cXML_out := '<root name="get_ahm_dow_referencies" result="ok">' || cXML_out || '</root>';
    end;
    end if;
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_dow_referencies" result="ok"></root>';
  end;
  end if;
END SP_WB_REF_GET_DOW_REF;
/

create or replace PROCEDURE SP_WB_REF_GET_DOW_REF_TEMP (cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_Data XMLType;
P_ELEM_ID number := -1;
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'AHMDOW';
P_ID_AC number := 0; -- ������������
P_ID_WS number := 0; -- ⨯ ��
P_ID_BORT number := 0; -- ����
P_ID_SL number := 0; -- ���䨣����
BEGIN
  -- ������ �室��� ��ࠬ���
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select count(t1.ID) into REC_COUNT
  from WB_SCHED t1
  where t1.ID = P_ELEM_ID;

  -- �᫨ ���� ३�
  if REC_COUNT > 0 then
  begin
    select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.ID_SL
    INTO P_ID_AC, P_ID_WS, P_ID_BORT, P_ID_SL
    from WB_SCHED t1
    where t1.ID = P_ELEM_ID and rownum <= 1;

    -- ���䨣����
    select XMLAGG(
                    XMLELEMENT("CabinConfigurations",
                                -- ����� ��������� ����⮢
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.ID "id",
                                                                            t1.TABLE_NAME "Name")
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t2.ID, t2.TABLE_NAME
                                    from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
                                    where t1.ID_BORT = P_ID_BORT
                                  ) t1
                                )
                              )
                  )
    into cXML_data
    from dual;

    cXML_out := cXML_Data.GetCLOBVal();

    -- CrewCodes
    select XMLAGG(
                    XMLELEMENT("CrewCodes",
                                -- ����� ��������� ����⮢
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
                                    select t1.ID, t1.CR_CODE_NAME, t1.WEIHGT_DIFF, to_char(t1.INDEX_DIFF, 'FM999990.99') INDEX_DIFF, t1.FC_NUMBER, t1.CC_NUMBER, 'N' Included, t1.REMARKS
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
                                -- ����� ��������� ����⮢
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("code",
                                                              XMLATTRIBUTES(t1.ID "id",
                                                                            t1.PT_CODE_NAME "Name",
                                                                            t1.WEIHGT_DIFF "Weight",
                                                                            t1.INDEX_DIFF "Idx",
                                                                            t1.Included "Included",
                                                                            t1.REMARKS "Remarks"
                                                                            )
                                                            ) ORDER BY t1.ID
                                                )
                                  from
                                  (
                                    select t1.ID, t1.PT_CODE_NAME, t1.WEIHGT_DIFF, to_char(t1.INDEX_DIFF, 'FM999990.99') INDEX_DIFF, 'N' Included, t1.REMARKS
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
                                -- ����� ��������� ����⮢
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
                                    select t1.ID, t1.PW_CODE_NAME, t1.WEIHGT_DIFF, to_char(t1.INDEX_DIFF, 'FM999990.99') INDEX_DIFF, 'N' Included, t1.REMARKS, to_char(t1.TOTAL_WEIGHT, 'FM999990.99') TOTAL_WEIGHT
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
                                -- ����� ��������� ����⮢
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
                                    select t1.ID, t1.CODE_NAME_1, t1.WEIGHT, to_char(t1.INDEX_UNIT, 'FM999990.99') INDEX_UNIT, 'N' Included, t1.REMARKS
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
                                -- ����� ��������� ����⮢
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

    -- � ���� ����稢��� � root
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
END SP_WB_REF_GET_DOW_REF_TEMP;
/

create or replace PROCEDURE SP_WBA_AHM_PASS_BAG_WEIGHT(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- «®ª ¤ ­­ëå á ¥¤¨­¨æ ¬¨ ¨§¬¥à¥­¨ï ¯® ¡®àâã
-- ‘®§¤ «:  ¡ â®¢ ’.….
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number;
BEGIN
  -- ®«ãç¨âì ¢å®¤­®© ¯ à ¬¥âà
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ‡„…‘œ €—ˆ€…’‘Ÿ ˆ„ˆ‚ˆ„“€‹œ€Ÿ —€‘’œ
  cXML_out := '';

  with tblPassBag as
  (
    select distinct t1.ID ELEM_ID, t9.NAME_ENG_SMALL Units, t7.ID ID_CLASS, t6.CLASS_CODE, t10.ADULT, t10.MALE, t10.FEMALE, t10."CHILD", t10.INFANT
    from WB_SHED t1 inner join WB_REF_AIRPORTS t3 on t1.ID_AP_1 = t3.ID
                    inner join WB_REF_AIRPORTS t4 on t1.ID_AP_2 = t4.ID
                    inner join WB_REF_WS_AIR_S_L_C_ADV t5 on t1.ID_AC = t5.ID_AC and t1.ID_WS = t5.ID_WS -- and t1.ID_SL = t5.IDN -- ª®¬¯®­®¢ª  - ­  ¡ã¤ãé¥¥
                    -- left join WB_REF_WS_AIR_SL_CI_T t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t5.ID = t6.ADV_ID
                    inner join (
                                select t6.ID_AC, t6.ADV_ID, t6.ID_WS, t6.ID_BORT, t6.CLASS_CODE, sum(t6.NUM_OF_SEATS) cnt
                                from WB_REF_WS_AIR_SL_CAI_TT t6
                                group by t6.ID_AC, t6.ID_WS, t6.ID_BORT, t6.ADV_ID, t6.CLASS_CODE
                                having sum(t6.NUM_OF_SEATS) > 0
                              ) t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t5.ID = t6.ADV_ID
                    inner join WB_REF_AIRCO_CLASS_CODES t7 on t1.ID_AC = t7.ID_AC and t6.CLASS_CODE = t7.CLASS_CODE
                    inner join WB_REF_AIRCO_MEAS t8 on t1.ID_AC = t8.ID_AC
                    inner join WB_REF_MEASUREMENT_WEIGHT t9 on t8.WEIGHT_ID = t9.ID
                    inner join WB_REF_AIRCO_PAX_WEIGHTS t10 on t7.ID_AC = t10.ID_AC and t7.ID = t10.ID_CLASS
                    inner join (
                                  select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) DATE_FROM
                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                  group by t1.ID_AC, t1.ID_CLASS
                                ) t11 on t10.ID_AC = t11.ID_AC and t10.ID_CLASS = t11.ID_CLASS and t10.DATE_FROM = t11.DATE_FROM
    where t1.ID = P_ID
  )
  SELECT XMLAGG(
                  XMLELEMENT(
                              "weights",
                              XMLATTRIBUTES(tt1.Units "Units"),
                              (SELECT XMLAGG(
                                            XMLELEMENT("class",
                                                       XMLATTRIBUTES(
                                                                      tt2.CLASS_CODE as "code",
                                                                      to_char(tt2.Adult) AS "Adult",
                                                                      to_char(decode(tt2.adult, null, tt2.male, null)) AS "Male",
                                                                      to_char(decode(tt2.adult, null, tt2.female, null)) AS "Female",
                                                                      to_char(tt2.Child) AS "Child",
                                                                      nvl(to_char(tt2.Infant), ' ') as "Infant"
                                                                    )
                                                      ) ORDER BY tt2.ID_CLASS
                                            )
                               FROM tblPassBag tt2
                              ) as cls
                            )
            )
  INTO cXML_data
  from
  (
    SELECT distinct t1.Units
    FROM tblPassBag t1
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
END SP_WBA_AHM_PASS_BAG_WEIGHT;
/

create or replace PROCEDURE SP_WBA_GET_SET_FLIGHT(cXML_in IN CLOB,
                                                    cXML_out OUT CLOB)
AS
cXML_Data XMLType;
P_ELEM_ID number;
P_ID_AC number;
P_ID_WS number;
P_ID_BORT number;
P_ID_SL number;
P_NR varchar(10);
P_ID_ICAO number;
P_STD date;
P_ETD date;
sTemp varchar(100);
REC_COUNT number;
BEGIN
  select to_number(extractValue(xmltype(cXML_in), '/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.NR, t1.S_DTL_1, t1.E_DTL_1 -- , t1.ID_SL -- компоновка - набудущее
  into P_ID_AC, P_ID_WS, P_ID_BORT, P_NR, P_STD, P_ETD -- , P_ID_SL
  from WB_SHED t1
  where t1.ID = P_ELEM_ID;

  P_STD := nvl(P_STD, sysdate);
  P_ETD := nvl(P_STD, sysdate);

  P_ID_SL := 0;

  select count(t2.ID)
  into REC_COUNT
  from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
  where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS and t1.ID_BORT = P_ID_BORT;

  if REC_COUNT > 0 then
  begin
    select t2.ID
    into P_ID_SL
    from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
    where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS and t1.ID_BORT = P_ID_BORT and rownum <= 1;
  end;
  end if;

  -- Ветка aircompany
  select XMLAGG(
                XMLELEMENT("aircompany",
                            XMLATTRIBUTES(P_ID_AC as "id"),
                            (
                              select XMLAGG(
                                              XMLELEMENT("id_ac",
                                                          XMLATTRIBUTES(e.ID_AC "id",
                                                                        e.NAME_ENG_full "name",
                                                                        e.NAME_RUS_full "name_rus")
                                                        )
                                            )
                              --into cXML_data
                              from (
                                      select i.ID_AC, i.NAME_RUS_full, i.NAME_ENG_full
                                      from WB_REF_AIRCOMPANY_ADV_INFO i
                                      where i.date_from = nvl(
                                                                (
                                                                  select max(ii.date_from)
                                                                  from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                                  where ii.id_ac = i.id_ac and ii.date_from <= sysdate
                                                                ),
                                                                (
                                                                  select min(ii.date_from)
                                                                  from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                                  where ii.id_ac = i.id_ac and ii.date_from > sysdate
                                                                )
                                                              )
                                    ) e
                            )
                          )
                )
  INTO cXML_Data
  from dual;

  cXML_out := cXML_Data.GetCLOBVal();

  -- Ветка AC Type
  select XMLAGG(
                XMLELEMENT("ac_type",
                            XMLATTRIBUTES(P_ID_WS as "id"),
                            (
                              select XMLAGG(
                                              XMLELEMENT("id_ws",
                                                          XMLATTRIBUTES(e.ID_WS "id",
                                                                        e.NAME_ENG_SMALL "name")
                                                        )
                                            )
                              from (
                                      select t1.ID_WS, t2.NAME_ENG_SMALL
                                      from WB_REF_WS_AIR_TYPE t1 inner join WB_REF_WS_TYPES t2 on t1.ID_WS = t2.ID
                                      where t1.ID_AC = P_ID_AC
                                    ) e
                            )
                          )
                )
  INTO cXML_Data
  from dual;

  if cXML_data is not NULL then
  begin
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
  end;
  end if;

-- Ветка AC Reg
  select XMLAGG(
                XMLELEMENT("ac_reg",
                            XMLATTRIBUTES(P_ID_BORT as "id"),
                            (
                              select XMLAGG(
                                              XMLELEMENT("id_bort",
                                                          XMLATTRIBUTES(e.ID "id",
                                                                        e.NAME "name")
                                                        )
                                            )
                              from (
                                      /*
                                      select distinct t1.ID_BORT ID, case when t1.ID_BORT = -1 then 'usual' else t2.NAME_ENG_SMALL end NAME
                                      from WB_REF_WS_AIR_TYPE t1 inner join WB_REF_WS_TYPES t2 on t1.ID = t2.ID
                                      where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                      */
                                      select distinct t3.ID, t3.BORT_NUM NAME
                                      from WB_REF_WS_AIR_TYPE t1 inner join WB_REF_WS_TYPES t2 on t1.ID_WS = t2.ID
                                                                inner join WB_REF_AIRCO_WS_BORTS t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
                                      where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS
                                    ) e
                            )
                          )
                )
  INTO cXML_Data
  from dual;

  if cXML_data is not NULL then
  begin
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
  end;
  end if;

  -- Ветка Config
  select XMLAGG(
                XMLELEMENT("config",
                            XMLATTRIBUTES(P_ID_SL as "id"),
                            (
                              select XMLAGG(
                                              XMLELEMENT("id_sl",
                                                          XMLATTRIBUTES(e.ID "id",
                                                                        e.TABLE_NAME "name")
                                                        )
                                            )
                              from (
                                      /*
                                      select t1.ID, t2.TABLE_NAME
                                      from WB_REF_WS_AIR_S_L_C_ADV t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.IDN = t2.ID
                                      where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS and t1.ID_BORT = P_ID_BORT
                                      */
                                      select t2.ID, t2.TABLE_NAME
                                      from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
                                      where t1.ID_AC = P_ID_AC and t1.ID_WS = P_ID_WS and t1.ID_BORT = P_ID_BORT
                                    ) e
                            )
                          )
                )
  INTO cXML_Data
  from dual;

  if cXML_data is not NULL then
  begin
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
  end;
  end if;

  -- Ветка NR
  select count(t1.ID)
  into REC_COUNT
  from WB_REF_AIRCOMPANY_ADV_INFO t1
  where t1.ID_AC = P_ID_AC and t1.ICAO_CODE = substr(P_NR, 1, 3);

  select t1.ID
  into P_ID_ICAO
  from WB_REF_AIRCOMPANY_ADV_INFO t1
  where t1.ID_AC = P_ID_AC and ((t1.ICAO_CODE = substr(P_NR, 1, 3)) or (REC_COUNT = 0)) and rownum <= 1;

  select XMLAGG(
                XMLELEMENT("nr",
                            XMLATTRIBUTES(P_ID_ICAO as "id", P_NR as "numb"), -- substr(P_NR, 4, 7)
                            (
                              select XMLAGG(
                                              XMLELEMENT("id_oper",
                                                          XMLATTRIBUTES(e.ID "id",
                                                                        e.ICAO_CODE "name")
                                                        )
                                            )
                              from (
                                      select t1.ID, t1.ICAO_CODE
                                      from WB_REF_AIRCOMPANY_ADV_INFO t1
                                      where t1.ID_AC = P_ID_AC
                                    ) e
                            )
                          )
                )
  INTO cXML_Data
  from dual;

  if cXML_data is not NULL then
  begin
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
  end;
  end if;

  -- Ветка std
  select XMLAGG(
                XMLELEMENT("std",
                            XMLATTRIBUTES(to_char(P_STD, 'DD.MM.YYYY') || ' ' || to_char(P_STD, 'HH24:MI') as "value")
                          )
              )
  INTO cXML_Data
  from dual;

  if cXML_data is not NULL then
  begin
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
  end;
  end if;

  -- Ветка etd
  select XMLAGG(
                XMLELEMENT("etd",
                            XMLATTRIBUTES(to_char(P_ETD, 'DD.MM.YYYY') || ' ' || to_char(P_ETD, 'HH24:MI') as "value")
                          )
              )
  INTO cXML_Data
  from dual;

  if cXML_data is not NULL then
  begin
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
  end;
  end if;

  -- Ветка Legs
  select XMLAGG(
                XMLELEMENT("legs",
                            (
                              select XMLAGG(
                                              XMLELEMENT("airport",
                                                          XMLATTRIBUTES(e.ID "id",
                                                                        e.PORTNAME "nodename",
                                                                        e.ImageIndex "ImageIndex",
                                                                        e.SelectedIndex "SelectedIndex",
                                                                        e.parent_id "parent_id",
                                                                        e.Ext0 "ext0",
                                                                        e.Ext1 "ext1")
                                                        )
                                                        order by Ext1
                                            )
                              from (
                                      select t2.ID, t2.NAME_ENG_SMALL || '(' || t2.IATA || ')' PORTNAME, 0 ImageIndex, 0 SelectedIndex, ' ' parent_id, t2.IATA Ext0, 0 Ext1
                                      from WB_SHED t1 inner join WB_REF_AIRPORTS t2 on t1.ID_AP_1 = t2.ID
                                      where t1.ID = P_ELEM_ID
                                      union all
                                      select t2.ID, t2.NAME_ENG_SMALL || '(' || t2.IATA || ')' PORTNAME, 0 ImageIndex, 0 SelectedIndex, ' ' parent_id, t2.IATA Ext0, t2.ID Ext1
                                      from WB_SHED t1 inner join WB_REF_AIRPORTS t2 on t1.ID_AP_2 = t2.ID
                                      where t1.ID = P_ELEM_ID
                                    ) e
                            )
                          )
                )
  INTO cXML_Data
  from dual;

  if cXML_data is not NULL then
  begin
    SYS.DBMS_LOB.APPEND(cXML_out, cXML_data.GetCLOBVal());
  end;
  end if;

  -- if cXML_data is not NULL then
  if cXML_out is not null then
  begin
    -- cXML_out := '<root name="get_query_set_flight" result="ok">' || cXML_data.getClobVal() || '</root>';
    cXML_out := '<root name="get_query_set_flight" result="ok">' || cXML_out || '</root>';
  end;
  end if;

  commit;
END SP_WBA_GET_SET_FLIGHT;
/

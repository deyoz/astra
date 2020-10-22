create or replace PROCEDURE SP_WB_GET_SET_FLIGHT(cXML_in IN CLOB,
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
BEGIN
  select to_number(extractValue(xmltype(cXML_in), '/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.ID_SL, t1.NR, t1.STD, t1.ETD
  into P_ID_AC, P_ID_WS, P_ID_BORT, P_ID_SL, P_NR, P_STD, P_ETD
  from WB_SCHED t1
  where t1.ID = P_ELEM_ID;

  -- ��⪠ aircompany
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

  -- ��⪠ AC Type
  select XMLAGG(
                XMLELEMENT("ac_type",
                            XMLATTRIBUTES(P_ID_WS as "id"),
                            (
                              select XMLAGG(
                                              XMLELEMENT("id_ws",
                                                          XMLATTRIBUTES(e.ID "id",
                                                                        e.NAME_ENG_SMALL "name")
                                                        )
                                            )
                              from (
                                      select t1.ID, t2.NAME_ENG_SMALL
                                      from WB_REF_WS_AIR_TYPE t1 inner join WB_REF_WS_TYPES t2 on t1.ID = t2.ID
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

-- ��⪠ AC Reg
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

  -- ��⪠ Config
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

  -- ��⪠ NR
  select t1.ID
  into P_ID_ICAO
  from WB_REF_AIRCOMPANY_ADV_INFO t1
  where t1.ID_AC = P_ID_AC and t1.ICAO_CODE = substr(P_NR, 1, 3);

  select XMLAGG(
                XMLELEMENT("nr",
                            XMLATTRIBUTES(P_ID_ICAO as "id", substr(P_NR, 4, 7) as "numb"),
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

  -- ��⪠ std
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

  -- ��⪠ etd
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

  -- ��⪠ Legs
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
                                      from WB_SCHED_MRSHR t1 inner join WB_REF_AIRPORTS t2 on t1.ID_AP1 = t2.ID
                                                            inner join (select min(ID) ID from WB_SCHED_MRSHR where ELEM_ID = P_ELEM_ID) t3 on t1.ID = t3.ID
                                      where t1.ELEM_ID = P_ELEM_ID
                                      union all
                                      select t2.ID, t2.NAME_ENG_SMALL || '(' || t2.IATA || ')' PORTNAME, 0 ImageIndex, 0 SelectedIndex, ' ' parent_id, t2.IATA Ext0, t2.ID Ext1
                                      from WB_SCHED_MRSHR t1 inner join WB_REF_AIRPORTS t2 on t1.ID_AP2 = t2.ID
                                      where t1.ELEM_ID = P_ELEM_ID
                                      -- order by Ext1
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
END SP_WB_GET_SET_FLIGHT;
/

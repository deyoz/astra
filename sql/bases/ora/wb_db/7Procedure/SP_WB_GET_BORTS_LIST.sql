create or replace PROCEDURE SP_WB_GET_BORTS_LIST(cXML_in IN CLOB,
                                                    cXML_out OUT CLOB)
AS
cXML_data XMLType;
P_AC_ID number; P_WS_ID number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@ac_id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/@ws_id'))
  into P_AC_ID, P_WS_ID
  from dual;

  select XMLAGG(
                  XMLELEMENT("rec",
                              XMLATTRIBUTES(e.ID "id",
                                            e.NAME "name")
                            )
                )
  into cXML_data
  from (
          select distinct t3.ID, t3.BORT_NUM NAME
          from WB_REF_WS_AIR_TYPE t1 inner join WB_REF_WS_TYPES t2 on t1.ID_WS = t2.ID
                                    inner join WB_REF_AIRCO_WS_BORTS t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
          where t1.ID_AC = P_AC_ID and t1.ID_WS = P_WS_ID
          order by ID
        ) e;

  if cXML_data is not NULL then
  begin
    cXML_out := '<root name="get_borts_list" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<root name="get_borts_list" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WB_GET_BORTS_LIST;
/

create or replace PROCEDURE SP_WB_GET_CONFIG_LIST(cXML_in IN CLOB,
                                                    cXML_out OUT CLOB)
AS
cXML_data XMLType;
P_AC_ID number;
P_WS_ID number;
P_BORT_ID number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@ac_id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/@ws_id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/@bort_id'))
  into P_AC_ID, P_WS_ID, P_BORT_ID
  from dual;

  select XMLAGG(
                  XMLELEMENT("rec",
                              XMLATTRIBUTES(e.ID "id",
                                            e.TABLE_NAME "name")
                            ) ORDER BY e.ID
                )
  into cXML_data
  from (
          select t2.ID, t2.TABLE_NAME
          from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
          where t1.ID_BORT = P_BORT_ID
        ) e;

  if cXML_data is not NULL then
  begin
    cXML_out := '<root name="get_config_list" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_GET_CONFIG_LIST;
/

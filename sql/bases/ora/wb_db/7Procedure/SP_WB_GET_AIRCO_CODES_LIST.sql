create or replace PROCEDURE SP_WB_GET_AIRCO_CODES_LIST(cXML_in IN CLOB,
                                                    cXML_out OUT CLOB)
AS
cXML_data XMLType;
P_ID number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@ac_id'))
  into P_ID
  from dual;

  select XMLAGG(
                  XMLELEMENT("rec",
                              XMLATTRIBUTES(e.ID "id",
                                            e.ICAO_CODE "name")
                            )
                )
  into cXML_data
  from (
          select t1.ID, t1.ICAO_CODE
          from WB_REF_AIRCOMPANY_ADV_INFO t1
          where t1.ID_AC = P_ID
        ) e;

  if cXML_data is not NULL then
  begin
    cXML_out := '<root name="get_airco_codes_list" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_GET_AIRCO_CODES_LIST;
/

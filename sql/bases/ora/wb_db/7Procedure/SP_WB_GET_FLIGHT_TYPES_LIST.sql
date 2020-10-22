create or replace PROCEDURE SP_WB_GET_FLIGHT_TYPES_LIST(cXML_in IN CLOB,
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
                                            e.NAME_ENG_SMALL "name")
                            )
                )
  into cXML_data
  from (
          select t1.ID, t2.NAME_ENG_SMALL
          from WB_REF_WS_AIR_TYPE t1 inner join WB_REF_WS_TYPES t2 on t1.ID_WS = t2.ID
          where t1.ID_AC = P_ID
        ) e;

  if cXML_data is not NULL then
  begin
    cXML_out := '<root name="get_flight_types_list" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_GET_FLIGHT_TYPES_LIST;
/

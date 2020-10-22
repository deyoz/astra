create or replace PROCEDURE SP_WB_GET_AIRPORT_LIST(cXML_in IN CLOB,
                                                    cXML_out OUT CLOB)
AS
cXML_data XMLType;

BEGIN
  -- Получит входной параметр
  select XMLAGG(
                  XMLELEMENT("rec",
                              XMLATTRIBUTES(e.ID "id",
                                            e.PORTNAME "nodename",
                                            e.ImageIndex "ImageIndex",
                                            e.SelectedIndex "SelectedIndex",
                                            e.parent_id "parent_id",
                                            e.Ext0 "ext0")
                            )
                )
  into cXML_data
  from (
          select t1.ID, t1.NAME_ENG_SMALL || '(' || t1.IATA || ')' PORTNAME, 0 ImageIndex, 0 SelectedIndex, ' ' parent_id, t1.IATA Ext0
          from WB_REF_AIRPORTS t1
        ) e;

  if cXML_data is not NULL then
  begin
    select replace(cXML_data, '" "', '""') into cXML_out from dual;
    -- cXML_out := '<root name="get_airport_list" result="ok">' || cXML_data.getClobVal() || '</root>';
    cXML_out := '<root name="get_airport_list" result="ok">' || cXML_out || '</root>';
  end;
  end if;

  commit;
END SP_WB_GET_AIRPORT_LIST;
/

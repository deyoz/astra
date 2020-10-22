create or replace PROCEDURE SP_WB_REF_GET_AIRPORTS
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType;

BEGIN
  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("airport",
                              XMLATTRIBUTES(tt1.ID "id", tt1.en_name "en_name", tt1.ru_name "ru_name", tt1.en_full_name "en_full_name", tt1.ru_full_name "ru_full_name")
                            )
                )
  INTO cXML_data
  from
  (
    select t1.ID, t1.IATA en_name, t1.AP ru_name, t1.NAME_ENG_FULL en_full_name, t1.NAME_RUS_FULL ru_full_name
    from WB_REF_AIRPORTS t1
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_airports" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_airports" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_AIRPORTS;
/

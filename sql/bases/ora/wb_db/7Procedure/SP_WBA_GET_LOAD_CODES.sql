create or replace PROCEDURE SP_WBA_GET_LOAD_CODES
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; vID_AC number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC
  into vID_AC
  from WB_SHED t1
  where t1.ID = P_ID;

  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("code",
                              XMLATTRIBUTES(tt1.CODE_NAME "Code", tt1.DESCRIPTION "Description")
                            )
                )
  INTO cXML_data
  from
  (
    select t1.CODE_NAME, t1.DESCRIPTION
    from WB_REF_AIRCO_LOAD_CODES t1
    where t1.ID_AC = vID_AC
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_airline_load_codes" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WBA_GET_LOAD_CODES;
/

create or replace PROCEDURE SP_WB_REF_GET_USER_SETT_OLD
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
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("deadload_image",
                              XMLATTRIBUTES(tt1.scale "scale")
                            )
                )
  INTO cXML_data
  from
  (
    select 9 scale
    from dual
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" ?><root name="get_user_settings" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_user_settings" result="ok1"></root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_USER_SETT_OLd;

/*
<root name="get_user_settings" result="ok">
    <deadload_image scale="9"/> <!-- Целое от 5 до 100. -->
</root>*/
/

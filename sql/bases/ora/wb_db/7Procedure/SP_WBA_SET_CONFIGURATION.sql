create or replace procedure SP_WBA_SET_CONFIGURATION
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
P_ID_SL number := -1;
begin
  SP_WB_DELETE_CALCS(cXML_in, cXML_out); -- удаляем расчтеы для данного рейса

  -- Изменить конфигурацию в расписании
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id')), to_number(extractValue(xmltype(cXML_in),'/root[1]/@id_sl'))
  into P_ELEM_ID, P_ID_SL
  from dual;

  /* конфигурация - на будущее
  if P_ID_SL > 0 then
  begin
    update WB_SHED
    set ID_SL = P_ID_SL
    where ID = P_ELEM_ID;
  end;
  end if;
  */

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
-->
<root name="set_configuration" result="ok">
</root>';
  commit;
end SP_WBA_SET_CONFIGURATION;
/

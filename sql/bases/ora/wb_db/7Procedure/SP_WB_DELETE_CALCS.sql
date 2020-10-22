create or replace procedure SP_WB_DELETE_CALCS
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  delete
  from WB_CALCS_XML t1
  where ELEM_ID = P_ELEM_ID;

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
-->
<root name="delete_calcs" result="ok">
</root>';
  commit;
end SP_WB_DELETE_CALCS;
/

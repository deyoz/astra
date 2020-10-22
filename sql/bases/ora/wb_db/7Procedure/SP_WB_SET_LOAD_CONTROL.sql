create or replace procedure SP_WB_SET_LOAD_CONTROL
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
TABLEVAR varchar(40):='LoadControl';
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select replace(cXML_in, '"set_load_control"', '"get_load_control"') into cXML_out from dual;

/*
  select to_char(extractValue(xmltype(cXML_in),'/root[1]/@date'), 'DD.MM.YYYY')
  into vDATE
  from dual;

  select to_char(extractValue(xmltype(cXML_in),'/root[1]/@time'), 'HH24:MI')
  into vTIME
  from dual;

  select to_date(vDATE + ' ' + vTIME, 'DD.MM.YYYY HH24:MI')
  into vDT
  from dual;
*/

  insert into WB_CALCS_LOAD_CONTROL_XML (ELEM_ID, XML_VALUE, DT)
  values (P_ELEM_ID, cXML_out, sysdate);

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
-->
<root name="set_load_control" result="ok">
</root>';

  commit;
end SP_WB_SET_LOAD_CONTROL;
/

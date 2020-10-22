create or replace procedure SP_WB_SAVE_LOADSHEET_DOC
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number := -1;
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'LoadSheet';
DT_LAST date;
cXML_LS clob;
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select max(t1.DT)
  into DT_LAST
  from WB_CALCS_XML t1
  where t1.DATA_NAME = 'LoadSheet' and t1.ELEM_ID = P_ELEM_ID;

  select t1.XML_VALUE
  into cXML_LS
  from WB_CALCS_XML t1
  where t1.DATA_NAME = 'LoadSheet' and t1.ELEM_ID = P_ELEM_ID and t1.DT = DT_LAST;

   -- Сравнить данные из cXML_LS и cXML_in

  select replace(cXML_in, '"save_loadsheet_doc"', '"get_loadsheet_doc"') into cXML_out from dual;

  insert into WB_CALCS_XML (ELEM_ID, DATA_NAME, XML_VALUE)
  values (P_ELEM_ID, TABLEVAR, cXML_out);

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
-->
<root name="save_loadsheet_doc" result="ok">
</root>';
  commit;
end SP_WB_SAVE_LOADSHEET_DOC;
/

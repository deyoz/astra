create or replace procedure SP_WB_SET_DOW_DATA_NEW
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
TABLEVAR varchar(40):='DOWData';
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select replace(cXML_in, '"set_dow_data"', '"get_dow_data"') into cXML_out from dual;

  select count(t1.ELEM_ID) into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ELEM_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
    begin
      update WB_CALCS_XML
      set XML_VALUE = cXML_out
      where ELEM_ID = P_ELEM_ID and DATA_NAME = TABLEVAR;
    end;
  else
    begin
      insert into WB_CALCS_XML (ELEM_ID, DATA_NAME, XML_VALUE)
      values (P_ELEM_ID, TABLEVAR, cXML_out);
    end;
  end if;

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<!--
    �⢥�.
-->
<root name="set_dow_data" result="ok">
</root>';
  commit;
end SP_WB_SET_DOW_DATA_NEW;
/

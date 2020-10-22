create or replace procedure SP_WB_REF_SET_PASSENGERS_OLD
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
TABLEVAR varchar(40):='PassengersDetails';
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select replace(cXML_in, '"set_passengers_details"', '"get_passengers_details"') into cXML_out from dual;

  select count(t1.ELEM_ID) into REC_COUNT
  from WB_TMP_XML t1
  where t1.ELEM_ID = P_ELEM_ID and t1.TABLENAME = TABLEVAR;

  if REC_COUNT > 0 then
    begin
      update WB_TMP_XML
      set XML_VALUE = cXML_out
      where ELEM_ID = P_ELEM_ID and TABLENAME = TABLEVAR;
    end;
    else
    begin
      insert into WB_TMP_XML (ELEM_ID, TABLENAME, XML_VALUE)
      values (P_ELEM_ID, TABLEVAR, cXML_out);
    end;
  end if;

  cXML_out := '<?xml version="1.0" encoding="utf-8"?>
<!--
    �⢥�.
-->
<root name="set_passengers_details" result="ok">
</root>';
  commit;
end SP_WB_REF_SET_PASSENGERS_OLD;
/

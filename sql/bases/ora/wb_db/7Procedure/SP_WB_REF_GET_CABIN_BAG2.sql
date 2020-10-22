create or replace PROCEDURE SP_WB_REF_GET_CABIN_BAG2(cXML_in IN clob, cXML_out OUT CLOB)
AS
cXML_data clob:='';
P_ELEM_ID number:=-1;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select t1.XML_VALUE
  INTO cXML_data
  from
  (
    select t1.XML_VALUE
    from WB_TMP_XML t1
    where t1.TABLENAME = 'CabinBaggageByZone' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;
/*  cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_cabin_baggage_by_zone" elem_id="3458" result="ok">
  <class code="C">
    <section Name="0A" CabinBaggage="30"/>
  </class>
  <class code="Y">
    <section Name="0B" CabinBaggage="50"/>
    <section Name="0C" CabinBaggage="100"/>
    <section Name="0D" CabinBaggage="50"/>
  </class>
</root>';
*/
cXML_out:=cXML_data;
END SP_WB_REF_GET_CABIN_BAG2;
/

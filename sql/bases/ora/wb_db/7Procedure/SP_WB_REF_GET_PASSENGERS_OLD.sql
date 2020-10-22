create or replace PROCEDURE SP_WB_REF_GET_PASSENGERS_OLD(cXML_in IN clob,
                                           cXML_out OUT CLOB)
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
    where t1.TABLENAME = 'PassengersDetails' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;

/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_passengers_details" elem_id="3457" result="ok">
  <ARR ru_id="русский" id="26">
    <On Weight="5335">
      <class code="C" Male="3" Female="3" Child="3" Infant="0" CabinBaggage="30"/>
      <class code="Y" Male="25" Female="30" Child="10" Infant="" CabinBaggage="200"/>
    </On>
    <Tr Weight="0">
      <class code="C" Male="" Female="" Child="" Infant="" CabinBaggage=""/>
      <class code="Y" Male="" Female="" Child="" Infant="" CabinBaggage=""/>
    </Tr>
  </ARR>
  <Total Weight="5335">
    <class code="C" Male="3" Female="3" Child="3" Infant="0" CabinBaggage="30"/>
    <class code="Y" Male="25" Female="30" Child="10" Infant="0" CabinBaggage="200"/>
  </Total>
</root>';
*/

cXML_out:=cXML_data;
END SP_WB_REF_GET_PASSENGERS_OLD;
/

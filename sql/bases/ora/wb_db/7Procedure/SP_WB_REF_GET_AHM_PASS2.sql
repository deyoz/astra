create or replace PROCEDURE SP_WB_REF_GET_AHM_PASS2 (cXML_in IN clob, cXML_out OUT CLOB)
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
    where t1.TABLENAME = 'AHMPBWeights' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;
/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root
name="get_ahm_passenger_and_baggage_weights"
result="ok">
  <weights Units="Kg">
    <class code="C"
        Male="80"
        Female="75"
        Child="30"
        Infant="15"
    />
    <class code="Y"
        Male="80"
        Female="75"
        Child="30"
        Infant="15"
    />
  </weights>
</root>';
*/

cXML_out:=cXML_data;
END SP_WB_REF_GET_AHM_PASS2;
/

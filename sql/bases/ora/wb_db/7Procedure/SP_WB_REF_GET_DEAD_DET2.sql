create or replace PROCEDURE SP_WB_REF_GET_DEAD_DET2 (cXML_in IN clob, cXML_out OUT CLOB)
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
    where t1.TABLENAME = 'DeadloadDetails' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;
/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_deadload_details" elem_id="3458" result="ok">
  <row TypeOfLoad="C" DestId="26" NetWeight="293" Pcs="1" BulkVol="15.2" IsBulk="Y" Position="" PositionLocked="N" ULDIATAType="" ULDTypeID="" ULDOwnerCode="" Serial="" DOW="N" Priority="" DGSL="" Finalized="N"/>
  <row TypeOfLoad="M" DestId="26" NetWeight="989.5" Pcs="4" BulkVol="5" IsBulk="Y" Position="" PositionLocked="N" ULDIATAType="" ULDTypeID="" ULDOwnerCode="" Serial="" DOW="N" Priority="" DGSL="" Finalized="N"/>
</root>';
*/
cXML_out:=cXML_data;
END SP_WB_REF_GET_DEAD_DET2;
/
